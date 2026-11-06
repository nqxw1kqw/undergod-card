#include "quest_core.hpp"
#include "../third_party/nlohmann/json.hpp"
#include <fstream>
#include <set>

namespace undergod {
namespace {
using Json=nlohmann::json;
Json Read(const std::string& file){std::ifstream in(file);if(!in)throw std::runtime_error("Cannot open "+file);Json j;in>>j;return j;}
void Require(bool good,const char* error){if(!good)throw std::runtime_error(error);}
bool Ident(const std::string& id){return !id.empty()&&id.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_")==std::string::npos;}
}
const ClassDef& FindClass(const Config& config,const std::string& id){for(const auto& item:config.classes)if(item.id==id)return item;throw std::runtime_error("Unknown class: "+id);}
void LoadWorld(Config& config,const std::string& directory){
    std::set<std::string> ids;
    auto known_card=[&](const std::string& id){for(const auto& c:config.cards)if(c.id==id)return true;return false;};
    auto known_enemy=[&](const std::string& id){for(const auto& e:config.bestiary)if(e.id==id)return true;return false;};
    for(const auto& j:Read(directory+"/classes.json")){
        ClassDef c;c.id=j.at("id");c.name=j.at("name");c.description=j.at("description");c.sprite=j.at("sprite");
        c.hp=j.at("hp");c.energy=j.at("energy");c.draw=j.at("draw");c.ultimate=j.at("ultimate");c.ultimate_damage=j.at("ultimateDamage");c.ultimate_hits=j.at("ultimateHits");c.mana=j.value("mana",false);c.deck=j.at("deck").get<std::vector<std::string>>();
        Require(Ident(c.id)&&Ident(c.sprite)&&ids.insert(c.id).second&&c.hp>=20&&c.hp<=100&&c.energy>=1&&c.energy<=5&&c.draw>=1&&c.draw<=10&&c.ultimate_damage>0&&c.ultimate_hits>=1&&c.ultimate_hits<=8&&c.deck.size()==10,"Invalid class");
        Require(c.ultimate=="spear"||c.ultimate=="storm"||c.ultimate=="shadow","Unknown ultimate");for(const auto& id:c.deck)Require(known_card(id),"Unknown starting card");config.classes.push_back(c);
    }
    Require(config.classes.size()==3,"Campaign needs three classes");ids.clear();
    for(const auto& j:Read(directory+"/campaign.json")){
        ActDef act;act.id=j.at("id");act.name=j.at("name");act.biome=j.at("biome");act.floors=j.at("floors");act.columns=j.at("columns");act.bosses=j.at("bosses").get<std::vector<std::string>>();act.combats=j.at("combats").get<std::vector<std::string>>();act.elites=j.at("elites").get<std::vector<std::string>>();act.final_boss=j.value("finalBoss","");
        Require(Ident(act.id)&&ids.insert(act.id).second&&act.floors>=10&&act.floors<=13&&act.columns>=3&&act.columns<=4&&!act.bosses.empty()&&!act.combats.empty()&&!act.elites.empty(),"Invalid act");
        for(const auto* list:{&act.bosses,&act.combats,&act.elites})for(const auto& id:*list)Require(known_enemy(id),"Unknown encounter in act");
        if(!act.final_boss.empty())Require(known_enemy(act.final_boss),"Unknown final boss");config.acts.push_back(act);
    }
    Require(config.acts.size()==3,"Campaign needs three acts");ids.clear();
    const std::set<std::string> bonuses={"spear_bonus","max_hp","third_attack","damage","start_block","retain_block","rest_bonus","start_rage","ultimate_bonus","ultimate_vulnerable","draw","poison_bonus","first_attack","max_mana","energy","block","ultimate_discount"};
    for(const auto& j:Read(directory+"/skills.json")){
        SkillDef s;s.id=j.at("id");s.class_id=j.at("class");s.branch=j.at("branch");s.name=j.at("name");s.description=j.at("description");s.tier=j.at("tier");s.cost=j.at("cost");s.unlock_card=j.value("unlockCard","");s.bonuses=j.at("bonuses").get<std::map<std::string,int>>();
        FindClass(config,s.class_id);Require(Ident(s.id)&&ids.insert(s.id).second&&s.tier>=1&&s.tier<=5&&s.cost>0&&s.cost<=100,"Invalid skill");
        if(!s.unlock_card.empty())Require(known_card(s.unlock_card),"Unknown skill card");for(const auto& pair:s.bonuses)Require(bonuses.count(pair.first)&&pair.second>=0&&pair.second<=50,"Unknown skill bonus");config.skills.push_back(s);
    }
    Require(config.skills.size()==45,"Expected three trees of fifteen skills");
    for(const auto& c:config.cards)if(!c.requires_skill.empty())Require(ids.count(c.requires_skill)>0,"Card needs unknown skill");ids.clear();
    for(const auto& j:Read(directory+"/achievements.json")){
        AchievementDef a;a.id=j.at("id");a.name=j.at("name");a.description=j.at("description");a.metric=j.at("metric");a.threshold=j.at("threshold");a.feathers=j.at("feathers");Require(Ident(a.id)&&ids.insert(a.id).second&&a.threshold>0&&a.feathers>=0,"Invalid achievement");config.achievements.push_back(a);
    }
    int level=1;
    for(const auto& j:Read(directory+"/challenges.json")){
        ChallengeDef c;c.level=j.at("level");c.description=j.at("description");c.penalties=j.at("penalties").get<std::map<std::string,int>>();Require(c.level==level++,"Challenge levels must be ordered");config.challenges.push_back(c);
    }
    Require(config.challenges.size()==10,"Expected ten challenge levels");
    for(const auto& e:config.bestiary){for(const auto& p:e.patterns)if(!p.summon_id.empty())Require(known_enemy(p.summon_id),"Unknown summon");for(const auto& p:e.phases)if(!p.summon_id.empty())Require(known_enemy(p.summon_id),"Unknown phase summon");}
}
}
