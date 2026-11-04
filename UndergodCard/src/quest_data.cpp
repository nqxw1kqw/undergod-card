#include "quest_core.hpp"
#include "quest_timeline.hpp"
#include "../third_party/nlohmann/json.hpp"
#include <fstream>
#include <set>
#include <cmath>

namespace undergod {
namespace {
using Json=nlohmann::json;
Json Read(const std::string& path){std::ifstream f(path);if(!f)throw std::runtime_error("Cannot open "+path);Json data;f>>data;return data;}
}
Config LoadConfig(const std::string& directory){
    Config c;const auto b=Read(directory+"/balance.json");
    c.balance={b.at("playerHp"),b.at("energy"),b.at("draw"),b.at("rageMax"),b.at("rageAttack"),b.at("rageBlock"),b.at("ultimateDamage")};
    const auto e=Read(directory+"/enemies.json").at(0);
    c.enemy={e.at("name"),e.at("maxHp"),e.at("minDamage"),e.at("maxDamage"),e.at("block"),e.at("attackChance")};
    c.enemy.opening_attack=e.value("openingAttack",true);c.enemy.max_guards=e.value("maxConsecutiveGuards",1);
    const auto character=Read(directory+"/characters.json").at(0);c.player_name=character.at("name");c.player_id=character.at("id");
    if(c.player_id.empty()||c.player_id.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_")!=std::string::npos||c.enemy.max_guards<0)throw std::runtime_error("Invalid character ID or enemy guard limit");
    std::set<std::string> ids;int copies=0;
    for(const auto& j:Read(directory+"/cards.json")){
        CardDef card{j.at("id"),j.at("name"),j.at("description"),j.at("cost"),j.value("damage",0),j.value("block",0),j.value("heal",0),j.value("copies",0)};
        if(card.id.empty()||!ids.insert(card.id).second||card.cost<0||card.damage<0||card.block<0||card.heal<0||card.copies<0)throw std::runtime_error("Invalid card definition");
        copies+=card.copies;c.cards.push_back(card);
    }
    if(copies<1||copies>1000||c.balance.player_hp<1||c.enemy.hp<1||c.balance.draw<1||c.balance.draw>10||c.balance.energy<1||c.balance.rage_max<1||c.balance.rage_attack<0||c.balance.rage_block<0||c.balance.ultimate_damage<1||c.enemy.attack_chance<0||c.enemy.attack_chance>1||c.enemy.min_damage<0||c.enemy.max_damage<c.enemy.min_damage||c.enemy.block<0)throw std::runtime_error("Invalid balance configuration");
    return c;
}
Timeline LoadTimeline(const std::string& file){
    const auto j=Read(file);Timeline timeline;timeline.skippable=j.value("skippable",true);int impacts=0,ends=0,falls=0;float impact_time=0,end_time=0,fall_end=0;
    for(const auto& s:j.at("steps")){
        Step step;step.t=s.at("t");const std::string action=s.at("action");
        if(action=="dimScreen"){step.cue=Cue::Dim;step.value=s.at("alpha");}
        else if(action=="cameraZoom"){step.cue=Cue::Zoom;step.value=s.at("to");step.duration=s.at("duration");}
        else if(action=="playSprite"){
            step.cue=Cue::Pose;const std::string target=s.at("target");if(target!="player"&&target!="enemy")throw std::runtime_error("Unknown cinematic actor");step.enemy=target=="enemy";step.pose=s.at("anim");
            if(step.pose!="idle"&&step.pose!="attack"&&step.pose!="hit"&&step.pose!="defend"&&step.pose!="ult_charge"&&step.pose!="ult_throw")throw std::runtime_error("Unknown cinematic pose");
        }
        else if(action=="spawnVFX"){
            step.cue=Cue::Vfx;step.value=s.at("frames");const std::string target=s.at("at");if(target!="player"&&target!="enemy")throw std::runtime_error("Unknown VFX target");step.enemy=target=="enemy";
            if(s.at("sheet")!="vfx/spear_impact.png"||step.value<1||step.value>4)throw std::runtime_error("Unsupported VFX sheet or frame count");
        }
        else if(action=="moveSpear"){
            step.cue=Cue::Spear;const std::string phase=s.at("phase");if(phase!="rise"&&phase!="fall")throw std::runtime_error("Invalid spear phase");step.falling=phase=="fall";
            step.duration=s.at("duration");step.from_x=s.at("from").at(0);step.from_y=s.at("from").at(1);step.to_x=s.at("to").at(0);step.to_y=s.at("to").at(1);
            if(step.duration<=0)throw std::runtime_error("Spear movement needs a positive duration");
            if(step.falling){++falls;fall_end=step.t+step.duration;}
        }
        else if(action=="shake"){step.cue=Cue::Shake;step.value=s.at("intensity");step.duration=s.at("duration");}
        else if(action=="impact"){if(s.at("callback")!="applyDamage")throw std::runtime_error("Unknown cinematic callback");step.cue=Cue::Impact;++impacts;impact_time=step.t;}
        else if(action=="flashWhite"){step.cue=Cue::Flash;step.duration=s.at("duration");}
        else if(action=="end"){step.cue=Cue::End;++ends;end_time=step.t;}
        else throw std::runtime_error("Unknown cinematic action: "+action);
        if(step.t<0||step.duration<0)throw std::runtime_error("Invalid cinematic timing");
        timeline.steps.push_back(step);
    }
    if(impacts!=1||ends!=1||impact_time>end_time)throw std::runtime_error("Cinematic needs one impact before end");
    if(falls!=1||std::abs(fall_end-impact_time)>.1f)throw std::runtime_error("Spear contact must match damage timing");
    return timeline;
}
}
