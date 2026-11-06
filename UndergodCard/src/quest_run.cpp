#include "quest_run.hpp"

namespace undergod {
namespace {
int RarityWeight(const std::string& rarity){return rarity=="legendary"?1:(rarity=="rare"||rarity=="uncommon"?3:6);}
bool Rewardable(const CardDef& card){return !card.upgraded&&card.rarity!="starter"&&card.rarity!="curse"&&card.copies<=0;}
bool Available(const RunState& run,const CardDef& card){return Rewardable(card)&&(card.class_id=="any"||card.class_id==run.class_id)&&(card.requires_skill.empty()||std::find(run.skills.begin(),run.skills.end(),card.requires_skill)!=run.skills.end());}
int CardPrice(const CardDef& card){return card.rarity=="legendary"?150:(card.rarity=="rare"||card.rarity=="uncommon"?75:50);}
int CountId(const std::vector<std::string>& ids,const std::string& id){int n=0;for(const auto& have:ids)if(have==id)++n;return n;}
bool Owned(const std::vector<std::string>& ids,const std::string& id){return CountId(ids,id)>0;}
int Discount(const RunState& run,const Config& config){int discount=0;for(const auto& id:run.relics)discount+=FindRelic(config,id).shop_discount;return std::min(40,discount);}
int PriceFor(const RunState& run,const Config& config,int base){return std::max(1,base*(100-Discount(run,config))/100);}
void ShowMap(RunState& run){
    run.place=Place::Map;run.pending_reward=false;run.rest_used=false;run.options.clear();
    run.reward_from_event=false;run.selection.clear();run.event_choices.clear();run.event_title.clear();run.event_text.clear();
    if(run.node<0){for(int i=0;i<int(run.map.size());++i)if(run.map[i].floor==0)run.options.push_back(i);return;}
    run.options=run.map[run.node].next;
}
void GiveRelic(RunState& run,const Config& config,const std::string& id){
    if(id.empty())return;
    if(Owned(run.relics,id)){run.gold+=15;return;}
    const auto& relic=FindRelic(config,id);
    run.relics.push_back(id);
    if(relic.max_hp>0){run.max_hp+=relic.max_hp;run.hp+=relic.max_hp;}
    if(relic.potion_slot>0)run.potion_slots=std::min(4,run.potion_slots+relic.potion_slot);
}
void GivePotion(RunState& run,const std::string& id){if(id.empty())return;if(int(run.potions.size())>=run.potion_slots){run.gold+=10;return;}run.potions.push_back(id);}
void RandomRelic(RunState& run,const Config& config,Rng& rng,const std::string& rarity=""){
    std::vector<std::string> pool;for(const auto& r:config.relics)if(r.rarity!="boss"&&(rarity.empty()||r.rarity==rarity)&&!Owned(run.relics,r.id))pool.push_back(r.id);
    if(pool.empty()){run.gold+=25;return;}GiveRelic(run,config,pool[std::min(int(pool.size())-1,int(rng.Next()*pool.size()))]);
}
void BeginFight(RunState& run,Combat& combat,const std::string& id){
    run.active_encounter_id=id;run.place=Place::Fight;run.fight_resolved=false;run.pending_reward=false;
    int opening_rage=run.rage;for(const auto& relic:run.relics)opening_rage=std::max(opening_rage,FindRelic(combat.config,relic).start_rage);
    opening_rage=std::min(combat.config.balance.rage_max,opening_rage+RunBonus(run,"start_rage"));
    combat.BeginEncounter(FindEnemy(combat.config,id),EncounterSeed(run.seed,run.node+run.act*100+int(run.sequence.size())*1000),run.hp,run.max_hp,opening_rage,run.deck,ModsFor(run,combat.config));
}
void BankBattle(RunState& run,Combat& combat){
    run.hp=combat.player.hp;run.rage=combat.rage;run.phoenix_spent|=combat.live.revives>0;
    run.metrics["damage"]+=combat.live.damage_dealt;run.metrics["poison"]+=combat.live.poison_applied;run.metrics["revives"]+=combat.live.revives;run.metrics["ultimates"]+=combat.live.ultimates;
}
void HealTo(RunState& run,int amount){if(amount>0)run.hp=std::min(run.max_hp,run.hp+amount);}
void Hurt(RunState& run,int damage){if(damage<=0)return;run.hp=std::max(0,run.hp-damage);if(run.hp<=0){run.hp=0;run.defeat=true;}}
bool Pay(RunState& run,int gold){if(gold>=0){run.gold+=gold;return true;}if(run.gold<-gold)return false;run.gold+=gold;return true;}
int UpgradeIndex(const RunState& run,const Config& config){
    int best=-1,score=-1;
    for(int i=0;i<int(run.deck.size());++i){
        const auto& card=FindCard(config,run.deck[i]);
        if(card.upgraded||card.upgrade_id.empty())continue;
        const int value=card.damage*3+card.block+card.heal*2+card.poison*4+card.strength*5+(card.cost==0?2:0);
        if(value>score){score=value;best=i;}
    }
    return best;
}
int CardValue(const CardDef& card){return card.damage*3+card.block+card.heal*2+card.poison*4+card.strength*6+card.vulnerable*4+card.weak*2+card.cards_drawn*3+card.energy_gain*3+(card.cost==0?4:0);}
void OfferRewards(RunState& run,const Config& config){
    const bool elite=!run.active_encounter_id.empty()&&FindEnemy(config,run.active_encounter_id).role=="elite";
    std::vector<const CardDef*> pool;for(const auto& card:config.cards)if(Available(run,card)&&(!elite||card.rarity!="common"))pool.push_back(&card);
    Rng rng;rng.Seed(run.seed^0xC0FFEEu^(uint32_t(run.node+1+run.act*101)*0x45D9F3Bu));
    run.rewards.clear();
    for(int n=0;n<std::min(config.reward_choices,int(pool.size()));++n){
        const std::string tiers[]={"common","rare","legendary"};const int weights[]={60,33,7};
        int total=0;for(int tier=0;tier<3;++tier)if(std::any_of(pool.begin(),pool.end(),[&](const auto* c){return c->rarity==tiers[tier];}))total+=weights[tier];
        const double roll=rng.Next()*std::max(1,total);int acc=0;std::string rarity=pool.back()->rarity;
        for(int tier=0;tier<3;++tier){if(!std::any_of(pool.begin(),pool.end(),[&](const auto* c){return c->rarity==tiers[tier];}))continue;acc+=weights[tier];if(roll<acc){rarity=tiers[tier];break;}}
        std::vector<int> candidates;for(int i=0;i<int(pool.size());++i)if(pool[i]->rarity==rarity)candidates.push_back(i);
        const int index=candidates[std::min(int(candidates.size())-1,int(rng.Next()*candidates.size()))];
        run.rewards.push_back(pool[index]->id);pool.erase(pool.begin()+index);
    }
    run.pending_reward=!run.rewards.empty();
    if(run.pending_reward)run.place=Place::Reward;else ShowMap(run);
}
void FillShop(RunState& run,const Config& config){
    Rng rng;rng.Seed(run.seed^0x5110ABu^(uint32_t(run.node+3)*0x9E3779B9u));
    std::vector<const CardDef*> cards;std::vector<const RelicDef*> relics;std::vector<const PotionDef*> potions;
    for(const auto& card:config.cards)if(Available(run,card))cards.push_back(&card);
    for(const auto& relic:config.relics)if(relic.rarity!="boss"&&!Owned(run.relics,relic.id))relics.push_back(&relic);
    for(const auto& potion:config.potions)potions.push_back(&potion);
    run.shop.clear();
    auto pick_card=[&](){
        if(cards.empty())return;
        int total=0;for(const auto* card:cards)total+=RarityWeight(card->rarity);
        const double roll=rng.Next()*std::max(1,total);int acc=0,index=int(cards.size())-1;
        for(int i=0;i<int(cards.size());++i){acc+=RarityWeight(cards[i]->rarity);if(roll<acc){index=i;break;}}
        run.shop.push_back({Ware::Card,cards[index]->id,PriceFor(run,config,CardPrice(*cards[index])),false});
        cards.erase(cards.begin()+index);
    };
    auto pick_any=[&](auto& pool,Ware ware,auto price_of){
        if(pool.empty())return;
        const int index=std::min(int(pool.size())-1,int(rng.Next()*pool.size()));
        run.shop.push_back({ware,pool[index]->id,PriceFor(run,config,price_of(pool[index])),false});
        pool.erase(pool.begin()+index);
    };
    for(int i=0;i<3;++i)pick_card();
    for(int i=0;i<2;++i)pick_any(relics,Ware::Relic,[](const RelicDef* relic){return relic->price;});
    for(int i=0;i<2;++i)pick_any(potions,Ware::Potion,[](const PotionDef* potion){return potion->price;});
}
int PotionHeal(const PotionDef& potion){int n=0;for(const auto& effect:potion.effects)if(effect.op==EffectOp::Heal)n+=effect.amount*std::max(1,effect.times);return n;}
int PotionHarm(const PotionDef& potion){int n=0;for(const auto& effect:potion.effects)if(effect.op==EffectOp::Apply&&effect.target==EffectTarget::Enemy)n+=effect.amount;if(potion.effects.size()==1&&potion.effects[0].op==EffectOp::Damage)n+=potion.effects[0].amount;return n;}
int ChoiceScore(const RunState& run,const EventChoice& choice,const Config& config){
    const int missing=std::max(0,run.max_hp-run.hp);
    int score=std::min(choice.heal,std::max(0,missing))*(run.hp*2<run.max_hp?3:1);
    if(choice.gold>0)score+=choice.gold;
    if(choice.gold<0)score+=run.gold>=-choice.gold?choice.gold/2:-40;
    score-=choice.damage*(run.hp<28?4:1);
    score+=choice.max_hp*4;
    if(!choice.card.empty())score+=14;
    if(!choice.relic.empty())score+=Owned(run.relics,choice.relic)?4:18;
    if(!choice.potion.empty())score+=int(run.potions.size())<run.potion_slots?12:3;
    if(choice.upgrade)score+=UpgradeIndex(run,config)>=0?16:2;
    return score;
}
int NodeScore(const RunState& run,const MapNode& node){
    const bool hurt=run.hp*100<run.max_hp*68,safe=run.hp*100>=run.max_hp*72;
    switch(node.kind){
    case NodeKind::Rest: return hurt?90:12;
    case NodeKind::Shop: return run.gold>=55?48:8;
    case NodeKind::Event: return 32;
    case NodeKind::Combat: return hurt?18:42;
    case NodeKind::Elite: return safe?46:6;
    case NodeKind::Boss: return 100;
    }
    return 0;
}
}

const EnemyDef& FindEnemy(const Config& config,const std::string& id){for(const auto& enemy:config.bestiary)if(enemy.id==id)return enemy;throw std::runtime_error("Unknown enemy: "+id);}
const CardDef& FindCard(const Config& config,const std::string& id){for(const auto& card:config.cards)if(card.id==id)return card;throw std::runtime_error("Unknown card: "+id);}
const RelicDef& FindRelic(const Config& config,const std::string& id){for(const auto& relic:config.relics)if(relic.id==id)return relic;throw std::runtime_error("Unknown relic: "+id);}
const PotionDef& FindPotion(const Config& config,const std::string& id){for(const auto& potion:config.potions)if(potion.id==id)return potion;throw std::runtime_error("Unknown potion: "+id);}
uint32_t EncounterSeed(unsigned seed,int index){return seed+uint32_t(std::max(0,index))*0x9E3779B9u;}

CombatMods ModsFor(const RunState& run,const Config& config){
    CombatMods mods;
    const auto& character=FindClass(config,run.class_id);mods.class_id=run.class_id;mods.ultimate=character.ultimate;mods.ultimate_damage=character.ultimate_damage;mods.ultimate_hits=character.ultimate_hits;mods.mana=character.mana;
    mods.energy=character.energy-config.balance.energy+RunBonus(run,"energy");mods.draw=character.draw-config.balance.draw+RunBonus(run,"draw");
    mods.bonus_damage=RunBonus(run,"damage");mods.bonus_block=RunBonus(run,"block");mods.spear_bonus=RunBonus(run,"spear_bonus");mods.poison_bonus=RunBonus(run,"poison_bonus");
    mods.first_attack=RunBonus(run,"first_attack");mods.third_attack=RunBonus(run,"third_attack");mods.retain_block=RunBonus(run,"retain_block");mods.ultimate_bonus=RunBonus(run,"ultimate_bonus");mods.ultimate_discount=RunBonus(run,"ultimate_discount");mods.ultimate_vulnerable=RunBonus(run,"ultimate_vulnerable");mods.max_mana=3+RunBonus(run,"max_mana");mods.revive_spent=run.phoenix_spent;
    if(RunBonus(run,"start_block")>0)mods.combat_start.push_back({EffectOp::Block,EffectTarget::Self,StatusId::Block,RunBonus(run,"start_block"),1});
    const auto penalties=ChallengeBonuses(config,run.challenge);auto penalty=[&](const std::string& id){const auto it=penalties.find(id);return it==penalties.end()?0:it->second;};
    const std::string role=run.active_encounter_id.empty()?"combat":FindEnemy(config,run.active_encounter_id).role;
    mods.enemy_hp_percent=100+penalty("enemy_hp")+(role=="boss"?penalty("boss_hp"):run.act*10)+(role=="elite"?penalty("elite_hp"):0);
    if(run.active_encounter_id=="undergod")mods.enemy_hp_percent+=RunBonus(run,"final_boss_hp");
    mods.enemy_damage=penalty("enemy_damage")+(role=="boss"?penalty("boss_damage"):run.act);
    for(const auto& id:run.relics){
        const auto& relic=FindRelic(config,id);
        mods.bonus_damage+=relic.bonus_damage;mods.bonus_block+=relic.bonus_block;
        mods.first_attack+=relic.first_attack;mods.retain_block=std::max(mods.retain_block,relic.retain_block);mods.energy+=relic.energy;mods.draw+=relic.draw;mods.ultimate_bonus+=relic.ultimate_bonus;mods.preview|=relic.preview;mods.comet|=relic.comet;
        mods.combat_start.insert(mods.combat_start.end(),relic.combat_start.begin(),relic.combat_start.end());
        mods.turn_start.insert(mods.turn_start.end(),relic.turn_start.begin(),relic.turn_start.end());
    }
    return mods;
}

int RunBonus(const RunState& run,const std::string& key){const auto it=run.bonuses.find(key);return it==run.bonuses.end()?0:it->second;}
std::map<std::string,int> ChallengeBonuses(const Config& config,int level){std::map<std::string,int> result;for(const auto& rule:config.challenges)if(rule.level<=level)for(const auto& pair:rule.penalties)result[pair.first]+=pair.second;return result;}

std::vector<MapNode> GenerateMap(const Config& config,unsigned seed,int act){
    if(act<0||act>=int(config.acts.size()))throw std::runtime_error("Unknown act");const auto& chapter=config.acts[act];
    const int columns=std::max(1,act==0?config.map_columns:chapter.columns),floors=std::max(1,act==0?config.map_floors:chapter.floors);
    seed^=uint32_t(act)*0x7F4A7C15u;
    std::vector<int> event_order(config.events.size());
    for(int i=0;i<int(event_order.size());++i)event_order[i]=i;
    Rng rng;rng.Seed(seed^0xE11Eu);
    for(int i=int(event_order.size())-1;i>0;--i)std::swap(event_order[i],event_order[int(rng.Next()*(i+1))]);
    int event_cursor=0;
    std::vector<MapNode> map;
    for(int floor=0;floor<floors;++floor)for(int column=0;column<columns;++column){
        MapNode node;node.floor=floor;node.column=column;
        if(floor==0)node.kind=NodeKind::Combat;
        else if(floor==floors-1)node.kind=NodeKind::Rest;
        else{
            int total=0;for(int kind=0;kind<5;++kind)if(kind!=int(NodeKind::Elite)||floor>=config.elite_min_floor)total+=config.map_weights[kind];
            const double roll=rng.Next()*total;int sum=0;
            for(int kind=0;kind<5;++kind){if(kind==int(NodeKind::Elite)&&floor<config.elite_min_floor)continue;sum+=config.map_weights[kind];if(roll<sum){node.kind=NodeKind(kind);break;}}
        }
        if(node.kind==NodeKind::Combat){
            std::vector<std::string> pool;for(const auto& id:chapter.combats)if(FindEnemy(config,id).min_floor<=floor)pool.push_back(id);
            if(pool.empty())throw std::runtime_error("No enemy available at this floor");
            node.encounter_id=floor==0?chapter.combats.front():pool[std::min(int(pool.size())-1,int(rng.Next()*pool.size()))];
            node.reward_gold=config.combat_gold_min+int(rng.Next()*(config.combat_gold_max-config.combat_gold_min+1));
        }else if(node.kind==NodeKind::Elite){
            node.encounter_id=chapter.elites[std::min(int(chapter.elites.size())-1,int(rng.Next()*chapter.elites.size()))];
            node.reward_gold=config.elite_gold_min+int(rng.Next()*(config.elite_gold_max-config.elite_gold_min+1));
        }
        else if(node.kind==NodeKind::Event&&!config.events.empty()){node.event_id=config.events[event_order[event_cursor%int(event_order.size())]].id;++event_cursor;}
        map.push_back(node);
    }
    auto at=[&](int floor,int column){return floor*columns+column;};
    for(int floor=0;floor<floors-1;++floor)for(int column=0;column<columns;++column)for(int delta=-1;delta<=1;++delta){
        const int next=column+delta;if(next>=0&&next<columns)map[at(floor,column)].next.push_back(at(floor+1,next));
    }
    MapNode boss;boss.kind=NodeKind::Boss;boss.floor=floors;boss.column=columns/2;boss.encounter_id=chapter.bosses[std::min(int(chapter.bosses.size())-1,int(rng.Next()*chapter.bosses.size()))];boss.reward_gold=50;
    const int boss_index=int(map.size());
    for(int column=0;column<columns;++column)map[at(floors-1,column)].next.push_back(boss_index);
    map.push_back(boss);
    if(!chapter.final_boss.empty()){
        MapNode rest;rest.kind=NodeKind::Rest;rest.floor=floors+1;rest.column=columns/2;rest.next={int(map.size())+1};map.back().next={int(map.size())};map.push_back(rest);
        boss.encounter_id=chapter.final_boss;boss.floor=floors+2;boss.next.clear();map.push_back(boss);
    }
    return map;
}

void StartRun(RunState& run,Combat& combat,unsigned seed,const std::string& class_id,int challenge,const std::vector<std::string>& skills){
    run=RunState{};
    combat.Reset(seed);
    const auto& character=FindClass(combat.config,class_id);run.class_id=class_id;run.challenge=std::clamp(challenge,0,10);run.skills=skills;
    for(const auto& id:skills)for(const auto& s:combat.config.skills)if(s.id==id&&s.class_id==class_id)for(const auto& pair:s.bonuses)run.bonuses[pair.first]+=pair.second;
    const auto penalties=ChallengeBonuses(combat.config,run.challenge);auto penalty=[&](const std::string& id){const auto it=penalties.find(id);return it==penalties.end()?0:it->second;};
    run.seed=seed;run.hp=run.max_hp=std::max(15,character.hp+RunBonus(run,"max_hp")+penalty("player_hp"));run.gold=std::max(0,combat.config.gold_start+penalty("start_gold"));run.potion_slots=std::clamp(combat.config.potion_slots+penalty("potion_slots"),1,4);
    run.deck=character.deck;run.map=GenerateMap(combat.config,seed,0);
    if(run.map.empty())throw std::runtime_error("Run map is empty");
    ShowMap(run);
}

std::string NextEncounterName(const RunState& run,const Config& config){
    for(int option:run.options)if(IsFightNode(run.map[option].kind))return FindEnemy(config,run.map[option].encounter_id).name;
    return "";
}

void ObserveCombat(RunState& run,Combat& combat){
    if(run.fight_resolved||run.victory||run.defeat||run.place!=Place::Fight)return;
    if(run.trial_limit>0&&combat.turn>run.trial_limit&&combat.phase==Phase::Player){BankBattle(run,combat);run.fight_resolved=true;run.trial_limit=0;run.last_outcome="Hết ba lượt. Bạn chưa hạ được Võ Sĩ Xiếc.";ShowMap(run);return;}
    if(combat.phase!=Phase::Won&&combat.phase!=Phase::Lost)return;
    run.fight_resolved=true;
    BankBattle(run,combat);
    if(combat.phase==Phase::Lost){run.defeat=true;return;}
    if(run.trial_limit>0){run.trial_limit=0;run.gold+=100;run.last_outcome="Thắng thử thách ba lượt! Nhận 100 vàng.";ShowMap(run);return;}
    if(!run.sequence.empty()){const auto next=run.sequence.front();run.sequence.erase(run.sequence.begin());BeginFight(run,combat,next);return;}
    if(combat.Role()=="boss"){
        run.defeated_bosses.push_back(combat.EnemyId());
        if(run.act+1==int(combat.config.acts.size())&&run.map[run.node].next.empty()){run.victory=true;return;}
        HealTo(run,run.max_hp/2);combat.player.hp=run.hp;run.gold+=50;
        run.advance_act=run.map[run.node].next.empty();run.relic_rewards.clear();
        for(const auto& r:combat.config.relics)if(r.rarity=="boss"&&!Owned(run.relics,r.id))run.relic_rewards.push_back(r.id);
        if(!run.relic_rewards.empty()){run.place=Place::RelicReward;return;}
        run.gold+=50;
        if(run.advance_act){++run.act;run.node=run.index=-1;run.map=GenerateMap(combat.config,run.seed,run.act);}
        ShowMap(run);return;
    }
    int bonus=0;for(const auto& id:run.relics)bonus+=FindRelic(combat.config,id).gold_bonus;
    if(run.node>=0)run.gold+=std::max(0,run.map[run.node].reward_gold*run.reward_multiplier+bonus)+run.sequence_gold;
    if(combat.Role()=="elite"||run.sequence_relic){
        std::vector<std::string> available;for(const auto& relic:combat.config.relics)if(relic.rarity!="boss"&&!Owned(run.relics,relic.id))available.push_back(relic.id);
        if(!available.empty()){Rng rng;rng.Seed(run.seed^uint32_t(run.node+1)*0x6C8E9CF5u);
            const int count=combat.Role()=="elite"?run.reward_multiplier:1;
            for(int i=0;i<count;++i){if(available.empty()){run.gold+=25;break;}const int which=std::min(int(available.size())-1,int(rng.Next()*available.size()));GiveRelic(run,combat.config,available[which]);available.erase(available.begin()+which);}
        }
        else run.gold+=25;
        combat.player.max_hp=run.max_hp;combat.player.hp=run.hp;
    }
    HealTo(run,RunBonus(run,"companion_heal"));combat.player.hp=run.hp;
    run.sequence_gold=0;run.sequence_relic=false;run.reward_multiplier=1;run.reward_from_event=false;
    OfferRewards(run,combat.config);
}

bool ChooseNode(RunState& run,Combat& combat,int option){
    if(run.place!=Place::Map||run.victory||run.defeat||option<0||option>=int(run.options.size()))return false;
    const int id=run.options[option];
    run.options.clear();
    run.node=run.index=id;run.map[id].visited=true;
    const auto& node=run.map[id];
    if(IsFightNode(node.kind)){
        run.sequence.clear();run.trial_limit=run.sequence_gold=0;run.sequence_relic=false;run.reward_multiplier=1;
        BeginFight(run,combat,node.encounter_id);
        return true;
    }
    if(node.kind==NodeKind::Event){
        const EventDef* found=nullptr;for(const auto& event:combat.config.events)if(event.id==node.event_id)found=&event;
        if(!found)throw std::runtime_error("Unknown event: "+node.event_id);
        run.place=Place::Event;run.event_title=found->title;run.event_text=found->text;run.event_choices=found->choices;return true;
    }
    if(node.kind==NodeKind::Shop){run.place=Place::Shop;FillShop(run,combat.config);return true;}
    if(node.kind==NodeKind::Rest){run.place=Place::Rest;run.rest_used=false;return true;}
    return false;
}

bool ChooseReward(RunState& run,Combat&,int index){
    if(run.place!=Place::Reward||!run.pending_reward||index<0||index>=int(run.rewards.size()))return false;
    run.deck.push_back(run.rewards[index]);run.rewards.clear();run.pending_reward=false;run.fight_resolved=false;ShowMap(run);return true;
}

bool ChooseRelicReward(RunState& run,const Config& config,int index){
    if(run.place!=Place::RelicReward||index<0||index>=int(run.relic_rewards.size()))return false;
    GiveRelic(run,config,run.relic_rewards[index]);run.relic_rewards.clear();run.fight_resolved=false;
    if(run.advance_act){++run.act;run.node=run.index=-1;run.map=GenerateMap(config,run.seed,run.act);}
    run.advance_act=false;ShowMap(run);return true;
}
bool ChooseSelectedCard(RunState& run,const Config& config,int index){
    if(run.place!=Place::CardSelect||index<0||index>=int(run.deck.size()))return false;
    if(run.selection=="upgrade"){const auto& card=FindCard(config,run.deck[index]);if(card.upgrade_id.empty())return false;run.deck[index]=card.upgrade_id;}
    else if(run.selection=="remove"){if(run.deck.size()<=5)return false;run.deck.erase(run.deck.begin()+index);}
    else return false;
    ShowMap(run);return true;
}
bool ChooseTradeRelic(RunState& run,const Config& config,int index){
    if(run.place!=Place::RelicSelect||index<0||index>=int(run.relics.size()))return false;
    const auto relic=FindRelic(config,run.relics[index]);run.relics.erase(run.relics.begin()+index);
    run.max_hp=std::max(1,run.max_hp-relic.max_hp);run.hp=std::min(run.hp,run.max_hp);run.potion_slots=std::max(1,run.potion_slots-relic.potion_slot);
    while(run.potions.size()>size_t(run.potion_slots)){run.potions.pop_back();run.gold+=10;}
    Rng rng;rng.Seed(run.seed^uint32_t(run.node+1+run.act*100)*0x5D91u);
    RandomRelic(run,config,rng,"common");RandomRelic(run,config,rng,"common");ShowMap(run);return true;
}
void CancelSelection(RunState& run){if(run.place==Place::CardSelect||run.place==Place::RelicSelect)ShowMap(run);}

bool CanChooseEvent(const RunState& run,int index){
    if(run.place!=Place::Event||run.defeat||index<0||index>=int(run.event_choices.size()))return false;
    const auto& choice=run.event_choices[index];
    return run.gold>=std::max(0,-choice.gold)&&(choice.action!="star_sacrifice"||run.max_hp>10)&&(choice.action!="select_trade"||!run.relics.empty())&&(choice.action!="select_remove"||run.deck.size()>5);
}

bool ChooseEvent(RunState& run,const Config& config,int index){
    if(!CanChooseEvent(run,index))return false;
    const auto choice=run.event_choices[index];
    run.last_outcome=choice.outcome;
    const bool paid=choice.gold<0?Pay(run,choice.gold):(choice.gold>0?(run.gold+=choice.gold,true):true);
    if(paid)HealTo(run,choice.heal);
    if(choice.damage>0)Hurt(run,choice.damage);
    if(choice.max_hp>0){run.max_hp+=choice.max_hp;HealTo(run,choice.max_hp);}
    if(paid&&!choice.card.empty())run.deck.push_back(choice.card);
    if(paid&&!choice.relic.empty())GiveRelic(run,config,choice.relic);
    if(paid&&!choice.potion.empty())GivePotion(run,choice.potion);
    if(choice.upgrade){const int which=UpgradeIndex(run,config);if(which>=0)run.deck[which]=FindCard(config,run.deck[which]).upgrade_id;else if(paid)HealTo(run,5);}
    Rng rng;rng.Seed(run.seed^uint32_t(run.node+run.act*101+1)*0xF13A735u);
    const auto& action=choice.action;
    if(action=="wish"){
        if(rng.Next()<choice.chance){RandomRelic(run,config,rng,"common");run.last_outcome="Điều ước được đáp lại. Bạn nhận một di vật.";}else run.last_outcome="Đồng xu chìm xuống. Giếng không đáp lời.";
    }else if(action=="select_upgrade"||action=="select_remove"){
        run.selection=action=="select_upgrade"?"upgrade":"remove";run.place=Place::CardSelect;return true;
    }else if(action=="select_trade"){run.place=Place::RelicSelect;return true;}
    else if(action=="forge_spear")run.bonuses["spear_bonus"]+=choice.amount;
    else if(action=="companion")run.bonuses["companion_heal"]+=choice.amount;
    else if(action=="star_sacrifice"){run.max_hp-=10;run.hp=std::min(run.hp,run.max_hp);run.bonuses["ultimate_bonus"]+=choice.amount;}
    else if(action=="start_rage")run.bonuses["start_rage"]+=choice.amount;
    else if(action=="pact"){++run.bonuses["energy"];run.bonuses["final_boss_hp"]+=25;}
    else if(action=="fog"){
        run.fog_until=run.map[run.node].floor+choice.amount;
        GivePotion(run,config.potions[std::min(int(config.potions.size())-1,int(rng.Next()*config.potions.size()))].id);
    }else if(action=="egg"||action=="random_card"||action=="rare_offer"){
        std::vector<std::string> pool;for(const auto& card:config.cards)if(Available(run,card)&&(action!="egg"||card.rarity=="legendary")&&(action!="rare_offer"||card.rarity=="rare"))pool.push_back(card.id);
        for(int i=int(pool.size())-1;i>0;--i)std::swap(pool[i],pool[int(rng.Next()*(i+1))]);
        if(action=="rare_offer"){run.rewards.assign(pool.begin(),pool.begin()+std::min(3,int(pool.size())));run.pending_reward=!run.rewards.empty();run.reward_from_event=true;run.place=Place::Reward;return true;}
        if(!pool.empty())run.deck.push_back(pool.front());if(action=="egg")run.deck.push_back("mud");
    }
    // Combat events are opened by the controller overload below, which owns Combat.
    if(!run.defeat)ShowMap(run);
    return true;
}

bool BuyItem(RunState& run,const Config& config,int index){
    if(run.place!=Place::Shop||index<0||index>=int(run.shop.size())||run.shop[index].sold)return false;
    auto& item=run.shop[index];
    if(run.gold<item.price)return false;
    if(item.ware==Ware::Potion&&int(run.potions.size())>=run.potion_slots)return false;
    if(item.ware==Ware::Relic&&Owned(run.relics,item.id))return false;
    run.gold-=item.price;item.sold=true;
    if(item.ware==Ware::Card)run.deck.push_back(item.id);
    else if(item.ware==Ware::Relic)GiveRelic(run,config,item.id);
    else GivePotion(run,item.id);
    return true;
}

bool ChooseEvent(RunState& run,Combat& combat,int index){
    if(!CanChooseEvent(run,index))return false;
    const auto choice=run.event_choices[index];
    if(!ChooseEvent(run,combat.config,index))return false;
    if(run.defeat)return true;
    if(choice.action!="bridge"&&choice.action!="detour"&&choice.action!="ambush"&&choice.action!="trial")return true;
    Rng rng;rng.Seed(run.seed^uint32_t(run.node+1+run.act*100)*0xB41D63u);const auto& chapter=combat.config.acts.at(run.act);
    auto regular=[&](){return chapter.combats[std::min(int(chapter.combats.size())-1,int(rng.Next()*chapter.combats.size()))];};
    std::string encounter=regular();run.sequence.clear();run.sequence_gold=0;run.sequence_relic=false;run.reward_multiplier=1;
    if(choice.action=="bridge"||choice.action=="detour")run.sequence_gold=combat.config.combat_gold_min+int(rng.Next()*(combat.config.combat_gold_max-combat.config.combat_gold_min+1));
    if(choice.action=="bridge"&&rng.Next()<choice.chance){encounter=chapter.elites[std::min(int(chapter.elites.size())-1,int(rng.Next()*chapter.elites.size()))];run.reward_multiplier=2;run.sequence_gold=2*(combat.config.elite_gold_min+int(rng.Next()*(combat.config.elite_gold_max-combat.config.elite_gold_min+1)));}
    else if(choice.action=="ambush"){run.sequence.push_back(regular());run.sequence_gold=35;run.sequence_relic=true;}
    else if(choice.action=="trial"){encounter="circus_dummy";run.trial_limit=choice.amount;}
    BeginFight(run,combat,encounter);return true;
}

bool RemoveCard(RunState& run,const Config& config,int index){
    if(run.place!=Place::Shop||index<0||index>=int(run.deck.size())||int(run.deck.size())<=5)return false;
    if(run.gold<config.remove_price)return false;
    run.gold-=config.remove_price;run.deck.erase(run.deck.begin()+index);return true;
}

void LeaveShop(RunState& run){if(run.place==Place::Shop)ShowMap(run);}

int RestHealAmount(const RunState& run,const Config& config){
    int bonus=RunBonus(run,"rest_bonus"),percent=config.heal_percent;for(const auto& id:run.relics){bonus+=FindRelic(config,id).rest_bonus;percent+=FindRelic(config,id).rest_percent;}
    const auto penalties=ChallengeBonuses(config,run.challenge);const auto found=penalties.find("rest_penalty");if(found!=penalties.end())percent-=found->second;
    return std::min(run.max_hp-run.hp,std::max(1,run.max_hp*std::max(0,percent)/100)+bonus);
}
bool RestHeal(RunState& run,const Config& config){
    if(run.place!=Place::Rest||run.rest_used)return false;
    HealTo(run,RestHealAmount(run,config));
    run.rest_used=true;ShowMap(run);return true;
}

bool UpgradeCard(RunState& run,const Config& config,int index){
    if(run.place!=Place::Rest||run.rest_used||index<0||index>=int(run.deck.size()))return false;
    const auto& card=FindCard(config,run.deck[index]);
    if(card.upgraded||card.upgrade_id.empty())return false;
    run.deck[index]=card.upgrade_id;run.rest_used=true;ShowMap(run);return true;
}

bool DrinkPotion(RunState& run,Combat& combat,int index){
    if(run.place!=Place::Fight||combat.phase!=Phase::Player||index<0||index>=int(run.potions.size()))return false;
    const auto potion=FindPotion(combat.config,run.potions[index]);
    run.potions.erase(run.potions.begin()+index);
    ++run.metrics["potions"];
    combat.Cast(potion.effects);
    if(combat.phase==Phase::Won||combat.phase==Phase::Lost)ObserveCombat(run,combat);
    return true;
}

int SuggestCard(const Combat& combat){
    if(combat.phase!=Phase::Player)return -1;
    const int incoming=combat.IncomingDamage();
    int best=-1;double score=-1;
    for(int i=0;i<int(combat.hand.size());++i){
        if(!combat.CanPlay(i))continue;
        const auto& card=combat.Definition(combat.hand[i]);
        double value=card.damage;
        if(card.damage>0&&card.damage>=combat.enemy.hp+combat.enemy.block)value+=80;
        if(card.block>0){const int need=std::max(0,incoming-combat.player.block);value+=std::min(card.block,need)*1.4+(need>0?2.0:.4);if(need>=combat.player.hp)value+=120;}
        if(card.heal>0){const int missing=combat.player.max_hp-combat.player.hp;value+=std::min(card.heal,missing)*(combat.player.hp*2<combat.player.max_hp?2.4:.55);}
        value+=card.poison*(combat.PhaseCount()>1?3.2:2.2);
        value+=card.vulnerable*(combat.enemy.status.vulnerable>0?1.0:3.4);
        value+=card.strength*4.2;value+=card.weak*(incoming>0?2.4:.8);value+=card.cards_drawn*2.1;value+=card.energy_gain*2.0;
        for(const auto& effect:card.effects){
            if(effect.op==EffectOp::DoubleAttack)value+=combat.live.double_attack==0?16:0;
            if(effect.op==EffectOp::EndBlock)value+=effect.amount*2.5;
            if(effect.op==EffectOp::Revive)value+=!combat.live.revive_ready&&combat.live.revives==0?10:0;
            if(effect.op==EffectOp::UltDiscount)value+=combat.live.ult_discount==0?8:1;
            if(effect.op==EffectOp::Retain)value+=std::max(0,combat.player.block-incoming)*.5;
            if(effect.op==EffectOp::Thorns)value+=incoming>0?effect.amount*1.5:0;
            if(effect.op==EffectOp::NextEnergy)value+=3;
            if(effect.op==EffectOp::Rage)value+=std::min(effect.amount,combat.UltimateCost()-combat.rage)*.25;
        }
        if(card.cost==0)value+=1.2;value/=std::max(1,card.cost);
        if(value>score){score=value;best=i;}
    }
    return score>0?best:-1;
}

int SuggestReward(const RunState& run,const Config& config){
    int best=0;double score=-1;
    for(int i=0;i<int(run.rewards.size());++i){
        double value=CardValue(FindCard(config,run.rewards[i]));
        if(value>score){score=value;best=i;}
    }
    return best;
}

AutoStep SuggestAuto(const RunState& run,const Combat& combat){
    if(run.victory||run.defeat)return {};
    if(run.place==Place::Map){
        int best=0,score=-999;
        for(int i=0;i<int(run.options.size());++i){const int value=NodeScore(run,run.map[run.options[i]])-run.map[run.options[i]].column;if(value>score){score=value;best=i;}}
        return {AutoKind::Node,best};
    }
    if(run.place==Place::Reward)return {AutoKind::Reward,SuggestReward(run,combat.config)};
    if(run.place==Place::RelicReward)return {AutoKind::Relic,0};
    if(run.place==Place::RelicSelect)return {AutoKind::TradeRelic,0};
    if(run.place==Place::CardSelect){int card=run.selection=="upgrade"?UpgradeIndex(run,combat.config):0;return {AutoKind::SelectCard,card};}
    if(run.place==Place::Event){
        int best=0,score=-999;
        for(int i=0;i<int(run.event_choices.size());++i){if(!CanChooseEvent(run,i))continue;const int value=ChoiceScore(run,run.event_choices[i],combat.config);if(value>score){score=value;best=i;}}
        return {AutoKind::Event,best};
    }
    if(run.place==Place::Shop){
        int best=-1,score=0;
        for(int i=0;i<int(run.shop.size());++i){
            const auto& item=run.shop[i];
            if(item.sold||item.price>run.gold)continue;
            int value=0;
            if(item.ware==Ware::Potion){const int heal=PotionHeal(FindPotion(combat.config,item.id));if(heal>0&&run.hp*100<run.max_hp*80)value=40;else if(heal==0&&run.hp*100>run.max_hp*50)value=12;}
            else if(item.ware==Ware::Relic){const auto& relic=FindRelic(combat.config,item.id);value=relic.bonus_damage*18+relic.rest_bonus+relic.gold_bonus+(relic.combat_start.empty()?0:16)+(relic.max_hp>0?14:0);if(item.price>run.gold-10)value=0;}
            else value=CardValue(FindCard(combat.config,item.id))/8;
            if(item.ware!=Ware::Potion&&run.gold-item.price<20)value=0;
            if(item.ware==Ware::Potion&&int(run.potions.size())>=run.potion_slots)value=0;
            if(value>score){score=value;best=i;}
        }
        if(best>=0)return {AutoKind::Buy,best};
        if(run.gold>=combat.config.remove_price&&int(run.deck.size())>8&&CountId(run.deck,"punch")>=4){
            for(int i=0;i<int(run.deck.size());++i)if(run.deck[i]=="punch")return {AutoKind::Remove,i};
        }
        return {AutoKind::Leave,0};
    }
    if(run.place==Place::Rest){
        if(run.hp*100<run.max_hp*72)return {AutoKind::Heal,0};
        const int which=UpgradeIndex(run,combat.config);
        if(which>=0)return {AutoKind::Upgrade,which};
        return {AutoKind::Heal,0};
    }
    if(combat.phase!=Phase::Player)return {};
    const auto targets=combat.Targets();if(!targets.empty()){const int wanted=targets.size()>1?targets.back():targets.front();if(combat.live.target!=wanted)return {AutoKind::Target,wanted};}
    for(int i=0;i<int(run.potions.size());++i){
        const auto& potion=FindPotion(combat.config,run.potions[i]);
        if(PotionHeal(potion)>0&&(combat.player.hp*2<combat.player.max_hp||(combat.IncomingDamage()>=combat.player.hp&&combat.player.block<combat.IncomingDamage())))return {AutoKind::Potion,i};
        if(PotionHarm(potion)>0&&combat.enemy.hp>18&&combat.player.hp>16)return {AutoKind::Potion,i};
        for(const auto& effect:potion.effects){
            if(effect.op==EffectOp::Block&&combat.IncomingDamage()>combat.player.block+3)return {AutoKind::Potion,i};
            if(effect.op==EffectOp::Rage&&combat.rage<combat.config.balance.rage_max&&combat.rage+effect.amount>=combat.config.balance.rage_max&&combat.enemy.hp>20)return {AutoKind::Potion,i};
            if(effect.op==EffectOp::Energy&&combat.energy<2&&!combat.hand.empty())return {AutoKind::Potion,i};
            if(effect.op==EffectOp::Draw&&combat.energy>0&&combat.hand.size()<3)return {AutoKind::Potion,i};
        }
    }
    if(combat.CanUltimate())return {AutoKind::Ultimate,0};
    const int card=SuggestCard(combat);
    if(card>=0)return {AutoKind::Play,card};
    return {AutoKind::EndTurn,0};
}
}
