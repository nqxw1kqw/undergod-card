#include "game.hpp"
#include "quest_json.hpp"
#include <cmath>
#include <fstream>
#include <set>
#include <iostream>
using namespace undergod;
extern void Check(bool,const char*);
namespace {
void Advance(Game& g,float seconds,float quantum=.01f){while(seconds>.00001f){const float dt=std::min(quantum,seconds);g.Update(dt/g.PlaybackRate());seconds-=dt;}}
void Settle(Game& g){for(int i=0;i<3000&&g.Busy();++i)Advance(g,.02f);Check(!g.Busy(),"integrated controller settles");}
Intent Wait(){Intent i;i.kind=IntentKind::Defend;return i;}
void Fixture(Game& g,std::vector<std::string> hand={"punch","punch","shield","cookie"}){
    g.StartBossPractice("tiger",1337);Settle(g);g.state.player.hp=g.state.player.max_hp=500;g.state.enemy.hp=g.state.enemy.max_hp=500;
    g.state.hand=std::move(hand);g.state.live.locked=-1;g.state.stars.enemy={{0,Wait(),true},{0,Wait(),true},{0,Wait(),true}};Advance(g,.25f);
}
void Attack(Game& g){g.Play(0);g.QueueWait();g.QueueWait();g.EndTurn();}
std::string State(const Game& g){return nlohmann::json(g.state.Snapshot()).dump();}
const CardVisual* Owned(const Game& g,int slot){for(const auto& c:g.cards)if(!c.enemy&&!c.leaving&&c.hand_index==slot)return &c;return nullptr;}
}
void IntegrationChecks(const Config& config,const Timeline& timeline){
    Check(config.cards.size()==98&&config.boss_cards.size()==36&&config.acts.size()==3,"integration retains full draft content");
    {
        Game g(config,timeline);g.NewRun(71,"mage",0);g.SetSessionSeed(9);g.NewMatch();const auto seed=g.Seed();const auto snapshot=State(g);g.Retry();
        Check(g.Seed()==seed&&State(g)==snapshot,"retry keeps the same seed class map and combat streams");g.NewMatch();Check(g.Seed()!=seed,"new match advances the separate session seed");
    }
    {
        Game g(config,timeline);Fixture(g,{"heavy+","cookie+","shield"});g.state.enemy.block=5;g.state.rage=99;const auto before=State(g);
        auto p=g.Preview(0);Check(p.cost==1&&p.damage==10&&p.absorbed==5&&p.rage==1,"resolver preview shows star cost actual HP absorption and capped rage");
        g.state.player.hp=498;auto heal=g.Preview(1);Check(heal.heal==2,"preview reports effective capped healing");g.state.player.hp=500;
        Check(State(g)==before,"preview preserves every saved combat field and RNG");g.state.live.locked=0;Check(!g.Preview(0).unavailable.empty(),"locked card preview explains its restriction");
    }
    {
        Game g(config,timeline);Fixture(g);std::set<unsigned> ids;for(const auto& c:g.cards)if(!c.leaving)ids.insert(c.id);
        Check(ids.size()==g.state.hand.size(),"duplicate card copies have distinct visual identities");const auto id=Owned(g,1)->id;
        g.Play(1);Check(Owned(g,1)&&Owned(g,1)->id==id&&Owned(g,1)->zone==CardZone::Queue,"queue moves the same owned card snapshot");
        g.UnqueueCard(0);Check(Owned(g,1)&&Owned(g,1)->id==id&&Owned(g,1)->zone==CardZone::Hand,"unqueue keeps identity during hand reflow");
        g.Play(1);g.QueueWait();g.QueueWait();g.EndTurn();Check(g.state.enemy.hp==500&&g.cards.size()>=4,"attack sweeps unselected cards but does not resolve damage early");
        bool leaving=false;for(const auto& c:g.cards)leaving|=c.leaving;Check(leaving,"unselected hand owns its discard animation");
        Advance(g,.15f);bool play=false;for(const auto& c:g.cards)play|=c.id==id&&c.zone==CardZone::Play;Check(play,"queued selected card travels through the play slot");
        Advance(g,.20f);bool discard=false;for(const auto& c:g.cards)discard|=c.id==id&&c.leaving&&c.snapshot.id=="punch";
        Check(discard&&g.state.enemy.hp==494,"resolved card owns a discard snapshot after core removes it");Settle(g);
    }
    {
        Game g(config,timeline);Fixture(g,{"punch"});g.state.enemy.block=4;Attack(g);Advance(g,.34f);bool blocked=false,hp=false;std::set<int> lanes;
        for(const auto& p:g.popups)if(p.enemy&&p.type==EventType::Damage){blocked|=p.blocked&&p.amount==4;hp|=!p.blocked&&p.amount==2;lanes.insert(p.lane);}
        Check(blocked&&hp&&lanes.size()==2,"partial absorption and HP loss have separate reserved popup lanes");
        Check(g.summary.cards==1&&g.summary.damage==2&&g.particles.size()<=64,"recap records actual event-owned effects and feedback is bounded");
        Settle(g);Advance(g,2);Check(g.particles.empty()&&g.popups.empty()&&g.visual.shake_left==0,"ordinary effects expire outside cinematics");
    }
    {
        Game g(config,timeline);Fixture(g);g.state.draw={"shield","punch","punch"};g.state.discard={"cookie","shield"};const auto before=State(g);
        const auto a=Game::PileCounts(g.state.draw);std::reverse(g.state.draw.begin(),g.state.draw.end());Check(a==Game::PileCounts(g.state.draw),"pile inspection exposes counts rather than draw order");std::reverse(g.state.draw.begin(),g.state.draw.end());
        g.TogglePile(1);g.Play(0);g.EndTurn();g.QueueWait();g.DrinkPotion(1);Check(g.pile_view==1&&State(g)==before,"pile modal prevents click-through cards Attack waits and potions");g.TogglePile(1);
        g.FocusCard(1);Check(g.focused_card==0,"keyboard focus starts on an actual available hand slot");g.Play(g.focused_card);g.FocusCard(1);Check(g.focused_card==1,"focus skips cards already queued");
    }
    {
        Game g(config,timeline);Fixture(g,{"punch"});g.state.enemy.hp=1;Attack(g);Advance(g,.34f);
        Check(g.terminal.active&&g.terminal.entries==1&&g.Busy()&&!g.run.victory,"fatal contact starts one locked terminal sequence");
        Advance(g,.85f);Check(g.ResultAlpha()>0&&g.Reaction(0).defeated&&!g.run.victory,"terminal fade and defeated actor finish before settlement");
        Advance(g,.8f);Check(g.run.victory&&g.terminal.settled&&g.summary.damage==1,"terminal recap uses effective damage and settles once");
        g.NewRun();Check(!g.terminal.active&&g.summary.cards==0&&g.particles.empty()&&g.cards.empty(),"new run clears terminal cards recap and feedback");
    }
    {
        Game g(config,timeline);Fixture(g);g.state.enemy.hp=1;g.state.rage=100;g.Ultimate();g.SkipCinematic();
        Check(g.terminal.active&&g.terminal.entries==1&&g.Busy()&&!g.run.victory,"skipped lethal ultimate enters the same locked terminal sequence once");
        const auto damage=g.summary.damage;g.SkipCinematic();Settle(g);
        Check(g.run.victory&&g.summary.damage==damage&&damage==1&&g.summary.ultimates==1,"repeated ultimate skip neither duplicates damage nor terminal recap");
    }
    {
        Timeline t;t.steps={{0,Cue::Zoom,1.4f,1000},{500,Cue::Zoom,1.2f,800},{1000,Cue::Zoom,1,500},{1600,Cue::Impact},{2000,Cue::End}};
        Game fine(config,t),coarse(config,t);Fixture(fine);Fixture(coarse);fine.state.rage=coarse.state.rage=100;fine.Ultimate();coarse.Ultimate();
        Advance(fine,.49f);const float old=fine.visual.zoom;Advance(fine,.02f);Check(fine.visual.zoom>old-.01f&&fine.visual.zoom>1.25f,"interrupted zoom samples the current segment instead of snapping home");
        Advance(fine,.74f);Advance(coarse,1.25f,1.25f);Check(std::abs(fine.visual.zoom-coarse.visual.zoom)<.001f,"multiple zoom boundaries agree for fine and coarse updates");
        fine.SkipCinematic();coarse.SkipCinematic();Check(fine.visual.zoom==1&&coarse.visual.zoom==1,"skip restores the camera exactly once");
        Fixture(fine);fine.ToggleFeedback();fine.state.rage=100;fine.Ultimate();Advance(fine,.7f);Check(fine.visual.zoom==1,"reduced feedback suppresses camera movement");fine.SkipCinematic();
    }
    {
        Game g(config,timeline);g.StartBossPractice("tiger",1337);Settle(g);const auto standard=g.state.Snapshot();const auto seed=g.Seed();
        Check(g.SelectPracticeDifficulty(PracticeDifficulty::Gentle),"practice can select a gentle profile while idle");Settle(g);const auto gentle=g.state.Snapshot();
        Check(g.SelectPracticeDifficulty(PracticeDifficulty::Challenge),"practice can select a challenge profile while idle");Settle(g);const auto hard=g.state.Snapshot();
        Check(g.Seed()==seed&&gentle.hand==standard.hand&&hard.hand==standard.hand&&gentle.intent_rng==standard.intent_rng&&hard.intent_rng==standard.intent_rng,"practice profiles keep seed shuffle and intent identity streams");
        bool ordered=true;for(int i=0;i<3;++i){const auto& a=gentle.stars.enemy[i].action;const auto& b=standard.stars.enemy[i].action;const auto& c=hard.stars.enemy[i].action;ordered&=a.deck_card_id==b.deck_card_id&&b.deck_card_id==c.deck_card_id&&a.value<=b.value&&b.value<=c.value;}
        Check(ordered&&gentle.enemy.max_hp==hard.enemy.max_hp,"difficulty changes announced damage without replacing boss decks or HP");
        g.NewRun();Check(!g.SelectPracticeDifficulty(PracticeDifficulty::Gentle),"practice difficulty cannot modify campaign challenge progression");
    }
    {
        Game g(config,timeline);Fixture(g);g.ToggleFeedback();g.NewRun();Check(g.ReducedFeedback(),"reduced FX preference survives leaving practice without changing meta rewards");
        const auto file=std::filesystem::path("integration-profile-test.json");std::string error;Check(SaveProfile(file,g.profile,error),"profile writes optional reduced FX preference");Profile loaded;Check(LoadProfile(file,loaded,config,error)&&loaded.reduced_feedback,"profile reload restores reduced FX preference");std::filesystem::remove(file);
    }
    std::cout<<"INTEGRATION presentation preview cards terminal camera seed profiles passed\n";
}
