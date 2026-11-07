#include "game.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <set>
using namespace undergod;
int checks=0,failures=0;
void Check(bool value,const char* name){++checks;if(!value){++failures;std::cout<<"FAIL "<<name<<"\n";}}
void Tick(Game& game,float logical_seconds){game.Update(logical_seconds/game.PlaybackRate());}
void Drain(Game& game){for(int i=0;i<100&&game.Busy();++i)game.Update(.1f);Check(!game.Busy(),"controller settles");}
int main(int,char** argv){
    std::filesystem::current_path(std::filesystem::absolute(argv[0]).parent_path());
    const auto config=LoadConfig();const auto timeline=LoadTimeline();
    {
        Combat c(config);Check(c.player.hp==50&&c.enemy.hp==120,"requested HP");Check(c.energy==3&&c.hand.size()==5&&c.draw.size()==5,"opening resources and piles");
        Check(c.hand==std::vector<std::string>({"punch","cookie","shield","punch","punch"}),"opening hand matches TypeScript seed 1337");
        Check(config.cards.size()==4&&c.config.balance.ultimate_damage==40,"JSON definitions");
        Rng r;r.Seed(42);const double expected[]={.6011037519201636,.44829055899754167,.8524657934904099,.6697340414393693};for(double value:expected)Check(std::abs(r.Next()-value)<1e-12,"RNG matches TypeScript golden values");
        auto legacy=config;legacy.enemy.opening_attack=false;legacy.enemy.max_guards=100;Combat original(legacy);
        const bool attacks[]={false,false,false,false,false,true,false,false};for(bool attack:attacks){Check(original.intent.attack==attack,"legacy intent RNG matches TypeScript");if(attack)Check(original.intent.value==8,"legacy intent damage golden value");original.EndPlayer();original.ActEnemy();}
    }
    {
        Fighter f{10,10,5};auto hit=ResolveDamage(f,8);Check(hit.lost==3&&hit.blocked==5&&f.hp==7&&f.block==0,"damage through armor");hit=ResolveDamage(f,99);Check(hit.lost==7&&f.hp==0,"lethal clamps actual HP loss");
        f={10,10,5};hit=ResolveDamage(f,-2);Check(hit.lost==0&&f.block==5&&f.hp==10,"negative damage cannot heal or grant block");
    }
    {
        Combat c(config);c.hand={"cookie"};c.player.hp=48;c.Play(0);Check(c.player.hp==50&&c.energy==1,"heal caps and pays cost");
        c.Reset();c.hand={"heavy"};c.energy=1;Check(!c.Play(0)&&c.hand.size()==1&&c.enemy.hp==120,"insufficient energy does nothing");Check(!c.Play(-1)&&!c.Play(99),"invalid card indices");
        c.Reset();c.hand={"punch","shield"};c.rage=98;c.Play(0);Check(c.rage==100,"attack rage cap");c.Play(0);Check(c.rage==100&&c.player.block==5,"defense rage cap");
        c.Reset();c.hand={"punch","shield"};c.Play(0);c.Play(0);Check(c.rage==13,"attack and defense rage gains");
    }
    {
        Combat c(config);c.player.block=20;c.intent={true,8};c.EndPlayer();Check(!c.Play(0)&&!c.EndPlayer(),"enemy phase rejects player actions");c.ActEnemy();Check(c.player.hp==50&&c.player.block==0&&c.energy==3&&c.turn==2,"block survives enemy hit then clears on player start");
        c.Reset();c.intent={false,6};c.EndPlayer();c.ActEnemy();Check(c.enemy.block==6,"enemy block survives into player phase");c.intent={true,8};c.EndPlayer();c.ActEnemy();Check(c.enemy.block==0,"enemy block clears on own action");
        c.Reset();c.hand={"punch"};c.enemy.hp=1;c.Play(0);Check(c.phase==Phase::Won&&!c.EndPlayer(),"victory and terminal lock");
        c.Reset();c.player.hp=1;c.intent={true,8};c.EndPlayer();c.ActEnemy();Check(c.phase==Phase::Lost&&c.player.hp==0,"defeat");
    }
    {
        Combat c(config);c.hand.clear();c.draw={"punch"};c.discard={"shield","cookie"};c.Draw(5);std::multiset<std::string> got(c.hand.begin(),c.hand.end());Check(got==std::multiset<std::string>({"punch","shield","cookie"})&&c.discard.empty()&&c.draw.empty(),"draw refills from discard without duplicating cards");
        c.Reset();c.player.hp=c.player.max_hp=500;for(int n=0;n<8;++n){if(c.CanPlay(0))c.Play(0);c.EndPlayer();c.ActEnemy();Check(c.hand.size()+c.draw.size()+c.discard.size()==10,"deck conserves cards across repeated reshuffles");}
        Combat a(config),b(config);a.Draw(20);a.EndPlayer();a.ActEnemy();b.EndPlayer();b.ActEnemy();Check(a.intent.attack==b.intent.attack&&a.intent.value==b.intent.value,"shuffle stream cannot perturb intent RNG");
    }
    {
        Combat c(config);Check(!c.StartUltimate(),"ultimate requires full rage");c.rage=100;c.enemy.block=6;Check(c.StartUltimate()&&c.rage==0&&!c.Play(0),"ultimate consumes rage and locks cards");Check(c.enemy.hp==120,"no ultimate damage at start");c.UltimateImpact();Check(!c.UltimateImpact()&&c.enemy.hp==86,"ultimate impact resolves once through armor");c.FinishUltimate();Check(c.phase==Phase::Player,"ultimate releases input after end");
        c.Reset();c.rage=100;c.StartUltimate();c.FinishUltimate();c.FinishUltimate();Check(c.enemy.hp==80,"skip before impact resolves once");
    }
    {
        TimelinePlayer player;int hits=0,ends=0,cues=0;player.Start(timeline,[&](const Step&){++cues;},[&]{++hits;},[&]{++ends;});player.Advance(2599);Check(hits==0,"timeline waits for 2600ms impact");player.Advance(2600);Check(hits==1,"timeline impact exact boundary");player.Advance(5000);player.Skip();Check(hits==1&&ends==1&&cues==8,"timeline ends once after skipped frames");
        for(float t:{0.f,900.f,2600.f,3400.f}){hits=ends=0;player.Start(timeline,[](const Step&){},[&]{++hits;},[&]{++ends;});player.Advance(t);player.Skip();player.Advance(9999);Check(hits==1&&ends==1,"skip before or after impact is idempotent");}
    }
    {
        Game g(config,timeline);g.NewRun();g.Play(0);Check(g.Busy()&&g.state.enemy.hp==120,"lunge defers resolution");Tick(g,.24f);Check(g.state.enemy.hp==120,"windup has no damage");Tick(g,.11f);Check(g.state.enemy.hp==114&&g.state.energy==2,"contact applies card");g.Play(0);Check(g.state.enemy.hp==114,"animation locks input");Drain(g);
        g.EndTurn();Tick(g,.8f);Check(g.Busy()&&g.state.turn==2,"controller holds lock through enemy recovery");Drain(g);
        g.NewRun();g.state.rage=100;g.state.enemy.block=6;g.Ultimate();Tick(g,2.59f);Check(g.state.enemy.hp==120&&g.Cinematic(),"cinematic delay");Tick(g,.02f);Check(g.state.enemy.hp==86,"cinematic callback applies damage");g.SkipCinematic();Check(g.CanAct()&&g.visual.zoom==1&&g.visual.dim==0&&g.state.enemy.hp==86,"skip cleans camera and unlocks once");
        g.NewRun();g.state.rage=100;g.state.enemy.hp=10;g.Ultimate();g.SkipCinematic();Check(g.Result()&&g.state.phase==Phase::Won,"skipped lethal ultimate shows result");
        g.NewRun();g.state.rage=100;g.Ultimate();g.NewRun();Tick(g,5);Check(g.state.enemy.hp==120&&g.CanAct()&&g.state.rage==0,"reset cancels pending cinematic safely");
        Game a(config,timeline),b(config,timeline);a.Play(0);b.Play(0);Tick(a,10);for(int i=0;i<100;++i)Tick(b,.1f);Check(a.state.enemy.hp==b.state.enemy.hp&&a.state.energy==b.state.energy,"frame cadence does not alter card outcome");
    }
    {
        Game g(config,timeline);g.state.rage=100;g.Ultimate();Tick(g,1.5f);
        Check(g.visual.spear.visible&&g.visual.spear.falling&&std::abs(g.visual.spear.age-.05f)<.001f&&std::abs(g.poses[0].age-1.25f)<.001f,"skipped frames sample spear trajectory and throw pose at absolute time");
        g.SkipCinematic();g.NewRun();g.state.enemy.block=10;g.Play(0);Tick(g,.4f);
        Check(g.state.enemy.hp==120&&!g.popups.empty()&&g.popups.back().blocked&&g.popups.back().amount==6,"fully blocked attack shows absorbed damage instead of HP loss");
    }
    {
        bool opening=true,streak_ok=true,range_ok=true;
        for(unsigned seed=0;seed<100;++seed){Combat c(config);c.player.hp=c.player.max_hp=10000;opening&=c.intent.attack;int guards=0;
            for(int turn=0;turn<20;++turn){guards=c.intent.attack?0:guards+1;streak_ok&=guards<=1;if(c.intent.attack)range_ok&=c.intent.value>=8&&c.intent.value<=13;c.EndPlayer();c.ActEnemy();}}
        Check(opening,"boss opens with attack across seeds");Check(streak_ok,"boss never spends consecutive turns guarding");Check(range_ok,"boss attack damage stays within configured range");
        Game g(config,timeline);const int incoming=g.state.intent.value;g.EndTurn();Tick(g,.69f);
        Check(g.poses[1].name=="attack"&&Game::EnemyHome+g.ActorOffset(true)-Game::PlayerHome<=175,"boss moves into contact range");
        Check(g.state.player.hp==50,"boss windup does not apply damage early");Tick(g,.09f);
        Check(g.state.player.hp==50-incoming&&g.poses[0].name=="hit","boss impact damages player and starts hit reaction");Tick(g,.6f);
        Check(g.CanAct()&&std::abs(g.ActorOffset(true))<.01f,"boss returns home and releases input");
        g.NewRun();g.Play(0);Tick(g,.24f);Check(Game::EnemyHome-(Game::PlayerHome+g.ActorOffset(false))<=165,"player spear lunge reaches opponent");
        Check(g.state.config.player_id=="chick","player asset identity comes from JSON");
    }
    {
        float durations[3]{};bool same_turn=true;Combat turn_reference(config);
        for(int speed=1;speed<=3;++speed){Game g(config,timeline);g.NewRun();g.SetSpeed(speed);g.Play(0);
            for(int i=0;i<300&&g.Busy();++i){g.Update(.01f);durations[speed-1]+=.01f;}
            Check(!g.Busy()&&g.state.enemy.hp==114&&g.state.energy==2,"speed changes duration without altering card result");
            g.EndTurn();for(int i=0;i<500&&g.Busy();++i)g.Update(.01f);
            if(speed==1)turn_reference=g.state;
            same_turn&=!g.Busy()&&g.state.player.hp==turn_reference.player.hp&&g.state.enemy.hp==turn_reference.enemy.hp&&g.state.energy==turn_reference.energy&&g.state.hand==turn_reference.hand&&g.state.intent.attack==turn_reference.intent.attack&&g.state.intent.value==turn_reference.intent.value;
        }
        Check(same_turn,"all speeds preserve boss damage, next hand, energy and RNG intent");
        Check(durations[0]>.95f&&durations[0]<1.05f,"default x1 has a slower readable action");
        Check(std::abs(durations[0]-durations[1]*2)<.035f&&std::abs(durations[0]-durations[2]*3)<.04f,"x2 and x3 scale the whole action clock");
        Game g(config,timeline);Check(g.Speed()==1&&!g.SetSpeed(0)&&!g.SetSpeed(4)&&g.Speed()==1,"speed defaults to x1 and rejects invalid values");
        g.SetSpeed(3);g.NewRun();Check(g.Speed()==1,"new run starts at slow x1");
    }
    {
        bool flight=true,impact=true,cleanup=true,skips=true;
        for(int speed=1;speed<=3;++speed){Game g(config,timeline);g.SetSpeed(speed);g.state.rage=100;g.state.enemy.block=6;g.Ultimate();
            Tick(g,.7f);flight&=!g.visual.spear.visible&&g.poses[0].name=="ult_throw";
            Tick(g,.1f);flight&=g.visual.spear.visible&&!g.visual.spear.falling&&g.SpearTip().second<110;
            Tick(g,1.f);flight&=g.visual.spear.falling&&std::abs(g.SpearTip().first-Game::EnemyHome)<.01f;
            Tick(g,.79f);impact&=g.state.enemy.hp==120&&g.SpearTip().second<135;
            Tick(g,.02f);impact&=g.state.enemy.hp==86&&std::abs(g.SpearTip().second-135)<.01f;
            Tick(g,1.1f);cleanup&=g.CanAct()&&!g.visual.spear.visible&&g.visual.zoom==1;
            for(float t:{0.f,.95f,2.55f,2.8f}){g.NewRun();g.SetSpeed(speed);g.state.rage=100;g.Ultimate();Tick(g,t);g.SkipCinematic();g.SkipCinematic();skips&=g.state.enemy.hp==80&&g.CanAct()&&!g.visual.spear.visible;}
        }
        Check(flight,"throw releases spear, rises into sky then falls over boss at every speed");
        Check(impact,"damage occurs exactly at spear contact at every speed");
        Check(cleanup&&skips,"normal end and all skip phases clean spear and resolve once");
        Game g(config,timeline);g.state.rage=100;g.Ultimate();Tick(g,2.f);const auto before=g.SpearTip();g.SetSpeed(3);
        Check(g.SpearTip()==before&&g.state.enemy.hp==120,"changing speed mid-flight causes no teleport or damage");
        Tick(g,.61f);Check(g.state.enemy.hp==80,"changed speed still reaches the single impact");
        g.NewRun();Check(!g.visual.spear.visible&&g.Speed()==1&&g.CanAct(),"reset cancels projectile and restores default speed");
    }
    {
        Game g(config,timeline);g.NewRun(42);const auto hand=g.state.hand;const auto intent=g.state.intent;
        g.Play(0);Tick(g,2);g.Retry();
        Check(g.Seed()==42&&g.state.hand==hand&&g.state.intent.value==intent.value,"retry reproduces seed");
        g.SetSessionSeed(42);g.NewMatch();const auto first=g.Seed();g.NewMatch();
        Check(g.Seed()!=first&&g.Seed()!=42,"fresh matches use distinct explicit seeds");
    }
    std::cout<<"RESULT "<<checks<<" checks, "<<failures<<" failures\n";return failures?1:0;
}
