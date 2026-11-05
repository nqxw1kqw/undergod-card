#pragma once
#include "quest_core.hpp"
#include <filesystem>

namespace undergod {
struct RunState;
struct Profile {
    int feathers=0,highest_challenge=-1;
    uint64_t next_run=1;
    std::string selected_class="chick";
    bool muted=false;
    std::vector<std::string> skills,achievements,bosses,relics,settled_runs;
    std::map<std::string,int> metrics;
};
int SkillLevel(const Profile& profile,const Config& config,const std::string& class_id,const std::string& branch);
bool UnlockSkill(Profile& profile,const Config& config,const std::string& skill_id);
bool SettleProfile(Profile& profile,RunState& run,const Config& config);
bool LoadProfile(const std::filesystem::path& file,Profile& profile,const Config& config,std::string& error);
bool SaveProfile(const std::filesystem::path& file,const Profile& profile,std::string& error);
}
