#include "quest_run.hpp"

namespace undergod {
namespace {
int RarityWeight(const std::string& rarity){
    if(rarity=="rare") return 1;
    if(rarity=="uncommon") return 3;
    return 6;
}
bool Rewardable(const CardDef& card){return card.rarity!="starter"&&card.copies<=0;}
}

std::vector<MapNode> BuildRoute(const std::vector<RouteNode>& route){
    std::vector<MapNode> map;
    for(const auto& node:route){
        if(!map.empty()) map.back().next.push_back(int(map.size()));
        map.push_back({node.kind,node.enemy_id,{}});
    }
    return map;
}

const EnemyDef& FindEnemy(const Config& config,const std::string& id){
    for(const auto& enemy:config.bestiary) if(enemy.id==id) return enemy;
    throw std::runtime_error("Unknown enemy: "+id);
}

uint32_t EncounterSeed(unsigned seed,int index){return seed+uint32_t(std::max(0,index))*0x9E3779B9u;}

void StartRun(RunState& run,Combat& combat,unsigned seed){
    run=RunState{};
    run.seed=seed;
    run.hp=run.max_hp=combat.config.balance.player_hp;
    run.class_id=combat.config.player_id;
    run.deck=StartingDeck(combat.config);
    run.map=BuildRoute(combat.config.route);
    if(run.map.empty()) throw std::runtime_error("Run route is empty");
    if(!IsFightNode(run.map.front().kind)) throw std::runtime_error("Encounter kind is not playable yet");
    const auto& enemy=FindEnemy(combat.config,run.map.front().encounter_id);
    combat.BeginEncounter(enemy,EncounterSeed(seed,0),run.hp,run.max_hp,0,run.deck);
}

std::string NextEncounterName(const RunState& run,const Config& config){
    const int next=run.index+1;
    if(next<0||next>=int(run.map.size())) return "";
    return FindEnemy(config,run.map[next].encounter_id).name;
}

void ObserveCombat(RunState& run,Combat& combat){
    if(run.fight_resolved||run.victory||run.defeat) return;
    if(combat.phase!=Phase::Won&&combat.phase!=Phase::Lost) return;
    run.fight_resolved=true;
    if(combat.phase==Phase::Lost){run.defeat=true;return;}
    run.hp=combat.player.hp;
    run.rage=combat.rage;
    if(run.index+1<int(run.map.size())){
        if(!IsFightNode(run.map[run.index+1].kind)) throw std::runtime_error("Encounter kind is not playable yet");
        std::vector<const CardDef*> pool;
        for(const auto& card:combat.config.cards) if(Rewardable(card)) pool.push_back(&card);
        Rng rng;
        rng.Seed(run.seed^0xC0FFEEu^(uint32_t(run.index+1)*0x45D9F3Bu));
        const int picks=std::min(combat.config.reward_choices,int(pool.size()));
        run.rewards.clear();
        for(int n=0;n<picks;++n){
            int total=0;
            for(const auto* card:pool) total+=RarityWeight(card->rarity);
            const double roll=rng.Next()*std::max(1,total);
            int acc=0,index=int(pool.size())-1;
            for(int i=0;i<int(pool.size());++i){
                acc+=RarityWeight(pool[i]->rarity);
                if(roll<acc){index=i;break;}
            }
            run.rewards.push_back(pool[index]->id);
            pool.erase(pool.begin()+index);
        }
        run.pending_reward=!run.rewards.empty();
        if(!run.pending_reward) run.victory=true;
    }else run.victory=true;
}

bool ChooseReward(RunState& run,Combat& combat,int index){
    if(!run.pending_reward||index<0||index>=int(run.rewards.size())) return false;
    run.deck.push_back(run.rewards[index]);
    run.pending_reward=false;
    run.rewards.clear();
    run.fight_resolved=false;
    ++run.index;
    if(run.index>=int(run.map.size())){run.victory=true;return true;}
    const auto& node=run.map[run.index];
    if(!IsFightNode(node.kind)) throw std::runtime_error("Encounter kind is not playable yet");
    const auto& enemy=FindEnemy(combat.config,node.encounter_id);
    combat.BeginEncounter(enemy,EncounterSeed(run.seed,run.index),run.hp,run.max_hp,run.rage,run.deck);
    return true;
}

int SuggestCard(const Combat& combat){
    if(combat.phase!=Phase::Player) return -1;
    const int incoming=combat.IncomingDamage();
    int best=-1;
    double score=-1;
    for(int i=0;i<int(combat.hand.size());++i){
        if(!combat.CanPlay(i)) continue;
        const auto& card=combat.Definition(combat.hand[i]);
        double value=card.damage;
        if(card.damage>0&&card.damage>=combat.enemy.hp+combat.enemy.block) value+=80;
        if(card.block>0){
            const int need=std::max(0,incoming-combat.player.block);
            value+=std::min(card.block,std::max(need,0))*1.4+(need>0?2.0:.4);
            if(need>=combat.player.hp) value+=120;
        }
        if(card.heal>0){
            const int missing=combat.player.max_hp-combat.player.hp;
            value+=std::min(card.heal,missing)*(combat.player.hp*2<combat.player.max_hp?2.4:.55);
        }
        value+=card.poison*(combat.PhaseCount()>1?3.2:2.2);
        value+=card.vulnerable*(combat.enemy.status.vulnerable>0?1.0:3.4);
        value+=card.strength*4.2;
        value+=card.weak*(incoming>0?2.4:.8);
        value+=card.cards_drawn*2.1;
        value+=card.energy_gain*2.0;
        if(card.cost==0) value+=1.2;
        value/=std::max(1,card.cost);
        if(value>score){score=value;best=i;}
    }
    return score>0?best:-1;
}

int SuggestReward(const RunState& run,const Config& config){
    int best=0;
    double score=-1;
    for(int i=0;i<int(run.rewards.size());++i){
        const auto& card=[&]() -> const CardDef& {
            for(const auto& entry:config.cards) if(entry.id==run.rewards[i]) return entry;
            throw std::runtime_error("Unknown reward card");
        }();
        double value=card.damage*3.0+card.block*1.15+card.heal*2.4+card.poison*4.0+card.strength*6.0+card.vulnerable*4.0+card.weak*2.0+card.cards_drawn*2.6+card.energy_gain*3.0;
        if(card.cost==0) value+=4;
        if(card.cost>=3) value-=2;
        if(value>score){score=value;best=i;}
    }
    return best;
}
}
