#include "quest_core.hpp"
#include <set>

namespace undergod {
bool Combat::HasBossDeck()const{
    if(role_!="boss")return false;
    for(const auto& card:config.boss_cards)if(card.boss_id==enemy_id_)return true;
    return false;
}
const BossCardDef* Combat::BossCardIdentity(const std::string& owner,const Intent& action)const{
    const BossCardDef* fallback=nullptr;
    for(const auto& card:config.boss_cards)if(card.boss_id==owner){
        if(!action.deck_card_id.empty()){if(card.id==action.deck_card_id)return &card;}
        else if(card.action.kind==action.kind){
            if(phase_index_>=card.min_phase&&phase_index_<=card.max_phase)return &card;
            fallback=&card;
        }
    }
    return action.deck_card_id.empty()?fallback:nullptr;
}
void Combat::BindBossCardIdentities(){
    auto bind=[&](const std::string& owner,Intent& action){if(action.deck_card_id.empty())if(const auto* card=BossCardIdentity(owner,action))action.deck_card_id=card->id;};
    bind(enemy_id_,intent);
    for(auto& q:stars.enemy)bind(q.actor==0?enemy_id_:adds.at(q.actor-1).id,q.action);
}
Intent Combat::DrawBossIntent(bool force_attack,bool allow_charge,const std::vector<std::string>& used,bool force_charge){
    std::vector<const BossCardDef*> pool;
    auto gather=[&](bool exclude_used){
        for(const auto& card:config.boss_cards){
            const auto kind=card.action.kind;
            const bool attack=kind==IntentKind::Attack||kind==IntentKind::MultiAttack||kind==IntentKind::Mimic||kind==IntentKind::Drain||kind==IntentKind::Charge;
            if(card.boss_id!=enemy_id_||phase_index_<card.min_phase||phase_index_>card.max_phase||(!allow_charge&&kind==IntentKind::Charge)||(force_charge&&kind!=IntentKind::Charge)||(force_attack&&!attack))continue;
            if(exclude_used&&std::find(used.begin(),used.end(),card.id)!=used.end())continue;
            pool.push_back(&card);
        }
    };
    gather(true);if(pool.empty())gather(false);
    if(pool.empty())throw std::runtime_error("Boss has no eligible card: "+enemy_id_);
    int total=0;for(const auto* card:pool)total+=card->action.weight;
    const double roll=intent_rng_.Next()*total;int accumulated=0;const auto* chosen=pool.back();
    for(const auto* card:pool){accumulated+=card->action.weight;if(roll<accumulated){chosen=card;break;}}
    const auto& pattern=chosen->action;Intent action;
    action.deck_card_id=chosen->id;action.kind=pattern.kind;action.hits=pattern.hits;action.status=pattern.status;action.status_amount=pattern.status_amount;action.summon_id=pattern.summon_id;
    action.attack=pattern.kind==IntentKind::Attack||pattern.kind==IntentKind::MultiAttack||pattern.kind==IntentKind::Mimic||pattern.kind==IntentKind::Drain;
    if(action.attack||pattern.kind==IntentKind::Charge)action.value=pattern.min_damage+mods_.enemy_damage+int(intent_rng_.Next()*(pattern.max_damage-pattern.min_damage+1));
    else action.value=pattern.block;
    if(action.kind==IntentKind::Mimic&&!live.last_turn.empty())action.card_id=live.last_turn.back();
    return action;
}
void Combat::EnableTurnStars(){
    if(stars.enabled)return;
    stars={};stars.enabled=true;live.mana=0;
    if(turn==1){
        int extra=mods_.energy*3+(mods_.mana?std::max(0,mods_.max_mana-3):0);
        for(const auto* list:{&mods_.combat_start,&mods_.turn_start})for(const auto& e:*list)if(e.op==EffectOp::Energy)extra+=e.amount*std::max(1,e.times);
        if(extra>=0)Draw(extra);
        else while(extra++<0&&hand.size()>1){draw.push_back(hand.back());hand.pop_back();}
    }
    while(hand.size()>10){draw.push_back(hand.back());hand.pop_back();}
    if(live.locked>=int(hand.size()))live.locked=-1;
    if(phase==Phase::Player){
        if(HasBossDeck()){
            if(live.charge_left>0)BindBossCardIdentities();
            else{phase_opening_|=turn==1;RollIntent();}
        }
        BuildEnemyStars();
    }
}
bool Combat::IsQueuedStar(int index)const{
    for(const auto& q:stars.player)if(q.hand_index==index&&index>=0)return true;
    return false;
}
bool Combat::CanQueueStar(int index)const{
    return stars.enabled&&!stars.resolving&&phase==Phase::Player&&stars.player.size()<TurnStars&&index>=0&&index<int(hand.size())&&index!=live.locked&&!Definition(hand[index]).unplayable&&!IsQueuedStar(index);
}
bool Combat::QueueStar(int index){
    if(!CanQueueStar(index))return false;
    stars.player.push_back({index,live.target,hand[index]});return true;
}
bool Combat::QueueWait(){
    if(!stars.enabled||stars.resolving||phase!=Phase::Player||stars.player.size()>=TurnStars)return false;
    stars.player.push_back({-1,live.target,{}});return true;
}
bool Combat::UnqueueStar(int slot){
    if(!stars.enabled||stars.resolving||phase!=Phase::Player||slot<0||slot>=int(stars.player.size()))return false;
    stars.player.erase(stars.player.begin()+slot);return true;
}
bool Combat::CanAttackStars()const{
    if(!stars.enabled||stars.resolving||phase!=Phase::Player||stars.player.size()!=TurnStars||stars.enemy.size()!=TurnStars)return false;
    std::set<int> reserved;
    for(const auto& q:stars.player)if(!q.id.empty()){
        if(q.hand_index<0||q.hand_index>=int(hand.size())||q.hand_index==live.locked||hand[q.hand_index]!=q.id||!reserved.insert(q.hand_index).second)return false;
    }
    return true;
}
void Combat::BuildEnemyStars(){
    stars.enemy.clear();if(phase!=Phase::Player)return;
    const auto targets=Targets();if(targets.empty())return;
    std::vector<std::string> used;
    for(int slot=0;slot<TurnStars;++slot){
        const int actor=targets[std::min(slot,int(targets.size())-1)];
        Intent action;
        if(actor==0&&slot==0)action=intent;
        else if(actor==0&&HasBossDeck()){
            if(intent.kind==IntentKind::Charge){
                action.kind=IntentKind::Defend;action.value=3;
                if(const auto* guard=BossCardIdentity(enemy_id_,action))action.deck_card_id=guard->id;
            }else action=DrawBossIntent(false,false,used);
        }else if(actor==0){
            EnemyDef def=definition_;const auto& p=CurrentPhase();
            def.patterns=p.patterns;def.min_damage=p.min_damage;def.max_damage=p.max_damage;def.block=p.block;def.attack_chance=p.attack_chance;
            action=PickIntent(def);
        }else{
            const auto& add=adds.at(actor-1);action=add.intent;
            bool already=false;for(const auto& q:stars.enemy)already|=q.actor==actor;
            if(already)for(const auto& def:config.bestiary)if(def.id==add.id){action=PickIntent(def);break;}
        }
        // Chỉ một nhịp tích tụ mỗi vòng; hai lá còn lại vẫn cho boss dựng giáp.
        if(action.kind==IntentKind::Charge&&slot>0){action={};action.kind=IntentKind::Defend;action.value=3;}
        if(action.kind!=IntentKind::Charge){
            if(action.attack||action.kind==IntentKind::Drain||action.kind==IntentKind::Mimic)action.value=std::max(1,(action.value*35+99)/100);
            if(action.status_amount>0)action.status_amount=std::max(1,(action.status_amount+1)/2);
        }
        if(action.kind==IntentKind::Mimic&&!live.last_turn.empty())action.card_id=live.last_turn.back();
        if(!action.deck_card_id.empty())used.push_back(action.deck_card_id);
        stars.enemy.push_back({actor,action,false});
    }
}
bool Combat::BeginStarAttack(){
    if(!CanAttackStars())return false;
    std::vector<std::string> selected;
    std::set<int> indices;
    for(const auto& q:stars.player)if(!q.id.empty()){
        if(q.hand_index<0||q.hand_index>=int(hand.size())||hand[q.hand_index]!=q.id||q.hand_index==live.locked||!indices.insert(q.hand_index).second)return false;
        selected.push_back(q.id);
    }
    for(int i=0;i<int(hand.size());++i)if(!indices.count(i))discard.push_back(hand[i]);
    hand=std::move(selected);live.locked=-1;
    int next=0;for(auto& q:stars.player)q.hand_index=q.id.empty()?-1:next++;
    stars.resolving=true;stars.cursor=0;star_phase_start_=phase_index_;star_curses_=0;
    star_enemy_started_.assign(adds.size()+1,false);
    star_player_decay_=player.status;star_enemy_decay_=enemy.status;
    star_add_decay_.clear();for(const auto& add:adds)star_add_decay_.push_back(add.fighter.status);
    return true;
}
bool Combat::ResolveStarAction(){
    if(!stars.resolving||phase!=Phase::Player||stars.cursor>=TurnStars*2)return false;
    const int cursor=stars.cursor++,slot=cursor/2;
    if(cursor%2==0){
        auto& q=stars.player.at(slot);
        if(q.hand_index>=0){
            const int at=q.hand_index;
            if(q.target>=0&&q.target<=int(adds.size())&&Target(q.target).hp>0)live.target=q.target;
            else{const auto alive=Targets();if(!alive.empty())live.target=alive.front();}
            reserved_play_=true;const bool played=Play(at);reserved_play_=false;
            if(!played)throw std::runtime_error("Reserved Turn Star card became invalid");
            q.hand_index=-1;for(auto& later:stars.player)if(later.hand_index>at)--later.hand_index;
        }
        if(slot==TurnStars-1){
            player.block+=live.end_block;
            player.status.strength=std::max(0,player.status.strength-live.temp_strength);live.temp_strength=0;
        }
    }else{
        const auto q=stars.enemy.at(slot);
        if(!q.wait&&q.actor<=int(adds.size())&&Target(q.actor).hp>0){
            const int phase_before=phase_index_;
            if(q.actor>=int(star_enemy_started_.size()))star_enemy_started_.resize(q.actor+1,false);
            if(!star_enemy_started_[q.actor]){
                star_enemy_started_[q.actor]=true;
                if(q.actor==0){
                    if(live.enemy_retain>0)--live.enemy_retain;else enemy.block=0;
                    live.enemy_thorns=0;bool phased=false;TickPoison(true,phased);
                }else{
                    auto& f=Target(q.actor);f.block=0;
                    if(f.status.poison>0){const int amount=f.status.poison--;const auto hit=ResolveDamage(f,amount,true);live.damage_dealt+=hit.lost;events.push_back({EventType::Damage,true,hit.lost,0,StatusId::Poison,q.actor});CheckEnd();}
                }
            }
            if(phase==Phase::Player&&Target(q.actor).hp>0&&phase_before==phase_index_){
                attacker_=q.actor;
                if(q.action.kind==IntentKind::Curse)++star_curses_;
                else if(q.actor==0){
                    const auto held=intent;intent=q.action;ResolveIntent();
                    if(phase_before==phase_index_)intent=held;
                }else{
                    auto& f=Target(q.actor);const auto& a=q.action;
                    if(a.attack)for(int hit=0;hit<std::max(1,a.hits)&&phase==Phase::Player&&f.hp>0;++hit)Hit(false,ScaleDamage(a.value,f.status,player.status));
                    else if(a.kind==IntentKind::Defend)f.block+=a.value;
                    else if(a.kind==IntentKind::Buff){f.block+=a.value;if(a.status_amount>0)AddStatusStack(f,a.status,a.status_amount);}
                    else if(a.kind==IntentKind::Debuff)AddStatusEvent(player,false,a.status,a.status_amount);
                    if(a.attack&&a.status_amount>0&&player.hp>0)AddStatusEvent(player,false,a.status,a.status_amount);
                }
            }
        }
    }
    CheckEnd();
    if(phase==Phase::Won||phase==Phase::Lost)stars.resolving=false;
    return true;
}
void Combat::FinishStarRound(){
    if(!stars.resolving||stars.cursor!=TurnStars*2||phase!=Phase::Player)return;
    if(live.constrict_left>0){--live.constrict_left;const auto hit=ResolveDamage(player,4,true);live.damage_taken+=hit.lost;events.push_back({EventType::Damage,false,hit.lost,hit.blocked});}
    if(live.doom_left>0&&--live.doom_left==0){live.doom_triggered=true;player.hp=0;}
    CheckEnd();
    if(phase==Phase::Won||phase==Phase::Lost){stars.resolving=false;return;}
    auto decay=[](StatusSet& now,const StatusSet& before){if(before.weak>0)now.weak=std::max(0,now.weak-1);if(before.vulnerable>0)now.vulnerable=std::max(0,now.vulnerable-1);};
    decay(player.status,star_player_decay_);decay(enemy.status,star_enemy_decay_);
    for(int i=0;i<int(adds.size())&&i<int(star_add_decay_.size());++i)decay(adds[i].fighter.status,star_add_decay_[i]);
    if(definition_.airborne&&!live.airborne&&live.land_turns>0&&--live.land_turns==0){live.airborne=true;live.flight_damage=0;}
    enemy.block+=live.enemy_end_block;
    live.last_turn.clear();const int count=std::min(live.played_turn,int(live.history.size()));live.last_turn.insert(live.last_turn.end(),live.history.end()-count,live.history.end());
    if(phase_index_==star_phase_start_)RollIntent();
    for(auto& add:adds)if(add.fighter.hp>0)for(const auto& def:config.bestiary)if(def.id==add.id){add.intent=PickIntent(def);break;}
    stars.resolving=false;stars.cursor=0;stars.player.clear();stars.enemy.clear();
    // Bài rút trong chuỗi hành động được giữ sang vòng kế tiếp, không bị vứt ngay.
    BeginPlayer();
    for(int n=0;n<star_curses_&&!hand.empty()&&phase==Phase::Player;++n){const int at=std::min(int(hand.size())-1,int(target_rng_.Next()*hand.size()));exhaust.push_back(hand[at]);hand[at]="mud";created.push_back("mud");}
    star_curses_=0;
    if(phase==Phase::Player)BuildEnemyStars();
    CheckEnd();
}
CardDef Combat::StarCardView(const CardDef& original,bool force)const{
    auto card=original;if(!stars.enabled&&!force)return card;card.cost=1;
    const bool guard=card.upgraded&&Definition(card.id.substr(0,card.id.size()-1)).cost>original.cost;
    if(guard){Effect e;e.op=EffectOp::Block;e.target=EffectTarget::Self;e.amount=3;card.effects.push_back(e);card.block+=3;}
    bool changed=false;for(const auto& e:card.effects)changed|=e.op==EffectOp::Energy||e.op==EffectOp::NextEnergy;
    if(!changed){if(guard)card.description+=" Nhận 3 giáp.";return card;}
    card.description.clear();
    for(const auto& e:card.effects){
        if(!card.description.empty())card.description+=" ";
        if(e.op==EffectOp::Energy||e.op==EffectOp::Draw)card.description+="Rút "+std::to_string(e.amount)+" lá.";
        else if(e.op==EffectOp::NextEnergy)card.description+="Rút thêm "+std::to_string(e.amount)+" lá vòng sau.";
        else if(e.op==EffectOp::Block)card.description+="Nhận "+std::to_string(e.amount)+" giáp.";
        else if(e.op==EffectOp::Rage)card.description+="Nhận "+std::to_string(e.amount)+" Nộ.";
        else if(e.op==EffectOp::Damage)card.description+="Gây "+std::to_string(e.amount)+" sát thương.";
        else if(e.op==EffectOp::Heal)card.description+="Hồi "+std::to_string(e.amount)+" HP.";
    }
    if(card.exhaust)card.description+=" Tiêu hao.";
    return card;
}
CardDef Combat::EnemyStarView(int slot)const{
    CardDef card;card.id="shield";card.name="Chờ";card.description="Không thực hiện hành động.";card.type="skill";card.cost=1;
    if(slot<0||slot>=int(stars.enemy.size()))return card;
    const auto& q=stars.enemy[slot];const auto& a=q.action;
    if(q.wait)return card;
    const std::string owner=q.actor==0?enemy_id_:adds.at(q.actor-1).id;
    const auto* own=BossCardIdentity(owner,a);
    const auto& f=Target(q.actor);
    if(f.hp<=0){card.id=own?own->id:std::string{};card.name="Lá bị hủy";card.description="Đơn vị đã gục, bỏ qua hành động.";return card;}
    const int damage=ScaleDamage(a.value*std::max(1,q.actor==0?live.enemy_double:1),f.status,player.status);
    if(a.kind==IntentKind::Charge){
        card.id="comet_lance";card.name=live.charge_left>1?"Tụ lực":"Cú vồ";card.damage=live.charge_left<=1?damage:0;
        card.description=live.charge_left>1?"Chờ 1 vòng, sau đó gây "+std::to_string(damage)+" sát thương.":"Gây "+std::to_string(damage)+" sát thương.";card.type=card.damage>0?"attack":"skill";
    }else if(a.kind==IntentKind::Mimic&&!a.card_id.empty()){
        const auto source=StarCardView(Definition(a.card_id));card=source;card.name="Sao chép: "+source.name;card.description.clear();card.damage=card.block=card.heal=0;card.exhaust=card.power=false;card.keywords={"Sao Chép"};
        for(const auto& e:source.effects){
            const int times=std::max(1,e.times),amount=e.amount;
            if(!card.description.empty())card.description+=" ";
            if(e.op==EffectOp::Damage){const int hit=ScaleDamage(amount*std::max(1,live.enemy_double),f.status,player.status);card.damage+=hit*times;card.description+="Gây "+std::to_string(hit)+(times>1?" ×"+std::to_string(times):"")+" sát thương"+(e.pierce?" xuyên giáp.":".");}
            else if(e.op==EffectOp::Block){card.block+=amount*times;card.description+="Nhận "+std::to_string(amount*times)+" giáp.";}
            else if(e.op==EffectOp::Heal){card.heal+=amount*times;card.description+="Hồi "+std::to_string(amount*times)+" HP.";}
            else if(e.op==EffectOp::Apply)card.description+=(e.target==EffectTarget::Self?"Nhận ":"Gây ")+std::to_string(amount*times)+" "+StatusName(e.status)+".";
            else if(e.op==EffectOp::TempStrength||e.op==EffectOp::Draw||e.op==EffectOp::Energy||e.op==EffectOp::UltDiscount||e.op==EffectOp::NextEnergy)card.description+="Nhận "+std::to_string((e.op==EffectOp::TempStrength?amount:std::min(2,amount))*times)+" Sức Mạnh.";
            else if(e.op==EffectOp::Rage)card.description+="Hút "+std::to_string(amount*times)+" Nộ.";
            else if(e.op==EffectOp::EndBlock)card.description+="Cuối vòng nhận thêm "+std::to_string(amount*times)+" giáp.";
            else if(e.op==EffectOp::Retain)card.description+="Giữ giáp "+std::to_string(amount)+" vòng.";
            else if(e.op==EffectOp::Thorns)card.description+="Phản "+std::to_string(amount*times)+" sát thương mỗi lần bị đánh.";
            else if(e.op==EffectOp::DoubleAttack)card.description+="Nhân "+std::to_string(std::max(2,amount))+" sát thương các đòn tiếp theo.";
            else if(e.op==EffectOp::Revive)card.description+="Hồi sinh một lần với 30% HP.";
        }
        card.type=card.damage>0?"attack":"skill";
    }else if(a.attack||a.kind==IntentKind::Drain){
        card.id=a.hits>1?"double_peck":"heavy";card.name=a.hits>1?"Liên kích":"Đòn đánh";card.type="attack";card.damage=damage*std::max(1,a.hits);
        card.description="Gây "+std::to_string(damage)+" sát thương"+(a.hits>1?" ×"+std::to_string(a.hits):"")+".";
        if(a.kind==IntentKind::Drain)card.description+=" Hút "+std::to_string(a.status_amount)+" Nộ.";
        else if(a.status_amount>0)card.description+=" Gây "+std::to_string(a.status_amount)+" "+StatusName(a.status)+".";
    }else if(a.kind==IntentKind::Buff){card.id="crow";card.name="Tăng sức";card.description="Nhận "+std::to_string(a.status_amount)+" "+StatusName(a.status)+" và "+std::to_string(a.value)+" giáp.";}
    else if(a.kind==IntentKind::Debuff){card.id="toxin";card.name="Yểm bùa";card.description="Gây "+std::to_string(a.status_amount)+" "+StatusName(a.status)+".";}
    else if(a.kind==IntentKind::Curse){card.id="mud";card.name="Bùn nguyền";card.description="Biến một lá trên tay vòng sau thành Bùn Lầy.";}
    else if(a.kind==IntentKind::Summon){card.id="shadow_dance";card.name="Gọi viện binh";card.description="Gọi "+std::to_string(a.hits)+" viện binh.";}
    else{card.block=a.value;card.name="Dựng giáp";card.description="Nhận "+std::to_string(a.value)+" giáp.";}
    card.keywords={card.type=="attack"?"Tấn Công":"Kỹ Năng"};
    if(own){
        card.id=own->id;card.name=own->name;card.upgraded=false;card.rarity="boss";card.class_id=owner;
        card.keywords.insert(card.keywords.begin(),"Thẻ trùm");
        if(a.kind==IntentKind::Mimic&&!a.card_id.empty())card.description="Sao chép "+Definition(a.card_id).name+". "+card.description;
    }
    return card;
}
}
