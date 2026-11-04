#include "quest_save.hpp"
#include "../third_party/nlohmann/json.hpp"
#include <fstream>
#include <set>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

namespace undergod {
namespace {
using Json=nlohmann::json;
void Require(bool valid,const char* message){if(!valid)throw std::runtime_error(message);}
int Integer(const Json& object,const char* key,int low,int high){
    const auto& value=object.at(key);
    Require(value.is_number_integer(),"Save needs integer values");
    const auto number=value.get<int64_t>();
    Require(number>=low&&number<=high,"Saved number is out of range");
    return int(number);
}
uint32_t Unsigned(const Json& object,const char* key){
    const auto& value=object.at(key);
    Require(value.is_number_integer(),"Invalid RNG state");
    const auto number=value.get<int64_t>();
    Require(number>=0&&number<=UINT32_MAX,"Invalid RNG state");
    return uint32_t(number);
}
std::vector<std::string> Ids(const Json& object,const char* key,size_t maximum){
    const auto& list=object.at(key);
    Require(list.is_array()&&list.size()<=maximum,"Invalid saved inventory size");
    return list.get<std::vector<std::string>>();
}
Json FighterJson(const Fighter& fighter){
    return {{"hp",fighter.hp},{"maxHp",fighter.max_hp},{"block",fighter.block},{"poison",fighter.status.poison},{"weak",fighter.status.weak},{"vulnerable",fighter.status.vulnerable},{"strength",fighter.status.strength}};
}
Fighter ReadFighter(const Json& j){
    Fighter f;
    f.max_hp=Integer(j,"maxHp",1,100000);f.hp=Integer(j,"hp",0,f.max_hp);f.block=Integer(j,"block",0,100000);
    f.status={Integer(j,"poison",0,10000),Integer(j,"weak",0,10000),Integer(j,"vulnerable",0,10000),Integer(j,"strength",0,10000)};
    return f;
}
Json CombatJson(const CombatSnapshot& s){
    return {{"enemyId",s.enemy_id},{"player",FighterJson(s.player)},{"enemy",FighterJson(s.enemy)},
        {"phase",int(s.phase)},{"intent",{{"attack",s.intent.attack},{"value",s.intent.value},{"kind",int(s.intent.kind)},{"status",int(s.intent.status)},{"amount",s.intent.status_amount}}},
        {"energy",s.energy},{"rage",s.rage},{"turn",s.turn},{"guardStreak",s.guard_streak},{"phaseIndex",s.phase_index},
        {"shuffleRng",s.shuffle_rng},{"intentRng",s.intent_rng},{"phaseOpening",s.phase_opening},{"draw",s.draw},{"hand",s.hand},{"discard",s.discard}};
}
CombatSnapshot ReadCombat(const Json& j){
    CombatSnapshot s;
    s.enemy_id=j.at("enemyId").get<std::string>();s.player=ReadFighter(j.at("player"));s.enemy=ReadFighter(j.at("enemy"));
    s.phase=Phase(Integer(j,"phase",0,int(Phase::Lost)));
    const auto& intent=j.at("intent");
    s.intent={intent.at("attack").get<bool>(),Integer(intent,"value",0,100000),IntentKind(Integer(intent,"kind",0,int(IntentKind::Debuff))),StatusId(Integer(intent,"status",0,int(StatusId::Strength))),Integer(intent,"amount",0,10000)};
    s.energy=Integer(j,"energy",0,100);s.rage=Integer(j,"rage",0,100000);s.turn=Integer(j,"turn",1,100000);
    s.guard_streak=Integer(j,"guardStreak",0,100000);s.phase_index=Integer(j,"phaseIndex",0,100);
    s.shuffle_rng=Unsigned(j,"shuffleRng");s.intent_rng=Unsigned(j,"intentRng");s.phase_opening=j.at("phaseOpening").get<bool>();
    s.draw=Ids(j,"draw",1000);s.hand=Ids(j,"hand",1000);s.discard=Ids(j,"discard",1000);
    return s;
}
Json NodeJson(const MapNode& node){
    return {{"kind",int(node.kind)},{"floor",node.floor},{"column",node.column},{"enemy",node.encounter_id},{"event",node.event_id},{"gold",node.reward_gold},{"next",node.next}};
}
Json RunJson(const RunState& run){
    Json nodes=Json::array(),shop=Json::array();
    for(const auto& node:run.map){auto saved=NodeJson(node);saved["visited"]=node.visited;nodes.push_back(saved);}
    for(const auto& item:run.shop)shop.push_back({{"ware",int(item.ware)},{"id",item.id},{"price",item.price},{"sold",item.sold}});
    return {{"seed",run.seed},{"node",run.node},{"hp",run.hp},{"maxHp",run.max_hp},{"rage",run.rage},{"gold",run.gold},{"potionSlots",run.potion_slots},
        {"class",run.class_id},{"deck",run.deck},{"relics",run.relics},{"potions",run.potions},{"rewards",run.rewards},{"map",nodes},{"shop",shop},
        {"place",int(run.place)},{"victory",run.victory},{"defeat",run.defeat},{"fightResolved",run.fight_resolved},{"restUsed",run.rest_used}};
}
RunState ReadRun(const Json& j,const Config& config){
    RunState run;
    run.seed=Unsigned(j,"seed");run.map=GenerateMap(config,run.seed);
    run.node=run.index=Integer(j,"node",-1,int(run.map.size())-1);
    run.max_hp=Integer(j,"maxHp",1,100000);run.hp=Integer(j,"hp",0,run.max_hp);run.rage=Integer(j,"rage",0,config.balance.rage_max);
    run.gold=Integer(j,"gold",0,1000000);run.potion_slots=Integer(j,"potionSlots",1,4);
    run.class_id=j.at("class").get<std::string>();Require(run.class_id==config.player_id,"Unknown saved class");
    run.deck=Ids(j,"deck",1000);Require(!run.deck.empty(),"Empty saved deck");
    for(const auto& id:run.deck)FindCard(config,id);
    run.relics=Ids(j,"relics",config.relics.size());run.potions=Ids(j,"potions",run.potion_slots);run.rewards=Ids(j,"rewards",config.reward_choices);
    std::set<std::string> relic_ids;for(const auto& id:run.relics){FindRelic(config,id);Require(relic_ids.insert(id).second,"Duplicate saved relic");}
    for(const auto& id:run.potions)FindPotion(config,id);
    std::set<std::string> offered;for(const auto& id:run.rewards){const auto& card=FindCard(config,id);Require(offered.insert(id).second&&!card.upgraded&&card.rarity!="starter","Invalid saved card offer");}
    run.place=Place(Integer(j,"place",0,int(Place::Rest)));run.pending_reward=run.place==Place::Reward;
    run.victory=j.at("victory").get<bool>();run.defeat=j.at("defeat").get<bool>();
    run.fight_resolved=j.at("fightResolved").get<bool>();run.rest_used=j.at("restUsed").get<bool>();
    Require(!(run.victory&&run.defeat)&&run.defeat==(run.hp==0),"Invalid saved run result");
    Require(j.at("map").is_array()&&j.at("map").size()==run.map.size(),"Saved map has a different layout");
    int previous=-1;
    for(int i=0;i<int(run.map.size());++i){
        auto layout=j.at("map").at(i);run.map[i].visited=layout.at("visited").get<bool>();layout.erase("visited");
        Require(layout==NodeJson(run.map[i]),"Map data changed since this save");
        if(!run.map[i].visited)continue;
        if(previous<0)Require(run.map[i].floor==0,"Saved path has no entrance");
        else Require(std::find(run.map[previous].next.begin(),run.map[previous].next.end(),i)!=run.map[previous].next.end(),"Invalid saved map path");
        previous=i;
    }
    Require(previous==run.node,"Saved current node does not match the path");
    if(run.node<0){Require(run.place==Place::Map&&!run.victory&&!run.defeat,"Invalid saved entrance");for(int i=0;i<int(run.map.size());++i)if(run.map[i].floor==0)run.options.push_back(i);}
    else{
        const auto& node=run.map[run.node];run.options=node.next;
        Require((run.place==Place::Map)||(IsFightNode(node.kind)&&(run.place==Place::Fight||run.place==Place::Reward))||(node.kind==NodeKind::Event&&run.place==Place::Event)||(node.kind==NodeKind::Shop&&run.place==Place::Shop)||(node.kind==NodeKind::Rest&&run.place==Place::Rest),"Saved screen does not match the node");
        if(run.place==Place::Event){for(const auto& event:config.events)if(event.id==node.event_id){run.event_title=event.title;run.event_text=event.text;run.event_choices=event.choices;}}
        Require(!run.victory||node.kind==NodeKind::Boss,"Saved victory is not at the boss");
    }
    Require(run.pending_reward==!run.rewards.empty(),"Invalid saved rewards");
    Require(run.place!=Place::Map||(!run.victory&&!run.defeat&&(run.node<0||run.map[run.node].kind!=NodeKind::Boss)),"Completed boss cannot return to the map");
    Require(j.at("shop").is_array()&&j.at("shop").size()<=7,"Invalid saved shop");
    for(const auto& item:j.at("shop")){
        ShopItem ware;ware.ware=Ware(Integer(item,"ware",0,int(Ware::Potion)));ware.id=item.at("id").get<std::string>();
        ware.price=Integer(item,"price",1,100000);ware.sold=item.at("sold").get<bool>();
        if(ware.ware==Ware::Card)FindCard(config,ware.id);else if(ware.ware==Ware::Relic)FindRelic(config,ware.id);else FindPotion(config,ware.id);
        run.shop.push_back(ware);
    }
    return run;
}
void ValidatePair(const RunState& run,const Combat& combat){
    const auto s=combat.Snapshot();
    if(run.place==Place::Fight||run.place==Place::Reward){
        Require(run.node>=0&&run.map[run.node].encounter_id==s.enemy_id,"Combat does not match the current map node");
        Require(run.max_hp==s.player.max_hp,"Saved player maximum HP differs");
        std::multiset<std::string> cards(s.draw.begin(),s.draw.end());cards.insert(s.hand.begin(),s.hand.end());cards.insert(s.discard.begin(),s.discard.end());
        Require(cards==std::multiset<std::string>(run.deck.begin(),run.deck.end()),"Saved combat deck is not conserved");
        if(run.place==Place::Reward)Require(s.phase==Phase::Won&&run.fight_resolved,"Rewards without a resolved victory");
        if(run.place==Place::Fight&&!run.victory&&!run.defeat)Require(s.phase==Phase::Player&&!run.fight_resolved,"Finished combat has no run result");
        if(run.victory)Require(s.phase==Phase::Won,"Run victory without a combat victory");
        if(run.defeat)Require(s.phase==Phase::Lost,"Run defeat without a combat defeat");
        if(run.fight_resolved)Require(run.hp==s.player.hp&&run.rage==s.rage,"Resolved combat resources were not banked");
    }
}
void ReplaceFile(const std::filesystem::path& temporary,const std::filesystem::path& file){
#ifdef _WIN32
    if(!MoveFileExW(temporary.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot replace save file");
#else
    std::filesystem::rename(temporary,file);
#endif
}
}

bool SaveSession(const std::filesystem::path& file,const RunState& run,const Combat& combat,int speed,std::string& error){
    auto temporary=file;temporary+=".tmp";
    try{
        Require(!file.empty()&&speed>=1&&speed<=3,"Invalid save path or speed");
        const auto snapshot=combat.Snapshot();
        Require(snapshot.phase==Phase::Player||snapshot.phase==Phase::Won||snapshot.phase==Phase::Lost,"Cannot save during an action");
        Json data={{"format","undergod-card"},{"version",1},{"speed",speed},{"run",RunJson(run)},{"combat",CombatJson(snapshot)}};
        // Validate before replacing an existing checkpoint.
        auto checked_run=ReadRun(data.at("run"),combat.config);Combat checked_combat(combat.config);checked_combat.Restore(snapshot,ModsFor(checked_run,combat.config));ValidatePair(checked_run,checked_combat);
        if(file.has_parent_path())std::filesystem::create_directories(file.parent_path());
        std::ofstream output(temporary,std::ios::binary|std::ios::trunc);
        if(!output)throw std::runtime_error("Cannot open temporary save");
        output<<data.dump(2)<<'\n';output.flush();if(!output)throw std::runtime_error("Cannot write save");output.close();if(!output)throw std::runtime_error("Cannot close save");
        ReplaceFile(temporary,file);error.clear();return true;
    }catch(const std::exception& e){error=e.what();std::error_code ignored;std::filesystem::remove(temporary,ignored);return false;}
}

bool LoadSession(const std::filesystem::path& file,RunState& run,Combat& combat,int& speed,std::string& error){
    try{
        Require(std::filesystem::is_regular_file(file)&&std::filesystem::file_size(file)<=1024*1024,"Save missing or too large");
        std::ifstream input(file,std::ios::binary);Json data;input>>data;
        Require(data.at("format")=="undergod-card"&&Integer(data,"version",1,1)==1,"Unknown save format");
        const int next_speed=Integer(data,"speed",1,3);
        auto next_run=ReadRun(data.at("run"),combat.config);Combat next_combat(combat.config);
        next_combat.Restore(ReadCombat(data.at("combat")),ModsFor(next_run,combat.config));ValidatePair(next_run,next_combat);
        run=std::move(next_run);combat=std::move(next_combat);speed=next_speed;error.clear();return true;
    }catch(const std::exception& e){error=e.what();return false;}
}
}
