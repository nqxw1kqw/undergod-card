#include "game.hpp"
#include "../third_party/nlohmann/json.hpp"
#include <fstream>
#include <iostream>
#include <set>
using namespace undergod;
using Json=nlohmann::json;
extern void Check(bool,const char*);
namespace {
void Settle(Game& g){for(int i=0;i<120&&g.Busy();++i)g.Update(.1f);}
bool AdvanceAuto(Game& g){const auto action=g.Suggest();if(action.kind==AutoKind::None)return false;g.Act(action);Settle(g);return true;}
Json Read(const std::filesystem::path& path){std::ifstream in(path);Json j;in>>j;return j;}
void Write(const std::filesystem::path& path,const Json& j){std::ofstream out(path);out<<j.dump(2);}
std::vector<int> Path(const std::vector<MapNode>& map,int target){
    std::vector<int> parent(map.size(),-2);
    for(int i=0;i<int(map.size());++i)if(map[i].floor==0)parent[i]=-1;
    for(int i=0;i<int(map.size());++i)if(parent[i]!=-2)for(int next:map[i].next)if(parent[next]==-2)parent[next]=i;
    std::vector<int> path;for(int at=target;at>=0;at=parent.at(at))path.push_back(at);std::reverse(path.begin(),path.end());return path;
}
bool Reach(Game& g,NodeKind kind){
    for(unsigned seed=0;seed<30;++seed){
        g.NewRun(seed);int target=-1;
        for(int i=0;i<int(g.run.map.size());++i)if(g.run.map[i].kind==kind){target=i;break;}
        if(target<0)continue;
        const auto path=Path(g.run.map,target);
        bool failed=false;
        for(int id:path){
            if(!g.Map()){failed=true;break;}
            const auto option=std::find(g.run.options.begin(),g.run.options.end(),id);
            if(option==g.run.options.end()){failed=true;break;}
            g.ChooseNode(int(option-g.run.options.begin()));
            if(id==target)return true;
            for(int n=0;n<400&&!g.Map()&&!g.Result();++n)if(!AdvanceAuto(g))break;
        }
        if(!failed&&g.run.node==target)return true;
    }
    return false;
}
bool SameMap(const std::vector<MapNode>& a,const std::vector<MapNode>& b){
    if(a.size()!=b.size())return false;
    for(size_t i=0;i<a.size();++i)if(a[i].kind!=b[i].kind||a[i].encounter_id!=b[i].encounter_id||a[i].event_id!=b[i].event_id||a[i].reward_gold!=b[i].reward_gold||a[i].next!=b[i].next)return false;
    return true;
}
}

void RunChecks(const Config& config,const Timeline& timeline){
    {
        bool deterministic=true,reachable=true,valid=true,varied=false;
        std::set<std::string> combats,elites;std::array<int,5> counts{};int samples=0;
        const auto reference=GenerateMap(config,0);
        for(unsigned seed=0;seed<300;++seed){
            const auto map=GenerateMap(config,seed);deterministic&=SameMap(map,GenerateMap(config,seed));varied|=!SameMap(map,reference);
            std::vector<bool> seen(map.size());std::vector<int> queue;
            for(int i=0;i<int(map.size());++i)if(map[i].floor==0){queue.push_back(i);valid&=map[i].kind==NodeKind::Combat;}
            while(!queue.empty()){const int id=queue.back();queue.pop_back();if(seen[id])continue;seen[id]=true;for(int next:map[id].next){valid&=next>id&&map[next].floor==map[id].floor+1;queue.push_back(next);}}
            reachable&=std::find(seen.begin(),seen.end(),false)==seen.end();
            for(const auto& node:map){
                if(node.floor==config.map_floors-1)valid&=node.kind==NodeKind::Rest;
                if(node.floor<config.map_floors)valid&=!node.next.empty();
                if(node.kind==NodeKind::Combat){combats.insert(node.encounter_id);valid&=FindEnemy(config,node.encounter_id).min_floor<=node.floor&&node.reward_gold>=config.combat_gold_min&&node.reward_gold<=config.combat_gold_max;}
                if(node.kind==NodeKind::Elite){elites.insert(node.encounter_id);valid&=node.floor>=config.elite_min_floor&&node.reward_gold>=config.elite_gold_min&&node.reward_gold<=config.elite_gold_max;}
                if(node.floor>=config.elite_min_floor&&node.floor<config.map_floors-1){++counts[int(node.kind)];++samples;}
            }
            valid&=map.back().kind==NodeKind::Boss&&map.back().floor==config.map_floors;
        }
        Check(deterministic&&varied,"map is repeatable by seed and changes across seeds");
        Check(reachable&&valid,"300 seeds have valid rewards, pacing, reachable paths and a final boss");
        Check(combats.size()==6&&elites.size()==2,"all six regular and two elite enemies occur");
        bool weighted=true;int total=0;for(int weight:config.map_weights)total+=weight;
        for(int kind=0;kind<5;++kind)weighted&=std::abs(double(counts[kind])/samples-double(config.map_weights[kind])/total)<.035;
        Check(weighted,"eligible random nodes follow JSON weights across 300 seeds");
        auto events_only=config;events_only.map_weights={0,0,100,0,0};const auto map=GenerateMap(events_only,19);bool all_events=true;
        for(const auto& node:map)if(node.floor>0&&node.floor<config.map_floors-1)all_events&=node.kind==NodeKind::Event;
        Check(all_events,"node ratios come from data rather than a fixed layout");
        auto wide=config;wide.map_columns=4;wide.map_floors=15;Check(GenerateMap(wide,12).size()==61,"four-column fifteen-floor maps are supported");
    }
    {
        Game g(config,timeline,false);g.NewRun();g.run.place=Place::Event;g.run.gold=3;g.run.hp=20;
        g.run.event_choices={{"pay","heal","","","",10,0,-12,0,false},{"leave","leave","","","",0,0,0,0,false}};
        const auto before=g.run;g.ChooseEvent(0);
        Check(g.Event()&&g.run.hp==before.hp&&g.run.gold==before.gold&&g.run.event_choices.size()==2,"unaffordable event choices do not charge, heal or leave");
        g.ChooseEvent(1);Check(g.Map(),"a free event exit remains available");
        g.run.place=Place::Rest;g.run.hp=10;g.run.relics={"water_jar"};const int amount=RestHealAmount(g.run,config);
        g.RestHeal();Check(amount==20&&g.run.hp==30,"rest preview and healing include the relic bonus");
        Check(!g.RestHeal(),"rest cannot be used twice");
        Game elite(config,timeline,false);const bool found=Reach(elite,NodeKind::Elite);Check(found,"a legal path reaches an elite");
        if(found){elite.state.enemy.hp=1;elite.state.hand={"punch"};elite.state.energy=3;elite.Play(0);Settle(elite);const auto count=elite.run.relics.size();elite.Update(1);Check(elite.Reward()&&count==1&&elite.run.relics.size()==count,"an elite grants one relic exactly once");}
        Game potion(config,timeline,false);potion.NewRun();potion.ChooseNode(0);potion.run.potions={"serum","smoke_shield","strength_tonic","weak_brew"};potion.run.potion_slots=4;
        potion.state.player.hp=20;Check(potion.DrinkPotion(0)&&potion.state.player.hp==35&&potion.run.potions.size()==3,"healing potion is consumed and restores fifteen HP");
        potion.state.rage=85;Check(potion.DrinkPotion(1)&&potion.state.rage==100,"rage potion uses the effect system and respects the cap");
        const auto cards=potion.state.hand.size();Check(potion.DrinkPotion(1)&&potion.state.hand.size()==cards+3,"feather potion draws three cards");
        potion.EndTurn();Check(!potion.DrinkPotion(0),"potions cannot interrupt an enemy action");
    }
    std::filesystem::create_directories("artifacts");
    const std::filesystem::path save="artifacts/run-check-save.json",copy="artifacts/run-check-copy.json",bad="artifacts/run-check-bad.json";
    {
        Game original(config,timeline,false),restored(config,timeline,false);original.NewRun(1337);original.SetSaveFile(save);restored.SetSaveFile(save);
        Check(original.Save()&&restored.Load()&&restored.Map()&&SameMap(original.run.map,restored.run.map),"map checkpoint restores seed, route and screen");
        original.ChooseNode(0);original.SetSpeed(2);original.run.relics={"water_jar"};original.run.potions={"serum","holy_water"};
        original.state.player.hp=39;original.state.enemy.status.poison=2;
        Check(original.Save(),"player-turn save includes the fight and inventory");
        Check(restored.Load()&&restored.Speed()==2&&restored.state.player.hp==39&&restored.state.enemy.status.poison==2&&restored.run.potions==original.run.potions,"loading restores speed, HP, statuses and potions");
        restored.SetSaveFile(copy);Check(restored.Save()&&Read(copy)==Read(save),"session JSON round trips without changing gameplay state");
        for(int n=0;n<35&&!original.Result();++n){AdvanceAuto(original);AdvanceAuto(restored);}
        original.Save();restored.Save();Check(Read(save)==Read(copy),"continued play preserves both RNG streams, offers, gold and cards");
        original.NewRun();original.ChooseNode(0);original.Save();const auto idle=Read(save);original.Play(0);
        Check(!original.Save()&&Read(save)==idle,"saving during a card animation preserves the last checkpoint");
        Check(!original.Load()&&original.Busy(),"loading cannot interrupt a pending impact");
        Settle(original);Check(Read(save)!=idle,"autosave commits after the action has settled");
        original.NewRun();original.ChooseNode(0);original.state.rage=100;original.Save();const auto before_cinematic=Read(save);original.Ultimate();
        Check(!original.Save()&&Read(save)==before_cinematic,"saving cannot serialize half of a cinematic");original.SkipCinematic();original.Update(0);
        Check(Read(save)!=before_cinematic,"a skipped cinematic is saved after its single impact");
        const auto good=Read(save);Game receiver(config,timeline,false);receiver.NewRun(42);receiver.SetSaveFile(bad);
        const auto old_hand=receiver.state.hand;const auto old_seed=receiver.run.seed;
        auto reject=[&](Json data,const char* name){Write(bad,data);Check(!receiver.Load()&&receiver.run.seed==old_seed&&receiver.state.hand==old_hand,name);};
        auto broken=good;broken["version"]=99;reject(broken,"unknown save versions leave the running session untouched");
        broken=good;broken["combat"]["shuffle_rng"]=-1;reject(broken,"negative RNG state is rejected");
        broken=good;broken["combat"]["enemy_id"]="missing";reject(broken,"unknown saved enemies are rejected");
        broken=good;broken["combat"]["live"]["history"]=Json::array({"missing"});reject(broken,"unknown replay card history is rejected");
        broken=good;broken["combat"]["live"]["slots"]=Json::array({99});reject(broken,"invalid visual target permutation is rejected");
        broken=good;broken["combat"]["live"]["end_block"]=-1;reject(broken,"negative power counters are rejected");
        broken=good;broken["run"]["deck"].push_back("punch");reject(broken,"a saved combat must conserve the run deck");
        broken=good;broken["run"]["map"][0]["next"]=Json::array({999});reject(broken,"tampered map links are rejected");
        broken=good;broken["run"]["gold"]=-4;reject(broken,"negative saved gold is rejected");
        broken=good;broken["run"]["node"]=999;reject(broken,"out-of-range saved nodes are rejected");
        broken=good;broken["speed"]=4;reject(broken,"unsupported saved speed is rejected");
        broken=good;broken["run"]["place"]=int(Place::Fight);broken["run"]["rewards"]=Json::array();reject(broken,"a resolved nonboss fight cannot resume without its rewards");
        {std::ofstream out(bad);out<<"{ incomplete";}Check(!receiver.Load()&&receiver.run.seed==old_seed,"a truncated save does not replace the current game");
        original.Save();const auto intact=Read(save);std::filesystem::create_directory(save.string()+".tmp");
        Check(!original.Save()&&Read(save)==intact,"a failed temporary write keeps the previous save intact");
    }
    {
        for(NodeKind kind:{NodeKind::Event,NodeKind::Shop,NodeKind::Rest}){
            Game g(config,timeline,false),h(config,timeline,false);Check(Reach(g,kind),"a real path reaches each noncombat screen");
            g.SetSaveFile(save);h.SetSaveFile(save);Check(g.Save()&&h.Load()&&h.run.place==g.run.place&&h.run.node==g.run.node,"noncombat screens resume on the same node");
            h.SetSaveFile(copy);Check(h.Save()&&Read(save)==Read(copy),"event choices, unsold shop and rest state round trip");
            if(kind==NodeKind::Shop){g.run.gold=300;Check(g.Buy(0)&&g.Save()&&h.Load(),"shop purchases can be saved");}
        }
        Game run(config,timeline,false),h(config,timeline,false);run.NewRun(1337);run.SetSaveFile(save);h.SetSaveFile(save);bool reward=false,phase_two=false,finished=false;
        for(int i=0;i<1600&&!run.Result();++i){
            if(!reward&&run.Reward()){reward=run.Save()&&h.Load()&&h.Reward()&&h.run.rewards==run.run.rewards;}
            if(!phase_two&&run.CanAct()&&run.state.PhaseIndex()==1)phase_two=run.Save()&&h.Load()&&h.state.PhaseIndex()==1&&h.state.PhaseName()==run.state.PhaseName();
            if(!AdvanceAuto(run))break;
        }
        if(run.Result())finished=run.Save()&&h.Load()&&h.Result()&&h.run.victory==run.run.victory;
        Check(reward,"unclaimed card rewards survive reload");Check(phase_two,"boss phase two survives reload");Check(finished,"finished runs reload as results rather than awarding twice");
        if(finished){auto invalid=Read(save);invalid["run"]["place"]=int(Place::Map);invalid["run"]["victory"]=false;Write(bad,invalid);h.SetSaveFile(bad);Check(!h.Load()&&h.Result(),"a completed boss cannot reload as an empty map");}
    }
    for(const auto& file:{save,copy,bad}){std::error_code ignored;std::filesystem::remove(file,ignored);}
}

void SimulateRuns(const Config& config,const Timeline& timeline,int count){
    int wins=0,stalled=0,total_hp=0,actions=0;
    Json classes=Json::object();std::map<int,int> failures_by_act;
    for(int seed=0;seed<count;++seed){
        const auto& character=config.classes[seed%config.classes.size()].id;
        auto& result=classes[character];if(result.is_null())result={{"runs",0},{"wins",0},{"losses",0},{"stalled",0}};
        result["runs"]=result["runs"].get<int>()+1;
        Game g(config,timeline,false);g.NewRun(unsigned(seed),character);int n=0;
        for(;n<4000&&!g.Result();++n)if(!AdvanceAuto(g))break;
        if(!g.Result()){++stalled;result["stalled"]=result["stalled"].get<int>()+1;}
        if(g.run.victory){++wins;total_hp+=g.run.hp;result["wins"]=result["wins"].get<int>()+1;}
        else if(g.Result()){result["losses"]=result["losses"].get<int>()+1;++failures_by_act[g.run.act+1];}
        actions+=n;
    }
    const Json report={{"runs",count},{"wins",wins},{"losses",count-wins-stalled},{"stalled",stalled},{"winPercent",100.0*wins/count},{"averageWinningHp",wins?double(total_hp)/wins:0},{"actions",actions},{"classes",classes},{"lossesByAct",failures_by_act},{"difficulty",0},{"metaSkills",false},{"policy","greedy heuristic; not a human win rate"}};
    Write("artifacts/simulation.json",report);std::cout<<"SIMULATION "<<report.dump()<<"\n";
    Check(stalled==0,"autoplay never stalls across the requested seeds");
}
