#include "quest_save.hpp"
#include "quest_json.hpp"
#include "quest_storage.hpp"
#include <fstream>
#include <set>

namespace undergod {
namespace {
using Json=nlohmann::json;
void Require(bool valid,const char* message){if(!valid)throw std::runtime_error(message);}
void NumericBounds(const Json& j,const std::string& key=""){
    if(j.is_object()){for(auto it=j.begin();it!=j.end();++it)NumericBounds(it.value(),it.key());return;}
    if(j.is_array()){Require(j.size()<=2000,"Saved collection too large");for(const auto& value:j)NumericBounds(value,key);return;}
    if(j.is_number_float()){Require(key=="chance"&&j.get<double>()>=0&&j.get<double>()<=1,"Non-integral saved number");return;}
    if(j.is_number_integer()){
        const auto value=j.get<int64_t>();const bool rng=key=="seed"||key.find("rng")!=std::string::npos;
        Require(value>=(rng?0:-1)&&value<=(rng?int64_t(UINT32_MAX):int64_t(100000000)),"Saved number out of range");
    }
    if(j.is_string())Require(j.get_ref<const std::string&>().size()<=4096,"Saved string too long");
}
void ValidateRun(RunState& run,const Config& config){
    Require(run.act>=0&&run.act<int(config.acts.size())&&run.challenge>=0&&run.challenge<=10,"Invalid act or difficulty");
    FindClass(config,run.class_id);
    Require(run.max_hp>=1&&run.max_hp<=10000&&run.hp>=0&&run.hp<=run.max_hp&&run.rage>=0&&run.rage<=config.balance.rage_max&&run.gold>=0&&run.gold<=1000000&&run.potion_slots>=1&&run.potion_slots<=4,"Invalid saved resources");
    Require(!(run.victory&&run.defeat)&&run.defeat==(run.hp==0),"Invalid saved result");
    Require(int(run.place)>=0&&int(run.place)<=int(Place::RelicSelect)&&run.deck.size()>=1&&run.deck.size()<=1000&&run.potions.size()<=size_t(run.potion_slots)&&run.rewards.size()<=3,"Invalid saved inventory");
    Require(run.run_id.size()<=128&&run.metrics.size()<=50&&run.bonuses.size()<=50,"Invalid run metadata");
    for(const auto& id:run.deck)FindCard(config,id);for(const auto& id:run.potions)FindPotion(config,id);
    std::set<std::string> owned;for(const auto& id:run.relics){FindRelic(config,id);Require(owned.insert(id).second,"Duplicate relic");}
    owned.clear();for(const auto& id:run.rewards){const auto& card=FindCard(config,id);Require(!card.upgraded&&card.rarity!="starter"&&card.rarity!="curse"&&owned.insert(id).second,"Invalid saved card offer");}
    for(const auto& id:run.relic_rewards)FindRelic(config,id);
    for(const auto& id:run.skills){bool known=false;for(const auto& skill:config.skills)known|=skill.id==id;Require(known,"Unknown saved skill");}
    for(const auto& id:run.sequence)FindEnemy(config,id);for(const auto& id:run.defeated_bosses)Require(FindEnemy(config,id).role=="boss","Invalid defeated boss");
    const auto expected=GenerateMap(config,run.seed,run.act);Require(expected.size()==run.map.size(),"Map layout changed");
    Require(run.node>=-1&&run.node<int(run.map.size())&&run.index==run.node,"Invalid map cursor");
    int previous=-1;
    for(int i=0;i<int(run.map.size());++i){const auto& e=expected[i];const auto& n=run.map[i];
        const auto& chapter=config.acts.at(run.act);
        const bool old_boss_choice=e.kind==NodeKind::Boss&&e.floor==(run.act==0?config.map_floors:chapter.floors)&&std::find(chapter.bosses.begin(),chapter.bosses.end(),n.encounter_id)!=chapter.bosses.end();
        Require(e.kind==n.kind&&e.floor==n.floor&&e.column==n.column&&(e.encounter_id==n.encounter_id||old_boss_choice)&&e.event_id==n.event_id&&e.reward_gold==n.reward_gold&&e.next==n.next,"Map data changed");
        if(!n.visited)continue;
        if(previous<0)Require(n.floor==0,"Saved path has no entrance");else Require(std::find(run.map[previous].next.begin(),run.map[previous].next.end(),i)!=run.map[previous].next.end(),"Invalid saved path");previous=i;
    }
    Require(previous==run.node,"Cursor does not match visited path");
    run.options.clear();run.event_choices.clear();run.event_title.clear();run.event_text.clear();
    if(run.node<0){Require(run.place==Place::Map&&!run.victory&&!run.defeat,"Invalid map entrance");for(int i=0;i<int(run.map.size());++i)if(run.map[i].floor==0)run.options.push_back(i);}
    else{
        const auto& node=run.map[run.node];if(run.place==Place::Map)run.options=node.next;
        if(run.place==Place::Event){Require(node.kind==NodeKind::Event,"Event on wrong node");for(const auto& event:config.events)if(event.id==node.event_id){run.event_title=event.title;run.event_text=event.text;run.event_choices=event.choices;}Require(!run.event_choices.empty(),"Unknown event");}
        if(run.place==Place::Shop)Require(node.kind==NodeKind::Shop,"Shop on wrong node");
        if(run.place==Place::Rest)Require(node.kind==NodeKind::Rest,"Rest on wrong node");
        if(run.place==Place::CardSelect||run.place==Place::RelicSelect)Require(node.kind==NodeKind::Event,"Selection on wrong node");
        if(run.place==Place::CardSelect)Require(run.selection=="upgrade"||run.selection=="remove","Unknown card selection");
        if(run.place==Place::RelicReward)Require(node.kind==NodeKind::Boss&&!run.relic_rewards.empty(),"Invalid boss reward");
        if(run.place==Place::Map)Require(!run.victory&&!run.defeat&&!node.next.empty(),"Completed map has no result");
    }
    Require(run.pending_reward==(run.place==Place::Reward&&!run.rewards.empty()),"Invalid pending reward");
    for(const auto& item:run.shop){Require(item.price>=1&&item.price<=100000&&int(item.ware)>=0&&int(item.ware)<=int(Ware::Potion),"Invalid shop");if(item.ware==Ware::Card)FindCard(config,item.id);else if(item.ware==Ware::Relic)FindRelic(config,item.id);else FindPotion(config,item.id);}
    for(const auto& value:run.metrics)Require(value.second>=0,"Invalid run metric");
}
void ValidatePair(const RunState& run,const Combat& combat){
    if(run.place!=Place::Fight&&(run.place!=Place::Reward||run.reward_from_event))return;
    const auto snapshot=combat.Snapshot();Require(run.node>=0&&run.active_encounter_id==snapshot.enemy_id,"Combat does not match the run");
    const auto& node=run.map[run.node];Require(IsFightNode(node.kind)||node.kind==NodeKind::Event,"Combat on wrong node");
    if(IsFightNode(node.kind))Require(node.encounter_id==snapshot.enemy_id,"Combat does not match map encounter");
    Require(run.max_hp==snapshot.player.max_hp,"Player maximum HP differs");
    std::multiset<std::string> pile;for(const auto* list:{&snapshot.draw,&snapshot.hand,&snapshot.discard,&snapshot.exhaust})pile.insert(list->begin(),list->end());
    for(const auto& id:snapshot.created){auto found=pile.find(id);Require(found!=pile.end(),"Generated card missing");pile.erase(found);}
    Require(pile==std::multiset<std::string>(run.deck.begin(),run.deck.end()),"Combat deck is not conserved");
    if(run.place==Place::Reward)Require(snapshot.phase==Phase::Won&&run.fight_resolved,"Rewards before victory");
    if(run.place==Place::Fight&&!run.victory&&!run.defeat)Require(snapshot.phase==Phase::Player&&!run.fight_resolved,"Finished fight has no outcome");
    if(run.victory)Require(snapshot.phase==Phase::Won&&run.act+1==int(combat.config.acts.size())&&node.next.empty(),"Invalid campaign victory");
    if(run.defeat)Require(snapshot.phase==Phase::Lost,"Invalid defeat");
    if(run.fight_resolved)Require(run.hp==snapshot.player.hp&&run.rage==snapshot.rage,"Combat resources not banked");
}
}

bool SaveSession(const std::filesystem::path& file,const RunState& run,const Combat& combat,int speed,std::string& error){
    try{
        Require(speed>=1&&speed<=3,"Invalid speed");auto canonical=run;ValidateRun(canonical,combat.config);
        Combat check(combat.config);check.Restore(combat.Snapshot(),combat.Mods());ValidatePair(canonical,check);
        const Json data={{"format","undergod-card"},{"version",2},{"speed",speed},{"run",canonical},{"combat",combat.Snapshot()},{"combat_mods",combat.Mods()}};
        return WriteAtomicText(file,data.dump(2)+"\n",error);
    }catch(const std::exception& e){error=e.what();return false;}
}
bool LoadSession(const std::filesystem::path& file,RunState& run,Combat& combat,int& speed,std::string& error){
    try{
        Require(std::filesystem::is_regular_file(file)&&std::filesystem::file_size(file)<=4*1024*1024,"Save missing or too large");std::ifstream input(file);Json data;input>>data;
        Require(data.at("format")=="undergod-card"&&data.at("version")==2,"Unknown save format");NumericBounds(data);
        const int next_speed=data.at("speed");Require(next_speed>=1&&next_speed<=3,"Invalid speed");
        auto next_run=data.at("run").get<RunState>();ValidateRun(next_run,combat.config);Combat next_combat(combat.config);
        const auto mods=data.at("combat_mods").get<CombatMods>();
        Require(mods.enemy_hp_percent>=50&&mods.enemy_hp_percent<=1000&&mods.enemy_damage>=0&&mods.enemy_damage<=100&&mods.max_mana>=0&&mods.max_mana<=10,"Invalid saved combat modifiers");
        if(next_run.place==Place::Fight)Require(mods.class_id==next_run.class_id,"Combat class differs from run");
        next_combat.Restore(data.at("combat").get<CombatSnapshot>(),mods);ValidatePair(next_run,next_combat);
        run=std::move(next_run);combat=std::move(next_combat);speed=next_speed;error.clear();return true;
    }catch(const std::exception& e){error=e.what();return false;}
}
}
