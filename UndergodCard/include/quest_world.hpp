#pragma once
#include "quest_effects.hpp"
#include <map>

namespace undergod {
struct Config;
void LoadWorld(Config& config,const std::string& directory);
struct ClassDef {
    std::string id,name,description,sprite,ultimate="spear";
    int hp=50,energy=3,draw=5,ultimate_damage=40,ultimate_hits=1;
    bool mana=false;
    std::vector<std::string> deck;
};
struct ActDef {
    std::string id,name,biome,final_boss;
    std::vector<std::string> bosses,combats,elites;
    int floors=12,columns=3;
};
struct SkillDef {
    std::string id,class_id,branch,name,description,unlock_card;
    int tier=1,cost=5;
    std::map<std::string,int> bonuses;
};
struct AchievementDef {
    std::string id,name,description,metric;
    int threshold=1,feathers=5;
};
struct ChallengeDef {
    int level=1;
    std::string description;
    std::map<std::string,int> penalties;
};
const ClassDef& FindClass(const Config& config,const std::string& id);
}
