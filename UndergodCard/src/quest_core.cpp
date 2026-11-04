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
}

Combat::Combat(Config c):config(std::move(c)){Reset();}

void Combat::Shuffle(std::vector<std::string>& pile){
    for(int i=int(pile.size())-1;i>0;--i) std::swap(pile[i],pile[int(shuffle_rng_.Next()*(i+1))]);
}

const PhaseDef& Combat::CurrentPhase() const {return phases_.at(phase_index_);}

void Combat::RollIntent(){
    const auto& current=CurrentPhase();
    const bool opening=phase_opening_||(turn==0&&current.opening_attack);
    phase_opening_=false;
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
    for(const auto& pattern:current.patterns) if(!force_attack||pattern.kind==IntentKind::Attack) pool.push_back(&pattern);
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
    intent.attack=picked->kind==IntentKind::Attack;
    if(intent.attack){
        const int span=std::max(1,picked->max_damage-picked->min_damage+1);
        intent.value=picked->min_damage+int(intent_rng_.Next()*span);
        guard_streak_=0;
    }else{
        intent.value=std::max(0,picked->block);
        ++guard_streak_;
    }
}

void Combat::BeginPlayer(){
    ++turn;
    phase=Phase::Player;
    player.block=0;
    bool phased=false;
    TickPoison(false,phased);
    if(player.hp<=0){phase=Phase::Lost;return;}
    energy=config.balance.energy;
    Draw(config.balance.draw);
}

void Combat::CheckEnd(){
    if(player.hp<=0) phase=Phase::Lost;
    else if(enemy.hp<=0) phase=Phase::Won;
}

void Combat::AdvancePhaseIfAny(){
    if(enemy.hp>0||phase_index_+1>=int(phases_.size())) return;
    ++phase_index_;
    const auto& next=phases_[phase_index_];
    enemy={next.hp,next.hp,0,{}};
    enemy.status.strength=std::max(0,next.strength);
    guard_streak_=0;
    phase_opening_=next.opening_attack;
    events.push_back({EventType::Phase,true,phase_index_+1});
    RollIntent();
}

void Combat::Hit(bool target_enemy,int damage,bool pierce){
    auto& target=target_enemy?enemy:player;
    const auto result=ResolveDamage(target,damage,pierce);
    events.push_back({EventType::Damage,target_enemy,result.lost,result.blocked});
    if(target_enemy) AdvancePhaseIfAny();
}

void Combat::AddStatusEvent(Fighter& fighter,bool enemy_target,StatusId id,int amount){
    if(amount==0) return;
    AddStatusStack(fighter,id,amount);
    events.push_back({EventType::Status,enemy_target,amount,0,id});
}

bool Combat::TickPoison(bool enemy_target,bool& phased){
    phased=false;
    auto& fighter=enemy_target?enemy:player;
    if(fighter.status.poison<=0) return false;
    const int damage=fighter.status.poison;
    fighter.status.poison=std::max(0,fighter.status.poison-1);
    const auto result=ResolveDamage(fighter,damage,true);
    events.push_back({EventType::Damage,enemy_target,result.lost,0});
    if(enemy_target&&fighter.hp<=0){
        const int before=phase_index_;
        AdvancePhaseIfAny();
        phased=phase_index_!=before;
    }
    return true;
}

void Combat::ApplyOne(const Effect& effect){
    auto& target=effect.target==EffectTarget::Enemy?enemy:player;
    const bool enemy_target=effect.target==EffectTarget::Enemy;
    switch(effect.op){
    case EffectOp::Damage:{
        const int damage=ScaleDamage(effect.amount,player.status,target.status);
        Hit(enemy_target,damage,false);
        break;
    }
    case EffectOp::Block:
        target.block+=effect.amount;
        events.push_back({EventType::Block,enemy_target,effect.amount});
        break;
    case EffectOp::Heal:{
        const int amount=std::min(effect.amount,target.max_hp-target.hp);
        target.hp+=amount;
        events.push_back({EventType::Heal,enemy_target,amount});
        break;
    }
    case EffectOp::Apply:
        AddStatusEvent(target,enemy_target,effect.status,effect.amount);
        break;
    case EffectOp::Draw:
        Draw(effect.amount);
        break;
    case EffectOp::Energy:
        energy=std::min(10,energy+effect.amount);
        break;
    }
}

void Combat::ResolveIntent(){
    if(intent.attack){
        Hit(false,ScaleDamage(intent.value,enemy.status,player.status),false);
        if(player.hp>0&&intent.status_amount>0) AddStatusEvent(player,false,intent.status,intent.status_amount);
        return;
    }
    if(intent.kind==IntentKind::Buff){
        if(intent.value>0){enemy.block+=intent.value; events.push_back({EventType::Block,true,intent.value});}
        if(intent.status_amount>0) AddStatusEvent(enemy,true,intent.status,intent.status_amount);
        return;
    }
    if(intent.kind==IntentKind::Debuff){
        if(intent.status_amount>0&&player.hp>0) AddStatusEvent(player,false,intent.status,intent.status_amount);
        if(intent.value>0) Hit(false,ScaleDamage(intent.value,enemy.status,player.status),false);
        return;
    }
    enemy.block=intent.value;
    events.push_back({EventType::Block,true,intent.value});
}

void Combat::BeginEncounter(const EnemyDef& enemy_def,uint32_t seed,int hp,int max_hp,int kept_rage,const std::vector<std::string>& deck){
    shuffle_rng_.Seed(seed);
    intent_rng_.Seed(seed^0x19ABCu);
    ultimate_pending_=false;
    phase_opening_=false;
    guard_streak_=0;
    phase_index_=0;
    role_=enemy_def.role.empty()?"combat":enemy_def.role;
    phases_.clear();
    phases_.push_back(BasePhase(enemy_def));
    for(const auto& extra:enemy_def.phases) phases_.push_back(extra);
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
        if(draw.empty()){draw=std::move(discard); discard.clear(); Shuffle(draw);}
        if(draw.empty()) break;
        hand.push_back(draw.back());
        draw.pop_back();
    }
}

bool Combat::CanPlay(int index) const {
    return phase==Phase::Player && index>=0 && index<int(hand.size()) && energy>=Definition(hand[index]).cost;
}

bool Combat::Play(int index){
    if(!CanPlay(index)) return false;
    const auto id=hand[index];
    const auto& card=Definition(id);
    hand.erase(hand.begin()+index);
    discard.push_back(id);
    energy-=card.cost;
    events.push_back({EventType::CardPlayed});
    bool dealt=false,blocked=false;
    for(const auto& effect:card.effects){
        const int times=std::max(1,effect.times);
        for(int n=0;n<times;++n){
            if(player.hp<=0) break;
            ApplyOne(effect);
            if(phase==Phase::Lost) break;
        }
        if(effect.op==EffectOp::Damage) dealt=true;
        if(effect.op==EffectOp::Block) blocked=true;
    }
    if(dealt) rage=std::min(config.balance.rage_max,rage+config.balance.rage_attack);
    if(blocked) rage=std::min(config.balance.rage_max,rage+config.balance.rage_block);
    if(phase!=Phase::Lost) CheckEnd();
    return true;
}

bool Combat::EndPlayer(){
    if(phase!=Phase::Player) return false;
    discard.insert(discard.end(),hand.begin(),hand.end());
    hand.clear();
    phase=Phase::Enemy;
    events.push_back({EventType::TurnEnd});
    return true;
}

bool Combat::ActEnemy(){
    if(phase!=Phase::Enemy) return false;
    enemy.block=0;
    const int player_weak=player.status.weak,player_vulnerable=player.status.vulnerable;
    const int enemy_weak=enemy.status.weak,enemy_vulnerable=enemy.status.vulnerable;
    bool phased=false;
    TickPoison(true,phased);
    if(enemy.hp<=0){
        events.push_back({EventType::TurnEnd,true});
        phase=Phase::CheckEnd;
        CheckEnd();
        return true;
    }
    if(!phased) ResolveIntent();
    auto keep=[&](StatusSet& status,int old_weak,int old_vulnerable){
        if(old_weak>0) status.weak=std::max(0,status.weak-1);
        if(old_vulnerable>0) status.vulnerable=std::max(0,status.vulnerable-1);
    };
    // Debuffs already on a fighter tick after the enemy action. Stacks applied by this action stay.
    if(enemy.hp>0){keep(player.status,player_weak,player_vulnerable); keep(enemy.status,enemy_weak,enemy_vulnerable);}
    if(enemy.hp<=0){
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
    if(phase!=Phase::Player||rage<config.balance.rage_max) return false;
    rage=0;
    phase=Phase::Cinematic;
    ultimate_pending_=true;
    events.push_back({EventType::Ultimate});
    return true;
}

bool Combat::UltimateImpact(){
    if(!ultimate_pending_) return false;
    ultimate_pending_=false;
    Hit(true,ScaleDamage(config.balance.ultimate_damage,player.status,enemy.status),false);
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
    if(!intent.attack) return 0;
    return ScaleDamage(intent.value,enemy.status,player.status);
}

std::string Combat::IntentText() const {
    if(intent.attack){
        std::string text="Sắp tấn công: "+std::to_string(IncomingDamage());
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
    add(fighter.status.vulnerable,"Dễ Thương");
    return text;
}
}
