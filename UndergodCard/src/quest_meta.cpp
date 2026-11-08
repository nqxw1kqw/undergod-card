#include "quest_meta.hpp"
#include "quest_run.hpp"
#include "quest_storage.hpp"
#include "../third_party/nlohmann/json.hpp"
#include <fstream>
#include <set>
namespace undergod {
namespace {
bool Has(const std::vector<std::string>& ids,const std::string& id){return std::find(ids.begin(),ids.end(),id)!=ids.end();}
void Unique(std::vector<std::string>& ids,const std::string& id){if(!Has(ids,id))ids.push_back(id);}
void AwardAchievements(Profile& profile,const Config& config){for(const auto& a:config.achievements)if(!Has(profile.achievements,a.id)&&profile.metrics[a.metric]>=a.threshold){profile.achievements.push_back(a.id);profile.feathers+=a.feathers;}}
}
int SkillLevel(const Profile& profile,const Config& config,const std::string& class_id,const std::string& branch){int level=0;for(const auto& s:config.skills)if(s.class_id==class_id&&s.branch==branch&&Has(profile.skills,s.id))level=std::max(level,s.tier);return level;}
bool UnlockSkill(Profile& profile,const Config& config,const std::string& id){
    for(const auto& s:config.skills)if(s.id==id){
        if(Has(profile.skills,id)||profile.feathers<s.cost||SkillLevel(profile,config,s.class_id,s.branch)!=s.tier-1)return false;
        profile.feathers-=s.cost;profile.skills.push_back(id);profile.metrics["skills"]=int(profile.skills.size());AwardAchievements(profile,config);return true;
    }
    return false;
}
bool SettleProfile(Profile& profile,RunState& run,const Config& config){
    if((!run.victory&&!run.defeat)||run.meta_claimed)return false;
    run.meta_claimed=true;
    if(Has(profile.settled_runs,run.run_id))return false;
    profile.settled_runs.push_back(run.run_id);profile.feathers+=run.victory?25+run.challenge*3:3+run.act*2;
    ++profile.metrics["runs"];if(run.victory){++profile.metrics["wins"];++profile.metrics["win_"+run.class_id];profile.highest_challenge=std::max(profile.highest_challenge,run.challenge);}
    for(const auto& pair:run.metrics)profile.metrics[pair.first]+=pair.second;
    for(const auto& id:run.defeated_bosses)Unique(profile.bosses,id);
    profile.metrics["bosses"]+=int(run.defeated_bosses.size());
    for(const auto& id:run.relics)Unique(profile.relics,id);
    profile.metrics["unique_bosses"]=int(profile.bosses.size());profile.metrics["unique_relics"]=int(profile.relics.size());profile.metrics["challenge"]=std::max(0,profile.highest_challenge);profile.metrics["skills"]=int(profile.skills.size());
    AwardAchievements(profile,config);
    return true;
}
bool SaveProfile(const std::filesystem::path& file,const Profile& p,std::string& error){
    nlohmann::json j={{"format","undergod-profile"},{"version",1},{"feathers",p.feathers},{"highestChallenge",p.highest_challenge},{"nextRun",p.next_run},{"class",p.selected_class},{"muted",p.muted},{"reducedFeedback",p.reduced_feedback},{"skills",p.skills},{"achievements",p.achievements},{"bosses",p.bosses},{"relics",p.relics},{"settledRuns",p.settled_runs},{"metrics",p.metrics}};
    return WriteAtomicText(file,j.dump(2)+"\n",error);
}
bool LoadProfile(const std::filesystem::path& file,Profile& profile,const Config& config,std::string& error){
    try{
        if(!std::filesystem::is_regular_file(file)||std::filesystem::file_size(file)>16*1024*1024)throw std::runtime_error("Profile missing or too large");
        std::ifstream in(file);nlohmann::json j;in>>j;if(j.at("format")!="undergod-profile"||j.at("version")!=1)throw std::runtime_error("Unknown profile version");
        Profile p;p.feathers=j.at("feathers");p.highest_challenge=j.at("highestChallenge");p.next_run=j.at("nextRun");p.selected_class=j.at("class");p.skills=j.at("skills").get<std::vector<std::string>>();p.achievements=j.at("achievements").get<std::vector<std::string>>();p.bosses=j.at("bosses").get<std::vector<std::string>>();p.relics=j.at("relics").get<std::vector<std::string>>();p.settled_runs=j.at("settledRuns").get<std::vector<std::string>>();p.metrics=j.at("metrics").get<std::map<std::string,int>>();
        p.muted=j.value("muted",false);p.reduced_feedback=j.value("reducedFeedback",false);
        if(j.at("nextRun").get<int64_t>()<1||j.at("nextRun").get<int64_t>()>1000000000||p.settled_runs.size()>100000)throw std::runtime_error("Invalid profile run counter");
        if(p.feathers<0||p.feathers>10000000||p.highest_challenge<-1||p.highest_challenge>10||p.next_run<1||p.skills.size()>config.skills.size()||p.achievements.size()>config.achievements.size())throw std::runtime_error("Invalid profile values");
        FindClass(config,p.selected_class);std::set<std::string> seen;
        for(const auto& id:p.skills){if(!seen.insert(id).second)throw std::runtime_error("Duplicate profile skill");bool known=false;for(const auto& s:config.skills)if(s.id==id)known=true;if(!known)throw std::runtime_error("Unknown profile skill");}
        for(const auto& s:config.skills)if(Has(p.skills,s.id)&&s.tier>1){bool previous=false;for(const auto& before:config.skills)if(before.class_id==s.class_id&&before.branch==s.branch&&before.tier==s.tier-1&&Has(p.skills,before.id))previous=true;if(!previous)throw std::runtime_error("Broken skill prerequisite");}
        for(const auto& pair:p.metrics)if(pair.second<0||pair.second>100000000)throw std::runtime_error("Invalid profile metric");
        seen.clear();for(const auto& id:p.achievements){bool known=false;for(const auto& a:config.achievements)known|=a.id==id;if(!known||!seen.insert(id).second)throw std::runtime_error("Invalid profile achievement");}
        seen.clear();for(const auto& id:p.bosses){if(FindEnemy(config,id).role!="boss"||!seen.insert(id).second)throw std::runtime_error("Invalid profile boss");}
        seen.clear();for(const auto& id:p.relics){FindRelic(config,id);if(!seen.insert(id).second)throw std::runtime_error("Duplicate profile relic");}
        seen.clear();for(const auto& id:p.settled_runs)if(id.empty()||id.size()>128||!seen.insert(id).second)throw std::runtime_error("Invalid settled run ledger");
        profile=std::move(p);error.clear();return true;
    }catch(const std::exception& e){error=e.what();return false;}
}
}
