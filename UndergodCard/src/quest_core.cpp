#include "quest_core.hpp"
#include <utility>

namespace undergod {
namespace {
PhaseDef BasePhase(const EnemyDef& enemy){
    PhaseDef phase;
    phase.name=enemy.name;
    phase.hp=enemy.hp;
    phase.min_damage=enemy.min_damage;
    phase.max_damage=enemy.max_damage;
    phase.block=enemy.block;
    phase.attack_chance=enemy.attack_chance;
    phase.opening_attack=enemy.opening_attack;
    phase.max_guards=enemy.max_guards;
    phase.patterns=enemy.patterns;
    return phase;
}
EnemyDef Scaled(EnemyDef enemy,const CombatMods& mods){
    auto stats=[&](int& hp,int& lo,int& hi,std::vector<IntentPattern>& patterns){hp=std::max(1,hp*mods.enemy_hp_percent/100);if(hi>0){lo+=mods.enemy_damage;hi+=mods.enemy_damage;}for(auto& p:patterns)if(p.max_damage>0){p.min_damage+=mods.enemy_damage;p.max_damage+=mods.enemy_damage;}};
    stats(enemy.hp,enemy.min_damage,enemy.max_damage,enemy.patterns);
    for(auto& p:enemy.phases)stats(p.hp,p.min_damage,p.max_damage,p.patterns);
    return enemy;
}
}

Combat::Combat(Config c):config(std::move(c)){Reset();}

void Combat::Shuffle(std::vector<std::string>& pile){
    for(int i=int(pile.size())-1;i>0;--i) std::swap(pile[i],pile[int(shuffle_rng_.Next()*(i+1))]);
}

const PhaseDef& Combat::CurrentPhase() const {return phases_.at(phase_index_);}

void Combat::RollIntent(){
    if(live.charge_left>0){intent.attack=live.charge_left<=1;return;}
    const auto& current=CurrentPhase();
    const bool opening=phase_opening_||(turn==0&&current.opening_attack);
    phase_opening_=false;
    if(stars.enabled&&HasBossDeck()){
        const bool charge_opening=opening&&!current.patterns.empty()&&std::all_of(current.patterns.begin(),current.patterns.end(),[](const IntentPattern& p){return p.kind==IntentKind::Charge;});
        intent=DrawBossIntent(opening||guard_streak_>=current.max_guards,true,{},charge_opening);
        if(intent.kind==IntentKind::Charge){const auto* card=BossCardIdentity(enemy_id_,intent);live.charge_left=card->action.charge_turns;intent.attack=live.charge_left<=1;}
        guard_streak_=intent.attack||intent.kind==IntentKind::Charge?0:guard_streak_+1;
        return;
    }
    intent={};
    if(current.patterns.empty()){
        const bool rolled=intent_rng_.Next()<current.attack_chance;
        intent.attack=rolled||opening||guard_streak_>=current.max_guards;
        guard_streak_=intent.attack?0:guard_streak_+1;
        if(intent.attack){
            intent.kind=IntentKind::Attack;
            const int span=current.max_damage-current.min_damage+1;
            intent.value=current.min_damage+int(intent_rng_.Next()*span);
        }else{
            intent.kind=IntentKind::Defend;
            intent.value=current.block;
        }
        return;
    }
    const bool force_attack=opening||guard_streak_>=current.max_guards;
    std::vector<const IntentPattern*> pool;
    for(const auto& pattern:current.patterns) if(!force_attack||pattern.kind==IntentKind::Attack||pattern.kind==IntentKind::MultiAttack||pattern.kind==IntentKind::Charge||pattern.kind==IntentKind::Mimic) pool.push_back(&pattern);
    if(pool.empty()) for(const auto& pattern:current.patterns) pool.push_back(&pattern);
    int total=0;
    for(const auto* pattern:pool) total+=std::max(1,pattern->weight);
    const double roll=intent_rng_.Next()*total;
    int acc=0;
    const IntentPattern* picked=pool.back();
    for(const auto* pattern:pool){acc+=std::max(1,pattern->weight); if(roll<acc){picked=pattern;break;}}
    intent.kind=picked->kind;
    intent.status=picked->status;
    intent.status_amount=std::max(0,picked->status_amount);
    intent.hits=picked->hits;intent.summon_id=picked->summon_id;
    intent.attack=picked->kind==IntentKind::Attack||picked->kind==IntentKind::MultiAttack||picked->kind==IntentKind::Mimic||picked->kind==IntentKind::Drain;
    if(intent.attack){
        const int span=std::max(1,picked->max_damage-picked->min_damage+1);
        intent.value=picked->min_damage+int(intent_rng_.Next()*span);
        guard_streak_=0;
    }else{
        intent.value=std::max(0,picked->block);
        ++guard_streak_;
    }
    if(intent.kind==IntentKind::Charge){live.charge_left=std::max(1,picked->charge_turns);intent.value=picked->min_damage+int(intent_rng_.Next()*(picked->max_damage-picked->min_damage+1));intent.attack=live.charge_left<=1;}
    if(intent.kind==IntentKind::Mimic&&!live.last_turn.empty())intent.card_id=live.last_turn.back();
}

void Combat::BeginPlayer(){
    ++turn;
    phase=Phase::Player;
    player.block=live.retain_turns>0?player.block:std::min(player.block,mods_.retain_block);
    if(live.retain_turns>0)--live.retain_turns;
    live.thorns=0;live.double_used=false;live.played_turn=live.attacks_turn=0;
    bool phased=false;
    TickPoison(false,phased);
    CheckEnd();if(phase==Phase::Lost||phase==Phase::Won)return;
    const int prepared=live.next_energy;
    energy=std::clamp(config.balance.energy+mods_.energy+prepared,0,10);live.next_energy=0;
    Draw(std::clamp(config.balance.draw+mods_.draw+(stars.enabled?prepared+mods_.energy*3+(mods_.mana?std::max(0,mods_.max_mana-3):0):0),1,10));
    if(turn==1) Cast(mods_.combat_start);
    if(phase==Phase::Player) Cast(mods_.turn_start);
    if(live.curse_next&&!hand.empty()){
        const int index=std::min(int(hand.size())-1,int(target_rng_.Next()*hand.size()));exhaust.push_back(hand[index]);hand[index]="mud";created.push_back("mud");live.curse_next=false;
    }
    LockCard();
}

void Combat::CheckEnd(){
    if(player.hp<=0){
        if(live.revive_ready&&live.revives==0&&!mods_.revive_spent){player.hp=std::max(1,player.max_hp*30/100);live.revives=1;live.revive_ready=false;events.push_back({EventType::Revive,false,player.hp});}
        else phase=Phase::Lost;
    }
    if(player.hp>0&&Targets().empty())phase=Phase::Won;
}

void Combat::AdvancePhaseIfAny(){
    if(phase_index_+1>=int(phases_.size()))return;
    const auto& upcoming=phases_[phase_index_+1];
    if(upcoming.threshold>0){if(enemy.hp<=0||enemy.hp>enemy.max_hp*upcoming.threshold/100)return;}
    else if(enemy.hp>0)return;
    ++phase_index_;
    const auto& next=phases_[phase_index_];
    if(next.threshold==0)enemy={next.hp,next.hp,0,{}};
    if(next.cleanse)enemy.status={};
    enemy.status.strength+=std::max(0,next.strength);
    live.charge_left=0;live.constrict_left=next.constrict;live.doom_left=next.doom;live.doom_triggered=false;
    guard_streak_=0;
    phase_opening_=next.opening_attack;
    events.push_back({EventType::Phase,true,phase_index_+1});
    RollIntent();
    if(next.summon_count>0)Summon(next.summon_id,next.summon_count,definition_.mechanic=="fox");
    LockCard();
    AdvancePhaseIfAny();
}

void Combat::Hit(bool target_enemy,int damage,bool pierce){
    if(target_enemy){HitTarget(live.target,damage,pierce,false);return;}
    auto& target=target_enemy?enemy:player;
    const auto result=ResolveDamage(target,damage,pierce);
    events.push_back({EventType::Damage,target_enemy,result.lost,result.blocked});
    live.damage_taken+=result.lost;
    if(damage>0&&live.thorns>0)HitTarget(attacker_,live.thorns,false,true);
    CheckEnd();
}

void Combat::AddStatusEvent(Fighter& fighter,bool enemy_target,StatusId id,int amount){
    if(amount==0) return;
    AddStatusStack(fighter,id,amount);
    int unit=0;
    if(enemy_target && &fighter!=&enemy)for(int i=0;i<int(adds.size());++i)if(&fighter==&adds[i].fighter){unit=i+1;break;}
    events.push_back({EventType::Status,enemy_target,amount,0,id,unit});
}

bool Combat::TickPoison(bool enemy_target,bool& phased){
    phased=false;
    auto& fighter=enemy_target?enemy:player;
    if(fighter.status.poison<=0) return false;
    const int damage=fighter.status.poison;
    fighter.status.poison=std::max(0,fighter.status.poison-1);
    const auto result=ResolveDamage(fighter,damage,true);
    events.push_back({EventType::Damage,enemy_target,result.lost,0});
    if(enemy_target){
        live.damage_dealt+=result.lost;
        if(enemy.hp==0&&live.enemy_revive==1){enemy.hp=std::max(1,enemy.max_hp*30/100);live.enemy_revive=2;events.push_back({EventType::Revive,true,enemy.hp});}
        const int before=phase_index_;
        AdvancePhaseIfAny();
        phased=phase_index_!=before;
    }
    return true;
}

void Combat::ApplyOne(const Effect& effect,bool from_card){
    Effect scaled=effect;
    if(scaled.target==EffectTarget::AllEnemies){for(int target:Targets()){live.target=target;auto single=scaled;single.target=EffectTarget::Enemy;ApplyOne(single,from_card);}return;}
    if(scaled.target==EffectTarget::RandomEnemy){const auto targets=Targets();if(targets.empty())return;live.target=targets[std::min(int(targets.size())-1,int(target_rng_.Next()*targets.size()))];scaled.target=EffectTarget::Enemy;}
    if(scaled.target==EffectTarget::Enemy){const auto targets=Targets();if(targets.empty())return;if(live.target<0||live.target>int(adds.size())||Target(live.target).hp<=0)live.target=targets.front();}
    if(from_card&&scaled.op==EffectOp::Damage&&scaled.target==EffectTarget::Enemy)scaled.amount=(scaled.amount+mods_.bonus_damage+live.card_bonus)*live.card_multiplier;
    if(from_card&&scaled.op==EffectOp::Block&&scaled.target==EffectTarget::Self) scaled.amount+=mods_.bonus_block;
    auto& target=scaled.target==EffectTarget::Enemy?Target(live.target):player;
    const bool enemy_target=scaled.target==EffectTarget::Enemy;
    switch(scaled.op){
    case EffectOp::Damage:{
        const int damage=ScaleDamage(scaled.amount,player.status,target.status);
        if(enemy_target)HitTarget(live.target,damage,scaled.pierce,scaled.ranged);else Hit(false,damage,scaled.pierce);
        break;
    }
    case EffectOp::Block:
        target.block+=scaled.amount;
        events.push_back({EventType::Block,enemy_target,scaled.amount});
        break;
    case EffectOp::Heal:{
        const int amount=std::min(scaled.amount,target.max_hp-target.hp);
        target.hp+=amount;
        events.push_back({EventType::Heal,enemy_target,amount});
        break;
    }
    case EffectOp::Apply:
        AddStatusEvent(target,enemy_target,scaled.status,scaled.amount);
        if(enemy_target&&scaled.status==StatusId::Poison)live.poison_applied+=scaled.amount;
        break;
    case EffectOp::Draw:
        Draw(scaled.amount);
        break;
    case EffectOp::Energy:
        if(stars.enabled)Draw(scaled.amount);else energy=std::min(10,energy+scaled.amount);
        break;
    case EffectOp::Rage:
        rage=std::min(config.balance.rage_max,rage+scaled.amount);
        break;
    case EffectOp::Retain:live.retain_turns=std::max(live.retain_turns,scaled.amount);break;
    case EffectOp::Thorns:live.thorns+=scaled.amount;break;
    case EffectOp::Revive:if(live.revives==0&&!mods_.revive_spent)live.revive_ready=true;break;
    case EffectOp::DoubleAttack:live.double_attack=std::max(2,scaled.amount);break;
    case EffectOp::EndBlock:live.end_block+=scaled.amount;break;
    case EffectOp::UltDiscount:live.ult_discount+=scaled.amount;break;
    case EffectOp::TempStrength:player.status.strength+=scaled.amount;live.temp_strength+=scaled.amount;break;
    case EffectOp::NextEnergy:live.next_energy+=scaled.amount;break;
    }
}

void Combat::Cast(const std::vector<Effect>& effects){
    if(effects.empty()||phase==Phase::Lost||phase==Phase::Won) return;
    for(const auto& effect:effects){
        const int times=std::max(1,effect.times);
        for(int n=0;n<times&&phase!=Phase::Lost&&phase!=Phase::Won;++n) ApplyOne(effect,false);
    }
    if(phase!=Phase::Lost&&phase!=Phase::Won) CheckEnd();
}

void Combat::ResolveIntent(){
    const auto action=intent;attacker_=0;
    const int multiplier=std::max(1,live.enemy_double);
    if(action.kind==IntentKind::Charge){
        if(live.charge_left>0)--live.charge_left;
        if(live.charge_left>0){intent.attack=live.charge_left<=1;return;}
        Hit(false,ScaleDamage(action.value*multiplier,enemy.status,player.status));return;
    }
    if(action.kind==IntentKind::Summon){Summon(action.summon_id,std::max(1,action.hits));return;}
    if(action.kind==IntentKind::Curse){live.curse_next=true;return;}
    if(action.kind==IntentKind::Mimic&&!action.card_id.empty()){
        const auto card=StarCardView(Definition(action.card_id));
        for(const auto& effect:card.effects)for(int n=0;n<effect.times&&player.hp>0&&enemy.hp>0;++n){
            if(effect.op==EffectOp::Damage)Hit(false,ScaleDamage(effect.amount*multiplier,enemy.status,player.status),effect.pierce);
            else if(effect.op==EffectOp::Block)enemy.block+=effect.amount;
            else if(effect.op==EffectOp::Heal)enemy.hp=std::min(enemy.max_hp,enemy.hp+effect.amount);
            else if(effect.op==EffectOp::Apply)AddStatusStack(effect.target==EffectTarget::Self?enemy:player,effect.status,effect.amount);
            else if(effect.op==EffectOp::TempStrength)enemy.status.strength+=effect.amount;
            else if(effect.op==EffectOp::Draw||effect.op==EffectOp::Energy)enemy.status.strength+=std::min(2,effect.amount);
            else if(effect.op==EffectOp::Rage)rage=std::max(0,rage-effect.amount);
            else if(effect.op==EffectOp::EndBlock)live.enemy_end_block+=effect.amount;
            else if(effect.op==EffectOp::Retain)live.enemy_retain=std::max(live.enemy_retain,effect.amount);
            else if(effect.op==EffectOp::Thorns)live.enemy_thorns+=effect.amount;
            else if(effect.op==EffectOp::DoubleAttack)live.enemy_double=std::max(2,effect.amount);
            else if(effect.op==EffectOp::Revive){if(live.enemy_revive==0)live.enemy_revive=1;}
            else if(effect.op==EffectOp::UltDiscount||effect.op==EffectOp::NextEnergy)enemy.status.strength+=std::min(2,effect.amount);
        }
        return;
    }
    if(action.kind==IntentKind::Drain)rage=std::max(0,rage-action.status_amount);
    if(action.attack||action.kind==IntentKind::Drain){
        const int old_phase=phase_index_;
        for(int hit=0;hit<std::max(1,action.hits)&&phase!=Phase::Lost&&enemy.hp>0&&phase_index_==old_phase;++hit)Hit(false,ScaleDamage(action.value*multiplier,enemy.status,player.status));
        if(player.hp>0&&action.status_amount>0&&action.kind!=IntentKind::Drain)AddStatusEvent(player,false,action.status,action.status_amount);
        return;
    }
    if(action.kind==IntentKind::Buff){
        if(action.value>0){enemy.block+=action.value;events.push_back({EventType::Block,true,action.value});}
        if(action.status_amount>0)AddStatusEvent(enemy,true,action.status,action.status_amount);
        return;
    }
    if(action.kind==IntentKind::Debuff){
        if(action.status_amount>0&&player.hp>0)AddStatusEvent(player,false,action.status,action.status_amount);
        if(action.value>0)Hit(false,ScaleDamage(action.value,enemy.status,player.status));return;
    }
    enemy.block=action.value;events.push_back({EventType::Block,true,action.value});
}

void Combat::BeginEncounter(const EnemyDef& enemy_def,uint32_t seed,int hp,int max_hp,int kept_rage,const std::vector<std::string>& deck,CombatMods mods){
    mods_=std::move(mods);
    definition_=Scaled(enemy_def,mods_);live={};stars={};reserved_play_=false;adds.clear();exhaust.clear();created.clear();attacker_=0;
    live.airborne=enemy_def.airborne;
    enemy_id_=enemy_def.id;
    shuffle_rng_.Seed(seed);
    intent_rng_.Seed(seed^0x19ABCu);
    target_rng_.Seed(seed^0xACED128Bu);
    ultimate_pending_=false;
    phase_opening_=false;
    guard_streak_=0;
    phase_index_=0;
    role_=enemy_def.role.empty()?"combat":enemy_def.role;
    phases_.clear();
    phases_.push_back(BasePhase(definition_));
    for(const auto& extra:definition_.phases)phases_.push_back(extra);
    const int safe_max=std::max(1,max_hp);
    const int safe_hp=std::min(std::max(0,hp),safe_max);
    player={safe_hp,safe_max,0,{}};
    enemy={phases_[0].hp,phases_[0].hp,0,{}};
    energy=0;
    rage=std::max(0,kept_rage);
    turn=0;
    draw=deck;
    hand.clear();
    discard.clear();
    events.clear();
    intent={};
    Shuffle(draw);
    RollIntent();
    BeginPlayer();
}

void Combat::Reset(uint32_t seed){
    BeginEncounter(config.enemy,seed,config.balance.player_hp,config.balance.player_hp,0,StartingDeck(config));
}

const CardDef& Combat::Definition(const std::string& id) const {
    for(const auto& card:config.cards) if(card.id==id) return card;
    throw std::runtime_error("Unknown card: "+id);
}

void Combat::Draw(int count){
    for(int i=0;i<count;++i){
        if(stars.enabled&&hand.size()>=10)break;
        if(draw.empty()){draw=std::move(discard); discard.clear(); Shuffle(draw);}
        if(draw.empty()) break;
        hand.push_back(draw.back());
        draw.pop_back();
    }
}

bool Combat::CanPlay(int index) const {
    return phase==Phase::Player&&(!stars.enabled||reserved_play_)&&index>=0&&index<int(hand.size())&&index!=live.locked&&!Definition(hand[index]).unplayable&&(reserved_play_||energy+live.mana>=Definition(hand[index]).cost);
}

bool Combat::Play(int index){
    if(!CanPlay(index)) return false;
    const auto id=hand[index];
    const auto card=StarCardView(Definition(id));
    hand.erase(hand.begin()+index);
    if(live.locked>index)--live.locked;
    if(!reserved_play_){const int normal=std::min(energy,card.cost);energy-=normal;live.mana-=card.cost-normal;}
    ++live.played_turn;live.card_bonus=0;live.card_multiplier=1;
    if(card.damage>0){
        ++live.attacks_turn;++live.attack_count;
        if(!live.first_attack_used){live.card_bonus+=mods_.first_attack;live.first_attack_used=true;}
        if(live.attacks_turn==3)live.card_bonus+=mods_.third_attack;
        if(card.id=="punch"||card.id=="punch+")live.card_bonus+=mods_.spear_bonus;
        if(live.double_attack>0&&!live.double_used){live.card_multiplier=live.double_attack;live.double_used=true;}
    }
    events.push_back({EventType::CardPlayed});
    const int alive_before=int(Targets().size());
    bool dealt=false,blocked=false;
    for(const auto& effect:card.effects){
        const int times=std::max(1,effect.times);
        for(int n=0;n<times;++n){
            if(phase==Phase::Lost||phase==Phase::Won)break;
            auto applied=effect;applied.ranged|=card.ranged;
            if(applied.op==EffectOp::Apply&&applied.status==StatusId::Poison&&applied.target!=EffectTarget::Self)applied.amount+=mods_.poison_bonus;
            ApplyOne(applied);
        }
        if(effect.op==EffectOp::Damage) dealt=true;
        if(effect.op==EffectOp::Block) blocked=true;
    }
    if(card.exhaust||card.power)exhaust.push_back(id);
    else if(card.return_on_kill&&int(Targets().size())<alive_before&&phase!=Phase::Won)hand.push_back(id);
    else discard.push_back(id);
    live.history.push_back(id);if(live.history.size()>3)live.history.erase(live.history.begin());
    if(dealt) rage=std::min(config.balance.rage_max,rage+config.balance.rage_attack);
    if(blocked) rage=std::min(config.balance.rage_max,rage+config.balance.rage_block);
    if(phase!=Phase::Lost) CheckEnd();
    return true;
}

bool Combat::EndPlayer(){
    if(stars.enabled)return false;
    if(phase!=Phase::Player) return false;
    if(mods_.mana)live.mana=std::min(mods_.max_mana,live.mana+energy);
    player.status.strength=std::max(0,player.status.strength-live.temp_strength);live.temp_strength=0;
    player.block+=live.end_block;
    live.last_turn.clear();const int count=std::min(live.played_turn,int(live.history.size()));
    live.last_turn.insert(live.last_turn.end(),live.history.end()-count,live.history.end());
    discard.insert(discard.end(),hand.begin(),hand.end());
    hand.clear();
    phase=Phase::Enemy;
    events.push_back({EventType::TurnEnd});
    return true;
}

bool Combat::ActEnemy(){
    if(phase!=Phase::Enemy) return false;
    if(live.enemy_retain>0)--live.enemy_retain;else enemy.block=0;
    live.enemy_thorns=0;
    const int player_weak=player.status.weak,player_vulnerable=player.status.vulnerable;
    const int enemy_weak=enemy.status.weak,enemy_vulnerable=enemy.status.vulnerable;
    bool phased=false;
    TickPoison(true,phased);
    if(Targets().empty()){
        events.push_back({EventType::TurnEnd,true});
        phase=Phase::CheckEnd;
        CheckEnd();
        return true;
    }
    if(!phased&&enemy.hp>0)ResolveIntent();
    if(enemy.hp>0)enemy.block+=live.enemy_end_block;
    for(int i=0;i<int(adds.size())&&phase!=Phase::Lost;++i)ResolveAdd(i);
    if(live.constrict_left>0&&player.hp>0){--live.constrict_left;const auto result=ResolveDamage(player,4,true);live.damage_taken+=result.lost;events.push_back({EventType::Damage,false,result.lost});}
    if(live.doom_triggered&&player.hp>0)player.hp=0;
    if(live.doom_left>0&&player.hp>0){--live.doom_left;if(live.doom_left==0){live.doom_triggered=true;player.hp=0;CheckEnd();}}
    if(definition_.airborne&&!live.airborne&&live.land_turns>0&&--live.land_turns==0){live.airborne=true;live.flight_damage=0;}
    auto keep=[&](StatusSet& status,int old_weak,int old_vulnerable){
        if(old_weak>0) status.weak=std::max(0,status.weak-1);
        if(old_vulnerable>0) status.vulnerable=std::max(0,status.vulnerable-1);
    };
    // Debuffs already on a fighter tick after the enemy action. Stacks applied by this action stay.
    if(enemy.hp>0){keep(player.status,player_weak,player_vulnerable); keep(enemy.status,enemy_weak,enemy_vulnerable);}
    if(Targets().empty()){
        events.push_back({EventType::TurnEnd,true});
        phase=Phase::CheckEnd;
        CheckEnd();
        return true;
    }
    events.push_back({EventType::TurnEnd,true});
    phase=Phase::CheckEnd;
    CheckEnd();
    if(phase==Phase::CheckEnd){
        if(!phased) RollIntent();
        BeginPlayer();
    }
    return true;
}

bool Combat::StartUltimate(){
    if(!CanUltimate())return false;
    rage-=UltimateCost();
    phase=Phase::Cinematic;
    ultimate_pending_=true;
    events.push_back({EventType::Ultimate});
    return true;
}

bool Combat::UltimateImpact(){
    if(!ultimate_pending_) return false;
    ultimate_pending_=false;
    ++live.ultimates;
    if(mods_.ultimate=="shadow"){
        const auto cards=live.history;live.card_bonus=0;live.card_multiplier=1;
        for(const auto& id:cards){const auto card=StarCardView(Definition(id));for(const auto& effect:card.effects){auto replay=effect;replay.ranged|=card.ranged;int times=effect.times;
            if(effect.op==EffectOp::Damage){replay.amount+=mods_.ultimate_bonus+mods_.bonus_damage;if(mods_.comet){replay.amount=replay.amount*60/100;times*=2;}}
            for(int n=0;n<times&&phase!=Phase::Won&&phase!=Phase::Lost;++n)ApplyOne(replay,effect.op!=EffectOp::Damage);
        }}
    }else{
        const int hits=std::max(1,mods_.ultimate_hits)*(mods_.comet?2:1);
        std::vector<int> struck;
        for(int hit=0;hit<hits&&!Targets().empty()&&phase!=Phase::Lost;++hit){
            const auto targets=Targets();if(mods_.ultimate=="storm")live.target=targets[std::min(int(targets.size())-1,int(target_rng_.Next()*targets.size()))];
            else if(Target(live.target).hp<=0)live.target=targets.front();
            int amount=(mods_.ultimate_damage>0?mods_.ultimate_damage:config.balance.ultimate_damage)+mods_.ultimate_bonus+mods_.bonus_damage;if(mods_.comet)amount=amount*60/100;
            const int target=live.target;HitTarget(target,ScaleDamage(amount,player.status,Target(target).status),false,true);
            if(std::find(struck.begin(),struck.end(),target)==struck.end())struck.push_back(target);
        }
        if(mods_.ultimate_vulnerable>0)for(int target:struck)if(Target(target).hp>0)AddStatusStack(Target(target),StatusId::Vulnerable,mods_.ultimate_vulnerable);
    }
    CheckEnd();
    return true;
}

void Combat::FinishUltimate(){
    if(phase!=Phase::Cinematic) return;
    UltimateImpact();
    if(phase==Phase::Cinematic) phase=Phase::Player;
}

const std::string& Combat::PhaseName() const {return CurrentPhase().name;}

int Combat::IncomingDamage() const {
    int damage=0;
    if(intent.kind==IntentKind::Mimic&&!intent.card_id.empty()){for(const auto& effect:Definition(intent.card_id).effects)if(effect.op==EffectOp::Damage)damage+=ScaleDamage(effect.amount*std::max(1,live.enemy_double),enemy.status,player.status)*effect.times;}
    else if(intent.attack||intent.kind==IntentKind::Drain)damage=ScaleDamage(intent.value*std::max(1,live.enemy_double),enemy.status,player.status)*std::max(1,intent.hits);
    for(const auto& add:adds)if(add.fighter.hp>0&&add.intent.attack)damage+=ScaleDamage(add.intent.value,add.fighter.status,player.status)*std::max(1,add.intent.hits);
    if(live.constrict_left>0)damage+=4;
    return damage;
}

std::string Combat::IntentText() const {
    if(intent.kind==IntentKind::Charge)return "Tích tụ: "+std::to_string(ScaleDamage(intent.value*std::max(1,live.enemy_double),enemy.status,player.status))+" sát thương sau "+std::to_string(live.charge_left)+" lượt";
    if(intent.kind==IntentKind::Summon)return "Triệu hồi "+std::to_string(intent.hits)+" viện binh";
    if(intent.kind==IntentKind::Curse)return "Xáo lá: biến một lá thành Bùn Lầy";
    if(intent.kind==IntentKind::Mimic&&!intent.card_id.empty())return "Sao chép: "+Definition(intent.card_id).name;
    if(intent.kind==IntentKind::Drain)return "Hút "+std::to_string(intent.status_amount)+" Nộ · Đánh "+std::to_string(intent.value);
    if(intent.attack){
        std::string text="Sắp tấn công: "+std::to_string(IncomingDamage());
        if(intent.hits>1)text="Sắp tấn công: "+std::to_string(ScaleDamage(intent.value*std::max(1,live.enemy_double),enemy.status,player.status))+" ×"+std::to_string(intent.hits);
        if(intent.status_amount>0) text+=" · "+std::to_string(intent.status_amount)+" "+StatusName(intent.status);
        return text;
    }
    if(intent.kind==IntentKind::Buff){
        std::string text;
        if(intent.status_amount>0) text="Sắp nhận "+std::to_string(intent.status_amount)+" "+StatusName(intent.status);
        if(intent.value>0){
            if(!text.empty()) text+=" · ";
            else text="Sắp nhận ";
            text+=std::to_string(intent.value)+" giáp";
        }
        return text.empty()?"Sắp tăng sức":text;
    }
    if(intent.kind==IntentKind::Debuff) return "Sắp gây "+std::to_string(intent.status_amount)+" "+StatusName(intent.status);
    return "Sắp nhận "+std::to_string(intent.value)+" giáp";
}

std::string Combat::StatusText(const Fighter& fighter) const {
    std::string text="Giáp: "+std::to_string(fighter.block);
    auto add=[&](int amount,const char* name){if(amount>0) text+=std::string(" · ")+name+" "+std::to_string(amount);};
    add(fighter.status.strength,"Sức Mạnh");
    add(fighter.status.poison,"Độc");
    add(fighter.status.weak,"Suy Yếu");
    add(fighter.status.vulnerable,"Dễ Vỡ");
    return text;
}

CombatSnapshot Combat::Snapshot()const{
    CombatSnapshot s;
    s.enemy_id=enemy_id_;s.player=player;s.enemy=enemy;s.phase=phase;s.intent=intent;
    s.energy=energy;s.rage=rage;s.turn=turn;s.guard_streak=guard_streak_;s.phase_index=phase_index_;
    s.shuffle_rng=shuffle_rng_.State();s.intent_rng=intent_rng_.State();s.phase_opening=phase_opening_;
    s.draw=draw;s.hand=hand;s.discard=discard;
    s.target_rng=target_rng_.State();s.live=live;s.adds=adds;s.exhaust=exhaust;s.created=created;s.stars=stars;
    return s;
}

void Combat::Restore(const CombatSnapshot& s,CombatMods mods){
    const EnemyDef* found=nullptr;
    for(const auto& definition:config.bestiary)if(definition.id==s.enemy_id)found=&definition;
    if(!found)throw std::runtime_error("Unknown saved enemy");
    const auto scaled=Scaled(*found,mods);std::vector<PhaseDef> phases={BasePhase(scaled)};
    phases.insert(phases.end(),scaled.phases.begin(),scaled.phases.end());
    auto valid_fighter=[](const Fighter& f){return f.max_hp>0&&f.max_hp<=100000&&f.hp>=0&&f.hp<=f.max_hp&&f.block>=0&&f.block<=100000&&f.status.poison>=0&&f.status.poison<=10000&&f.status.weak>=0&&f.status.weak<=10000&&f.status.vulnerable>=0&&f.status.vulnerable<=10000&&f.status.strength>=0&&f.status.strength<=10000;};
    const bool stable=s.phase==Phase::Player||s.phase==Phase::Won||s.phase==Phase::Lost;
    if(!stable||!valid_fighter(s.player)||!valid_fighter(s.enemy)||s.phase_index<0||s.phase_index>=int(phases.size())||s.energy<0||s.energy>100||s.rage<0||s.rage>config.balance.rage_max||s.turn<1||s.turn>100000||s.guard_streak<0||s.guard_streak>100000)throw std::runtime_error("Invalid saved combat");
    bool living=s.enemy.hp>0;for(const auto& a:s.adds)living|=a.fighter.hp>0;
    if((s.phase==Phase::Player&&(s.player.hp==0||!living))||(s.phase==Phase::Won&&living)||(s.phase==Phase::Lost&&s.player.hp!=0))throw std::runtime_error("Invalid saved combat result");
    if(s.enemy.max_hp!=phases[s.phase_index].hp||s.intent.value<0||s.intent.value>100000||s.intent.status_amount<0||s.intent.status_amount>10000||int(s.intent.kind)<0||int(s.intent.kind)>int(IntentKind::Drain)||int(s.intent.status)<0||int(s.intent.status)>int(StatusId::Strength)||s.intent.hits<1||s.intent.hits>8)throw std::runtime_error("Invalid saved intent");
    if(s.adds.size()>4||s.live.target<0||s.live.target>int(s.adds.size())||s.live.mana<0||s.live.mana>10||s.live.locked<-1||s.live.locked>=int(s.hand.size())||s.live.revives<0||s.live.revives>1)throw std::runtime_error("Invalid saved battle runtime");
    for(int value:{s.live.retain_turns,s.live.temp_strength,s.live.next_energy,s.live.thorns,s.live.double_attack,s.live.end_block,s.live.ult_discount,s.live.flight_damage,s.live.land_turns,s.live.charge_left,s.live.constrict_left,s.live.doom_left,s.live.played_turn,s.live.damage_dealt,s.live.damage_taken,s.live.poison_applied,s.live.ultimates,s.live.attacks_turn,s.live.attack_count,s.live.enemy_thorns,s.live.enemy_end_block,s.live.enemy_double,s.live.enemy_retain,s.live.enemy_revive})if(value<0||value>10000000)throw std::runtime_error("Invalid saved counter");
    if(s.live.history.size()>3||s.live.last_turn.size()>3)throw std::runtime_error("Invalid card history");
    for(const auto* history:{&s.live.history,&s.live.last_turn})for(const auto& id:*history)Definition(id);
    auto valid_intent=[&](const Intent& action,const std::string& owner){
        if(int(action.kind)<0||int(action.kind)>int(IntentKind::Drain)||int(action.status)<0||int(action.status)>int(StatusId::Strength)||action.value<0||action.value>100000||action.status_amount<0||action.status_amount>10000||action.hits<1||action.hits>8)return false;
        if(!action.card_id.empty())Definition(action.card_id);
        if(!action.summon_id.empty()){bool known=false;for(const auto& e:config.bestiary)known|=e.id==action.summon_id;if(!known)return false;}
        if(!action.deck_card_id.empty()){
            const auto* card=BossCardIdentity(owner,action);
            if(!card||card->action.kind!=action.kind)return false;
        }
        return true;
    };
    if(!valid_intent(s.intent,s.enemy_id))throw std::runtime_error("Invalid saved intent reference");
    if(s.stars.resolving||s.stars.cursor<0||s.stars.cursor>6||s.stars.player.size()>3||s.stars.enemy.size()>3)throw std::runtime_error("Invalid Turn Star plan");
    if(!s.stars.enabled&&(!s.stars.player.empty()||!s.stars.enemy.empty()))throw std::runtime_error("Disabled Turn Star plan has cards");
    if(s.stars.enabled&&s.phase==Phase::Player&&(s.stars.enemy.size()!=3||s.stars.cursor!=0))throw std::runtime_error("Incomplete Turn Star preview");
    std::vector<int> queued;
    for(const auto& q:s.stars.player){
        if(q.target<0||q.target>int(s.adds.size())||q.hand_index< -1||q.hand_index>=int(s.hand.size()))throw std::runtime_error("Invalid queued player card");
        if(!q.id.empty())Definition(q.id);
        if(q.hand_index>=0){if(q.id!=s.hand[q.hand_index]||q.hand_index==s.live.locked||Definition(s.hand[q.hand_index]).unplayable||std::find(queued.begin(),queued.end(),q.hand_index)!=queued.end())throw std::runtime_error("Invalid duplicate or locked queued card");queued.push_back(q.hand_index);}
        else if(s.phase==Phase::Player&&!q.id.empty())throw std::runtime_error("Queued card has no hand reservation");
    }
    for(const auto& q:s.stars.enemy)if(q.actor<0||q.actor>int(s.adds.size())||!valid_intent(q.action,q.actor==0?s.enemy_id:s.adds[q.actor-1].id))throw std::runtime_error("Invalid queued enemy card");
    if(!s.live.slots.empty()){auto slots=s.live.slots;std::sort(slots.begin(),slots.end());if(slots.size()!=s.adds.size()+1)throw std::runtime_error("Invalid target slots");for(int i=0;i<int(slots.size());++i)if(slots[i]!=i)throw std::runtime_error("Invalid target permutation");}
    for(const auto& a:s.adds){if(!valid_fighter(a.fighter)||!valid_intent(a.intent,a.id))throw std::runtime_error("Invalid saved summon");bool known=false;for(const auto& e:config.bestiary)known|=e.id==a.id;if(!known)throw std::runtime_error("Unknown saved summon");}
    for(const auto* pile:{&s.draw,&s.hand,&s.discard,&s.exhaust,&s.created}){if(pile->size()>1000)throw std::runtime_error("Saved deck too large");for(const auto& id:*pile)Definition(id);}
    phases_=std::move(phases);enemy_id_=s.enemy_id;role_=found->role;mods_=std::move(mods);
    player=s.player;enemy=s.enemy;phase=s.phase;intent=s.intent;energy=s.energy;rage=s.rage;turn=s.turn;
    guard_streak_=s.guard_streak;phase_index_=s.phase_index;phase_opening_=s.phase_opening;ultimate_pending_=false;
    definition_=scaled;live=s.live;stars=s.stars;reserved_play_=false;adds=s.adds;exhaust=s.exhaust;created=s.created;target_rng_.Seed(s.target_rng);
    shuffle_rng_.Seed(s.shuffle_rng);intent_rng_.Seed(s.intent_rng);draw=s.draw;hand=s.hand;discard=s.discard;events.clear();
    if(stars.enabled)BindBossCardIdentities();
}
}
