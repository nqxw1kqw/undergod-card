#include "game.hpp"
#include "quest_json.hpp"
#include <fstream>
#include <set>
using namespace undergod;
using Json=nlohmann::json;
extern void Check(bool,const char*);
namespace {
void End(Combat& c){c.EndPlayer();c.ActEnemy();}
void Settle(Game& g){for(int i=0;i<120&&g.Busy();++i)g.Update(.1f);}
std::string Read(const std::filesystem::path& p){std::ifstream in(p);return {std::istreambuf_iterator<char>(in),{}};}
}

void TigerChecks(const Config& config,const Timeline& timeline){
    const auto& tiger=FindEnemy(config,"tiger");
    Check(tiger.role=="boss"&&tiger.sprite=="tiger"&&tiger.hp==200&&tiger.phases.size()==1,"tiger is a distinct two-phase boss with two hundred HP");
    std::set<std::string> boss_ids;unsigned tiger_seed=0;bool found=false;
    for(unsigned seed=0;seed<200;++seed){const auto boss=GenerateMap(config,seed,1).back().encounter_id;boss_ids.insert(boss);if(boss=="tiger"){found=true;tiger_seed=seed;}}
    Check(found&&boss_ids==std::set<std::string>{"snake","hawk","tiger"},"all three act-two bosses are naturally reachable by seed");
    bool bounded=true;
    for(unsigned seed=0;seed<80;++seed){Combat c(config);c.BeginEncounter(tiger,seed,50,50,0,StartingDeck(config));bounded&=c.intent.attack&&((c.intent.hits==1&&c.intent.value>=10&&c.intent.value<=13)||(c.intent.hits==2&&c.intent.value>=4&&c.intent.value<=6));}
    Check(bounded,"tiger opening attacks use the configured physical and multi-hit ranges");
    Combat c(config);c.BeginEncounter(tiger,1337,50,50,0,StartingDeck(config));c.enemy.hp=101;c.enemy.status.poison=3;c.hand={"punch"};c.energy=1;c.Play(0);
    Check(c.PhaseIndex()==1&&c.enemy.hp==95&&c.enemy.status.strength==2&&c.enemy.status.poison==3,"tiger enrages below half HP without healing or erasing debuffs");
    Check(c.intent.kind==IntentKind::Charge&&c.live.charge_left==2&&!c.intent.attack,"tiger announces its pounce two player turns in advance");
    const int hp=c.player.hp;c.enemy.status.poison=0;End(c);Check(c.player.hp==hp&&c.live.charge_left==1&&c.intent.attack,"first charge turn waits and changes to attack intent");
    c.player.block=7;const int expected=std::max(0,c.IncomingDamage()-7);End(c);Check(c.player.hp==hp-expected&&c.live.charge_left==2,"pounce respects armor and starts its next announced windup");
    Combat restored(config);restored.Restore(c.Snapshot(),c.Mods());Check(Json(restored.Snapshot())==Json(c.Snapshot()),"tiger phase strength charge and RNG restore exactly");
    c.BeginEncounter(tiger,4,50,50,0,StartingDeck(config));c.intent={};c.intent.kind=IntentKind::Buff;c.intent.status=StatusId::Strength;c.intent.status_amount=2;c.intent.value=6;End(c);Check(c.enemy.status.strength==2&&c.enemy.block==6,"tiger roar really grants strength and armor");
    c.BeginEncounter(tiger,4,50,50,0,StartingDeck(config));c.player.block=5;c.intent={true,6};c.intent.kind=IntentKind::MultiAttack;c.intent.hits=2;End(c);Check(c.player.hp==43,"tiger twin claw spends block separately for each hit");
    int bosses=0;for(const auto& e:config.bestiary)bosses+=e.role=="boss";bool achievement=false;for(const auto& a:config.achievements)if(a.id=="all_bosses")achievement=a.threshold==bosses&&bosses==6;Check(achievement,"all-boss achievement includes the sixth boss");

    const std::filesystem::path session="artifacts/tiger-session.json",profile="artifacts/tiger-profile.json";
    Game g(config,timeline);g.SetSaveFile(session);g.SetProfileFile(profile);g.profile.feathers=100;g.NewRun(42);Check(g.Save(),"practice fixture has a normal saved campaign");
    const auto saved_session=Read(session),saved_profile=Read(profile);const auto counter=g.profile.next_run;const int feathers=g.profile.feathers;
    g.StartBossPractice("tiger",1337);Check(g.Practice()&&g.CanAct()&&g.state.EnemyId()=="tiger"&&g.state.enemy.hp==200,"boss practice opens the real tiger fight");Check(g.run.potions.size()==2&&FindCard(config,g.run.deck.front()).upgraded,"practice supplies upgraded starter deck and two potions");Check(!g.Save()&&Read(session)==saved_session,"practice cannot replace the real campaign save");
    Check(!g.BuySkill("chick_spear_1")&&g.profile.feathers==feathers,"practice cannot spend persistent skill currency");
    g.state.enemy.hp=1;g.state.hand={"punch"};g.state.energy=1;g.Play(0);Settle(g);g.Update(1);
    Check(g.Result()&&g.run.victory&&g.profile.feathers==feathers&&g.profile.next_run==counter&&Read(profile)==saved_profile,"practice victory awards no meta and does not write profile");
    Check(g.Load()&&!g.Practice()&&g.Map()&&g.run.seed==42,"F9 leaves practice and resumes the original campaign");
    g.StartBossPractice("tiger");g.SelectClass(1);g.ToggleAudio();Check(g.Load()&&g.profile.selected_class=="chick"&&!g.profile.muted,"leaving practice restores profile preferences too");g.Update(0);Check(Read(profile)==saved_profile,"preview menu choices cannot leak into persisted profile after F9");
    g.StartBossPractice("tiger");g.OpenClasses();g.SelectClass(1);g.PracticeSelected(8);Check(g.Practice()&&g.run.class_id=="mage"&&g.state.player.max_hp==40&&g.state.EnemyId()=="tiger","practice class menu can rematch tiger with another class");
    g.OpenClasses();g.BeginSelected(9);Check(!g.Practice()&&g.Map()&&g.run.class_id=="mage","normal start button exits practice and begins a campaign");

    // A saved pre-expansion act-two route keeps its valid original boss choice.
    auto legacy=config;legacy.acts[1].bosses={"snake","hawk"};g.NewRun(tiger_seed);g.run.act=1;g.run.map=GenerateMap(legacy,tiger_seed,1);const auto kept_boss=g.run.map.back().encounter_id;
    Check(g.Save()&&g.Load()&&g.Map()&&g.run.map.back().encounter_id==kept_boss,"old snake or hawk routes still load after expanding the boss pool");
    auto invalid=Json::parse(Read(session));invalid["run"]["map"].back()["encounter_id"]="undergod";{std::ofstream out(session);out<<invalid.dump();}
    Check(!g.Load()&&g.run.map.back().encounter_id==kept_boss,"compatibility does not allow replacing act-two boss with an unrelated boss");
    for(const auto& p:{session,profile}){std::error_code ec;std::filesystem::remove(p,ec);}
}
