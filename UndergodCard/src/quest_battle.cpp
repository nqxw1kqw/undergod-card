#include "quest_core.hpp"

namespace undergod {
std::vector<int> Combat::Targets()const{
    std::vector<int> result;if(enemy.hp>0)result.push_back(0);
    for(int i=0;i<int(adds.size());++i)if(adds[i].fighter.hp>0)result.push_back(i+1);
    return result;
}
const Fighter& Combat::Target(int index)const{return index==0?enemy:adds.at(size_t(index-1)).fighter;}
Fighter& Combat::Target(int index){return index==0?enemy:adds.at(size_t(index-1)).fighter;}
bool Combat::SelectTarget(int index){
    if(phase!=Phase::Player||index<0||index>int(adds.size())||Target(index).hp<=0)return false;
    live.target=index;return true;
}
std::string Combat::TargetName(int index)const{
    if(definition_.mechanic=="fox"&&phase_index_>0&&Targets().size()>1)return "Ảo ảnh";
    return index==0?PhaseName():adds.at(size_t(index-1)).name;
}
std::string Combat::Sprite(int index)const{return index==0?definition_.sprite:adds.at(size_t(index-1)).sprite;}
int Combat::UltimateCost()const{return std::max(40,config.balance.rage_max-mods_.ultimate_discount-live.ult_discount);}

void Combat::LockCard(){
    live.locked=-1;
    if(stars.resolving)return;
    if(CurrentPhase().lock_card&&!hand.empty()){
        if(!stars.enabled)live.locked=std::min(int(hand.size())-1,int(target_rng_.Next()*hand.size()));
        else{std::vector<int> candidates;for(int i=0;i<int(hand.size());++i)if(!IsQueuedStar(i))candidates.push_back(i);if(!candidates.empty())live.locked=candidates[std::min(int(candidates.size())-1,int(target_rng_.Next()*candidates.size()))];}
    }
}

Intent Combat::PickIntent(const EnemyDef& def){
    Intent result;
    if(def.patterns.empty()){
        result.attack=intent_rng_.Next()<def.attack_chance;result.kind=result.attack?IntentKind::Attack:IntentKind::Defend;
        result.value=result.attack?def.min_damage+int(intent_rng_.Next()*(def.max_damage-def.min_damage+1)):def.block;
        return result;
    }
    int total=0;for(const auto& pattern:def.patterns)total+=pattern.weight;
    const double roll=intent_rng_.Next()*total;int sum=0;const IntentPattern* choice=&def.patterns.back();
    for(const auto& pattern:def.patterns){sum+=pattern.weight;if(roll<sum){choice=&pattern;break;}}
    result.kind=choice->kind;result.hits=choice->hits;result.status=choice->status;result.status_amount=choice->status_amount;result.summon_id=choice->summon_id;
    result.attack=result.kind==IntentKind::Attack||result.kind==IntentKind::MultiAttack||result.kind==IntentKind::Mimic||result.kind==IntentKind::Drain;
    result.value=result.attack?choice->min_damage+int(intent_rng_.Next()*(choice->max_damage-choice->min_damage+1)):choice->block;
    return result;
}

void Combat::Summon(const std::string& id,int count,bool illusion){
    const EnemyDef* def=nullptr;for(const auto& candidate:config.bestiary)if(candidate.id==id)def=&candidate;
    if(!def||count<=0)return;
    for(int n=0;n<count&&adds.size()<4;++n){
        EnemyUnit unit;unit.id=def->id;unit.name=def->name;unit.sprite=def->sprite;unit.illusion=illusion;
        unit.fighter={illusion?1:def->hp,illusion?1:def->hp,0,{}};unit.intent=PickIntent(*def);adds.push_back(std::move(unit));
    }
    live.slots.clear();for(int i=0;i<=int(adds.size());++i)live.slots.push_back(i);
    if(illusion){for(int i=int(live.slots.size())-1;i>0;--i)std::swap(live.slots[i],live.slots[int(target_rng_.Next()*(i+1))]);live.target=live.slots.front();}
    events.push_back({EventType::Summon,true,count});
}

void Combat::HitTarget(int index,int damage,bool pierce,bool ranged){
    if(index<0||index>int(adds.size())||Target(index).hp<=0)return;
    if(index==0&&live.airborne&&!ranged)damage=std::max(0,damage/2);
    const auto result=ResolveDamage(Target(index),damage,pierce);
    live.damage_dealt+=result.lost;
    events.push_back({EventType::Damage,true,result.lost,result.blocked,StatusId::Poison,index});
    if(index==0){
        if(enemy.hp==0&&live.enemy_revive==1){enemy.hp=std::max(1,enemy.max_hp*30/100);live.enemy_revive=2;events.push_back({EventType::Revive,true,enemy.hp});}
        if(enemy.hp==0&&definition_.mechanic=="fox")for(auto& add:adds)if(add.illusion)add.fighter.hp=0;
        if(damage>0&&live.enemy_thorns>0){const auto reflected=ResolveDamage(player,live.enemy_thorns);live.damage_taken+=reflected.lost;events.push_back({EventType::Damage,false,reflected.lost,reflected.blocked});}
        if(live.airborne){live.flight_damage+=result.lost;if(live.flight_damage>=20){live.airborne=false;live.land_turns=2;}}
        AdvancePhaseIfAny();
    }
    if(Target(index).hp<=0&&live.target==index){const auto targets=Targets();if(!targets.empty())live.target=targets.front();}
    CheckEnd();
}

void Combat::ResolveAdd(int index){
    auto& add=adds.at(size_t(index));if(add.fighter.hp<=0)return;
    const int slot=index+1;attacker_=slot;add.fighter.block=0;
    if(add.fighter.status.poison>0){const int amount=add.fighter.status.poison--;const auto result=ResolveDamage(add.fighter,amount,true);live.damage_dealt+=result.lost;events.push_back({EventType::Damage,true,result.lost,0,StatusId::Poison,slot});}
    if(add.fighter.hp<=0){CheckEnd();return;}
    const auto action=add.intent;
    if(action.attack)for(int hit=0;hit<std::max(1,action.hits)&&add.fighter.hp>0&&phase!=Phase::Lost;++hit)Hit(false,ScaleDamage(action.value,add.fighter.status,player.status));
    else if(action.kind==IntentKind::Defend)add.fighter.block=action.value;
    else if(action.kind==IntentKind::Buff)AddStatusStack(add.fighter,action.status,action.status_amount);
    else if(action.kind==IntentKind::Debuff)AddStatusEvent(player,false,action.status,action.status_amount);
    if(action.attack&&action.status_amount>0&&player.hp>0)AddStatusEvent(player,false,action.status,action.status_amount);
    add.fighter.status.DecayDebuffs();
    for(const auto& def:config.bestiary)if(def.id==add.id){add.intent=PickIntent(def);break;}
    attacker_=0;
}

std::string Combat::PreviewIntentText()const{
    if(!mods_.preview||phase!=Phase::Player)return "";
    if(stars.enabled){
        if(stars.resolving)return "";
        Combat future=*this;future.player.hp=future.player.max_hp=100000;future.player.block=100000;
        std::string text;
        for(int round=1;round<=2;++round){
            while(future.stars.player.size()<TurnStars)future.QueueWait();
            if(!future.BeginStarAttack())break;
            for(int action=0;action<6&&future.stars.resolving;++action)future.ResolveStarAction();
            future.FinishStarRound();if(future.phase!=Phase::Player)break;
            if(!text.empty())text+=" | ";text+="Vòng +"+std::to_string(round)+": ";
            for(int slot=0;slot<3;++slot){if(slot)text+=" → ";text+=future.EnemyStarView(slot).name;}
        }
        return text;
    }
    Combat future=*this;future.player.hp=future.player.max_hp=100000;future.player.block=100000;
    future.EndPlayer();future.ActEnemy();
    const auto next=future.IntentText();
    future.EndPlayer();future.ActEnemy();
    return "Lượt +1: "+next+" | +2: "+future.IntentText();
}
}
