#include "quest_core.hpp"
#include "../third_party/nlohmann/json.hpp"
#include <fstream>
#include <set>

namespace undergod {
namespace {
using Json=nlohmann::json;
template<class T> T Enum(const Json& j,const char* field,T old,std::initializer_list<std::pair<const char*,T>> values){
    if(!j.contains(field))return old;const std::string name=j.at(field);
    for(const auto& item:values)if(name==item.first)return item.second;
    throw std::runtime_error("Unknown animation "+std::string(field)+": "+name);
}
SkillAnimation Read(const Json& j,SkillAnimation a){
    a.motion=Enum(j,"motion",a.motion,{{"still",SkillMotion::Still},{"thrust",SkillMotion::Thrust},{"slash",SkillMotion::Slash},{"peck",SkillMotion::Peck},{"stomp",SkillMotion::Stomp},{"dash",SkillMotion::Dash},{"pounce",SkillMotion::Pounce},{"swoop",SkillMotion::Swoop},{"guard",SkillMotion::Guard},{"cast",SkillMotion::Cast},{"roar",SkillMotion::Roar},{"charge",SkillMotion::Charge}});
    a.path=Enum(j,"path",a.path,{{"none",FlightPath::None},{"straight",FlightPath::Straight},{"arc",FlightPath::Arc},{"fan",FlightPath::Fan},{"rain",FlightPath::Rain},{"return",FlightPath::Return},{"helix",FlightPath::Helix},{"drain",FlightPath::Drain},{"orbit",FlightPath::Orbit},{"ground",FlightPath::Ground},{"beam",FlightPath::Beam}});
    a.pose=j.value("pose",a.pose);a.cast_fx=j.value("cast",a.cast_fx);a.projectile=j.value("projectile",a.projectile);a.impact_fx=j.value("impact",a.impact_fx);a.ward=j.value("ward",a.ward);a.prop=j.value("prop",a.prop);
    a.launch=j.value("launch",a.launch);a.contact=j.value("contact",a.contact);a.duration=j.value("duration",a.duration);a.size=j.value("size",a.size);a.spread=j.value("spread",a.spread);a.arc=j.value("arc",a.arc);a.lunge=j.value("lunge",a.lunge);a.count=j.value("count",a.count);a.self=j.value("self",a.self);
    return a;
}
}
const SkillAnimation& FindSkillAnimation(const Config& config,const std::string& id){
    const std::string base=!id.empty()&&id.back()=='+'?id.substr(0,id.size()-1):id;
    const auto it=config.animations.profiles.find(base);
    if(it==config.animations.profiles.end())throw std::runtime_error("Missing skill animation: "+id);
    return it->second;
}
void LoadSkillAnimations(Config& config,const std::string& directory){
    std::ifstream file(directory+"/skill_animations.json");if(!file)throw std::runtime_error("Missing skill_animations.json");Json root;file>>root;
    AnimationLibrary library;std::set<std::string> names;
    for(const auto& name:root.at("effects")){const std::string key=name;if(key.empty()||!names.insert(key).second)throw std::runtime_error("Duplicate animation effect");library.effects.push_back(key);}
    for(const auto& j:root.at("bodies")){
        BodyAction body{j.at("actor"),j.at("pose")};
        body.emitter_x=j.value("emitterX",400.f);body.emitter_y=j.value("emitterY",240.f);
        if(body.actor.empty()||body.pose.empty()||!names.insert(body.actor+"_"+body.pose).second)throw std::runtime_error("Duplicate body animation");
        bool known=false;for(const auto& c:config.classes)known|=c.sprite==body.actor;for(const auto& e:config.bestiary)known|=e.sprite==body.actor;
        if(!known)throw std::runtime_error("Unknown animation actor: "+body.actor);library.bodies.push_back(body);
    }
    std::map<std::string,SkillAnimation> families;
    for(auto it=root.at("families").begin();it!=root.at("families").end();++it)families[it.key()]=Read(it.value(),{});
    const std::set<std::string> poses={"idle","melee","cast","guard","peck","stomp","charge","pounce"};
    std::set<std::string> effects(library.effects.begin(),library.effects.end());effects.insert({"spear","feather","kunai","spear_impact"});
    for(auto it=root.at("profiles").begin();it!=root.at("profiles").end();++it){
        const std::string family=it.value().at("family");const auto found=families.find(family);if(found==families.end())throw std::runtime_error("Unknown animation family");
        auto a=Read(it.value(),found->second);a.id=it.key();
        if(!poses.count(a.pose)||a.launch<0||a.contact<a.launch||a.duration<a.contact+.12f||a.duration>3||a.size<20||a.size>500||a.count<1||a.count>8||a.spread<0||a.spread>240||a.arc< -200||a.arc>240||a.lunge<0||a.lunge>1.2f)throw std::runtime_error("Invalid skill animation: "+a.id);
        auto item_texture=[&](const std::string& key){for(const auto& card:config.cards)if("card_"+card.id==key)return true;for(const auto& card:config.boss_cards)if("card_"+card.id==key)return true;return false;};
        for(const auto* key:{&a.cast_fx,&a.projectile,&a.impact_fx,&a.ward})if(!key->empty()&&!effects.count(*key)&&!item_texture(*key))throw std::runtime_error("Unknown skill FX: "+*key);
        if(a.path!=FlightPath::None&&(a.projectile.empty()||a.contact-a.launch<.08f))throw std::runtime_error("Projectile needs a visible flight interval");
        if(!a.prop.empty()&&!item_texture(a.prop))throw std::runtime_error("Unknown animation prop");
        library.profiles.emplace(a.id,std::move(a));
    }
    for(const auto& c:config.cards)if(!c.upgraded&&!library.profiles.count(c.id))throw std::runtime_error("Player card has no animation: "+c.id);
    for(const auto& c:config.boss_cards)if(!library.profiles.count(c.id))throw std::runtime_error("Boss card has no animation: "+c.id);
    for(const auto* id:{"wait","mob_attack","mob_multi","mob_defend","mob_buff","mob_debuff","mob_charge","mob_curse","mob_summon","mob_mimic","mob_drain","mob_spit"})if(!library.profiles.count(id))throw std::runtime_error("Missing fallback action animation");
    config.animations=std::move(library);
}
}
