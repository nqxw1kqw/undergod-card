#include "quest_run.hpp"

namespace undergod {
namespace {
int RarityWeight(const std::string& rarity){return rarity=="rare"?1:(rarity=="uncommon"?3:6);}
bool Rewardable(const CardDef& card){return !card.upgraded&&card.rarity!="starter"&&card.copies<=0;}
int CardPrice(const CardDef& card){return card.rarity=="rare"?110:(card.rarity=="uncommon"?75:50);}
int CountId(const std::vector<std::string>& ids,const std::string& id){int n=0;for(const auto& have:ids)if(have==id)++n;return n;}
bool Owned(const std::vector<std::string>& ids,const std::string& id){return CountId(ids,id)>0;}
int Discount(const RunState& run,const Config& config){int discount=0;for(const auto& id:run.relics)discount+=FindRelic(config,id).shop_discount;return std::min(40,discount);}
int PriceFor(const RunState& run,const Config& config,int base){return std::max(1,base*(100-Discount(run,config))/100);}
void ShowMap(RunState& run){
    run.place=Place::Map;run.pending_reward=false;run.rest_used=false;run.options.clear();
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
    std::vector<const CardDef*> pool;for(const auto& card:config.cards)if(Rewardable(card))pool.push_back(&card);
    Rng rng;rng.Seed(run.seed^0xC0FFEEu^(uint32_t(run.node+1)*0x45D9F3Bu));
    run.rewards.clear();
    for(int n=0;n<std::min(config.reward_choices,int(pool.size()));++n){
        int total=0;for(const auto* card:pool)total+=RarityWeight(card->rarity);
        const double roll=rng.Next()*std::max(1,total);int acc=0,index=int(pool.size())-1;
        for(int i=0;i<int(pool.size());++i){acc+=RarityWeight(pool[i]->rarity);if(roll<acc){index=i;break;}}
        run.rewards.push_back(pool[index]->id);pool.erase(pool.begin()+index);
    }
    run.pending_reward=!run.rewards.empty();
    if(run.pending_reward)run.place=Place::Reward;else ShowMap(run);
}
void FillShop(RunState& run,const Config& config){
    Rng rng;rng.Seed(run.seed^0x5110ABu^(uint32_t(run.node+3)*0x9E3779B9u));
    std::vector<const CardDef*> cards;std::vector<const RelicDef*> relics;std::vector<const PotionDef*> potions;
    for(const auto& card:config.cards)if(Rewardable(card))cards.push_back(&card);
    for(const auto& relic:config.relics)if(!Owned(run.relics,relic.id))relics.push_back(&relic);
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
    for(const auto& id:run.relics){
        const auto& relic=FindRelic(config,id);
        mods.bonus_damage+=relic.bonus_damage;mods.bonus_block+=relic.bonus_block;
        mods.combat_start.insert(mods.combat_start.end(),relic.combat_start.begin(),relic.combat_start.end());
        mods.turn_start.insert(mods.turn_start.end(),relic.turn_start.begin(),relic.turn_start.end());
    }
    return mods;
}

std::vector<MapNode> GenerateMap(const Config& config,unsigned seed){
    const int columns=std::max(1,config.map_columns),floors=std::max(1,config.map_floors);
    const NodeKind pattern[5][3]={
        {NodeKind::Combat,NodeKind::Combat,NodeKind::Combat},
        {NodeKind::Event,NodeKind::Combat,NodeKind::Shop},
        {NodeKind::Rest,NodeKind::Event,NodeKind::Combat},
        {NodeKind::Combat,NodeKind::Elite,NodeKind::Rest},
        {NodeKind::Shop,NodeKind::Event,NodeKind::Rest}
    };
    std::vector<int> event_order(config.events.size());
    for(int i=0;i<int(event_order.size());++i)event_order[i]=i;
    Rng rng;rng.Seed(seed^0xE11Eu);
    for(int i=int(event_order.size())-1;i>0;--i)std::swap(event_order[i],event_order[int(rng.Next()*(i+1))]);
    int event_cursor=0;
    std::vector<MapNode> map;
    for(int floor=0;floor<floors;++floor)for(int column=0;column<columns;++column){
        MapNode node;node.floor=floor;node.column=column;node.kind=floor<5&&column<3?pattern[floor][column]:NodeKind::Combat;
        if(node.kind==NodeKind::Combat){
            node.encounter_id=config.combat_ids.empty()?"froglet":(floor==0?config.combat_ids.front():config.combat_ids[column%int(config.combat_ids.size())]);
            node.reward_gold=node.encounter_id=="toad"?18:15;
        }else if(node.kind==NodeKind::Elite){node.encounter_id=config.elite_id;node.reward_gold=32;}
        else if(node.kind==NodeKind::Event&&!config.events.empty()){node.event_id=config.events[event_order[event_cursor%int(event_order.size())]].id;++event_cursor;}
        map.push_back(node);
    }
    auto at=[&](int floor,int column){return floor*columns+column;};
    for(int floor=0;floor<floors-1;++floor)for(int column=0;column<columns;++column)for(int delta=-1;delta<=1;++delta){
        const int next=column+delta;if(next>=0&&next<columns)map[at(floor,column)].next.push_back(at(floor+1,next));
    }
    MapNode boss;boss.kind=NodeKind::Boss;boss.floor=floors;boss.column=columns/2;boss.encounter_id=config.boss_id;
    const int boss_index=int(map.size());
    for(int column=0;column<columns;++column)map[at(floors-1,column)].next.push_back(boss_index);
    map.push_back(boss);
    return map;
}

void StartRun(RunState& run,Combat& combat,unsigned seed){
    run=RunState{};
    run.seed=seed;run.hp=run.max_hp=combat.config.balance.player_hp;run.gold=combat.config.gold_start;run.potion_slots=combat.config.potion_slots;
    run.class_id=combat.config.player_id;run.deck=StartingDeck(combat.config);run.map=GenerateMap(combat.config,seed);
    if(run.map.empty())throw std::runtime_error("Run map is empty");
    ShowMap(run);
}

std::string NextEncounterName(const RunState& run,const Config& config){
    for(int option:run.options)if(IsFightNode(run.map[option].kind))return FindEnemy(config,run.map[option].encounter_id).name;
    return "";
}

void ObserveCombat(RunState& run,Combat& combat){
    if(run.fight_resolved||run.victory||run.defeat||run.place!=Place::Fight)return;
    if(combat.phase!=Phase::Won&&combat.phase!=Phase::Lost)return;
    run.fight_resolved=true;
    if(combat.phase==Phase::Lost){run.defeat=true;return;}
    run.hp=combat.player.hp;run.rage=combat.rage;
    if(run.node>=0&&run.map[run.node].kind==NodeKind::Boss){run.victory=true;return;}
    int bonus=0;for(const auto& id:run.relics)bonus+=FindRelic(combat.config,id).gold_bonus;
    if(run.node>=0)run.gold+=std::max(0,run.map[run.node].reward_gold+bonus);
    OfferRewards(run,combat.config);
}

bool ChooseNode(RunState& run,Combat& combat,int option){
    if(run.place!=Place::Map||run.victory||run.defeat||option<0||option>=int(run.options.size()))return false;
    const int id=run.options[option];
    run.node=run.index=id;run.map[id].visited=true;
    const auto& node=run.map[id];
    if(IsFightNode(node.kind)){
        run.place=Place::Fight;run.fight_resolved=false;run.pending_reward=false;
        combat.BeginEncounter(FindEnemy(combat.config,node.encounter_id),EncounterSeed(run.seed,id),run.hp,run.max_hp,run.rage,run.deck,ModsFor(run,combat.config));
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

bool ChooseEvent(RunState& run,const Config& config,int index){
    if(run.place!=Place::Event||index<0||index>=int(run.event_choices.size())||run.defeat)return false;
    const auto choice=run.event_choices[index];
    const bool paid=choice.gold<0?Pay(run,choice.gold):(choice.gold>0?(run.gold+=choice.gold,true):true);
    if(paid)HealTo(run,choice.heal);
    if(choice.damage>0)Hurt(run,choice.damage);
    if(choice.max_hp>0){run.max_hp+=choice.max_hp;HealTo(run,choice.max_hp);}
    if(paid&&!choice.card.empty())run.deck.push_back(choice.card);
    if(paid&&!choice.relic.empty())GiveRelic(run,config,choice.relic);
    if(paid&&!choice.potion.empty())GivePotion(run,choice.potion);
    if(choice.upgrade){const int which=UpgradeIndex(run,config);if(which>=0)run.deck[which]=FindCard(config,run.deck[which]).upgrade_id;else if(paid)HealTo(run,5);}
    run.event_choices.clear();
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

bool RemoveCard(RunState& run,const Config& config,int index){
    if(run.place!=Place::Shop||index<0||index>=int(run.deck.size())||int(run.deck.size())<=5)return false;
    if(run.gold<config.remove_price)return false;
    run.gold-=config.remove_price;run.deck.erase(run.deck.begin()+index);return true;
}

void LeaveShop(RunState& run){if(run.place==Place::Shop)ShowMap(run);}

bool RestHeal(RunState& run,const Config& config){
    if(run.place!=Place::Rest||run.rest_used)return false;
    int bonus=0;for(const auto& id:run.relics)bonus+=FindRelic(config,id).rest_bonus;
    HealTo(run,std::max(8,run.max_hp*config.heal_percent/100)+bonus);
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
    if(run.place==Place::Event){
        int best=0,score=-999;
        for(int i=0;i<int(run.event_choices.size());++i){const int value=ChoiceScore(run,run.event_choices[i],combat.config);if(value>score){score=value;best=i;}}
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
    for(int i=0;i<int(run.potions.size());++i){
        const auto& potion=FindPotion(combat.config,run.potions[i]);
        if(PotionHeal(potion)>0&&(combat.player.hp*2<combat.player.max_hp||(combat.IncomingDamage()>=combat.player.hp&&combat.player.block<combat.IncomingDamage())))return {AutoKind::Potion,i};
        if(PotionHarm(potion)>0&&combat.enemy.hp>18&&combat.player.hp>16)return {AutoKind::Potion,i};
    }
    if(combat.rage>=combat.config.balance.rage_max&&combat.enemy.hp+combat.enemy.block>14)return {AutoKind::Ultimate,0};
    const int card=SuggestCard(combat);
    if(card>=0)return {AutoKind::Play,card};
    return {AutoKind::EndTurn,0};
}
}
