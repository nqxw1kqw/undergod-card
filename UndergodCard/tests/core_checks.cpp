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
        TimelinePlayer player;int hits=0,ends=0,cues=0;player.Start(timeline,[&](const Step&){++cues;},[&]{++hits;},[&]{++ends;});player.Advance(2599);Check(hits==0,"timeline waits for 2600ms impact");player.Advance(2600);Check(hits==1,"timeline impact exact boundary");player.Advance(5000);player.Skip();Check(hits==1&&ends==1&&cues==9,"timeline ends once after skipped frames");
        for(float t:{0.f,900.f,2600.f,3400.f}){hits=ends=0;player.Start(timeline,[](const Step&){},[&]{++hits;},[&]{++ends;});player.Advance(t);player.Skip();player.Advance(9999);Check(hits==1&&ends==1,"skip before or after impact is idempotent");}
    }
    {
        Game g(config,timeline);g.NewRun();g.Play(0);Check(g.Busy()&&g.state.enemy.hp==120,"lunge defers resolution");Tick(g,.24f);Check(g.state.enemy.hp==120,"windup has no damage");Tick(g,.11f);Check(g.state.enemy.hp==114&&g.state.energy==2,"contact applies card");g.Play(0);Check(g.state.enemy.hp==114,"animation locks input");Drain(g);
        g.EndTurn();Tick(g,.8f);Check(g.Busy()&&g.state.turn==2,"controller holds lock through enemy recovery");Drain(g);
        g.NewRun();g.state.rage=100;g.state.enemy.block=6;g.Ultimate();Tick(g,2.59f);Check(g.state.enemy.hp==120&&g.Cinematic(),"cinematic delay");Tick(g,.02f);Check(g.state.enemy.hp==86,"cinematic callback applies damage");g.SkipCinematic();Check(g.CanAct()&&g.visual.zoom==1&&g.visual.dim==0&&g.state.enemy.hp==86,"skip cleans camera and unlocks once");
        g.NewRun();g.state.rage=100;g.state.enemy.hp=10;g.Ultimate();g.SkipCinematic();Check(g.terminal.active&&g.Busy()&&g.state.phase==Phase::Won,"skipped lethal ultimate starts terminal transition");
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
    {
        Combat c(config);c.rage=98;c.player.hp=48;
        Check(c.RageGain(c.Definition("heavy"))==2,"rage preview caps at remaining capacity");
        Check(c.EffectiveHeal(c.Definition("cookie"))==2,"heal preview caps at missing HP");
        c.player.hp=50;Check(c.EffectiveHeal(c.Definition("cookie"))==0,"full health preview is zero");
    }
    {
        Game g(config,timeline);g.NewRun(42);Combat expected=g.state;const auto intent=g.state.intent;
        expected.EndPlayer();expected.ActEnemy();g.EndTurn();Tick(g,.769f);
        Check(g.state.turn==1&&!g.ShowHand(),"enemy contact boundary holds old turn");
        Tick(g,.002f);Check(g.state.turn==2&&g.DisplayIntent().value==intent.value&&!g.ShowHand(),"next hand and intent hidden at contact");
        Tick(g,.378f);Check(g.EnemyResolving()&&!g.CanAct(),"recovery remains locked before boundary");
        Tick(g,.002f);Check(g.Transition()&&g.ShowHand()&&!g.CanAct(),"recovery reveals next hand in locked transition");
        Tick(g,.198f);Check(!g.CanAct(),"transition locks until exact duration");
        Tick(g,.002f);Check(g.CanAct()&&g.state.hand==expected.hand&&g.state.intent.value==expected.intent.value,"transition does not draw or roll twice");
        g.NewRun();g.state.player.hp=1;g.EndTurn();Tick(g,1.151f);
        Check(g.terminal.active&&!g.Transition(),"lethal enemy skips incoming player transition");
        g.NewRun();Check(g.CanAct()&&!g.Transition(),"reset clears transition");
    }
    {
        Game g(config,timeline);g.NewRun();std::set<unsigned> ids;
        for(const auto& c:g.cards)ids.insert(c.id);
        Check(ids.size()==5&&g.cards[0].card==g.state.hand[0],"duplicate cards have distinct owned visual identities");
        const auto selected=g.cards[0].id;const auto survivor=g.cards[1].id;
        g.Play(0);Tick(g,.319f);
        Check(g.cards[0].selected&&g.cards[0].id==selected&&g.state.hand.size()==5,"selected snapshot reserves slot until contact");
        Tick(g,.002f);
        bool leaving=false,stable=false;
        for(const auto& c:g.cards){if(c.id==selected)leaving=c.leaving&&c.card=="punch";if(c.id==survivor)stable=c.slot==0;}
        Check(leaving&&stable&&g.state.hand.size()==4,"contact moves selected snapshot to discard and preserves survivor identity");
        Tick(g,.33f);Check(g.cards.size()==4&&g.CanAct(),"discard snapshot expires and hand reflow settles before unlock");
        const auto hp=g.state.enemy.hp;Tick(g,2);Check(g.state.enemy.hp==hp,"visual lifecycle never replays card");
        g.EndTurn();Check(g.state.hand.empty()&&g.cards.size()==4&&g.cards[0].leaving,"end turn retains departing owned snapshots");
        Tick(g,.4f);Check(g.cards.empty(),"end turn discard snapshots expire");
        Tick(g,.751f);Check(g.Transition()&&g.cards.size()==5&&g.cards[0].Bounds().y<710,"incoming hand starts at draw pile after recovery");
        Tick(g,.20f);bool settled=true;for(const auto& c:g.cards)settled&=std::abs(c.Bounds().y-710)<.01f;
        Check(g.CanAct()&&settled,"staggered draw settles before input unlock");
        g.Play(0);g.NewRun(42);Check(g.cards.size()==5&&g.cards[0].id==1&&!g.cards[0].leaving&&!g.Busy(),"reset clears all departing and selected snapshots");
        g.state.hand={"shield"};g.Play(0);
        Check(g.state.player.block==5&&g.cards[0].via_play,"nonattack keeps immediate rules with a separate play trajectory");
        Tick(g,.20f);Check(std::abs(g.cards[0].Bounds().y-485)<.01f,"nonattack visual reaches play slot");
        Tick(g,.34f);Check(g.cards.empty()&&g.CanAct(),"nonattack visual discards before unlock");
        bool equivalent=true;
        for(int speed=1;speed<=3;++speed){
            Game a(config,timeline),b(config,timeline);a.NewRun();b.NewRun();a.SetSpeed(speed);b.SetSpeed(speed);
            a.Play(0);b.Play(0);Tick(a,2);for(int i=0;i<200;++i)Tick(b,.01f);
            equivalent&=a.cards.size()==b.cards.size()&&a.state.hand==b.state.hand&&a.state.enemy.hp==b.state.enemy.hp;
            for(size_t i=0;i<a.cards.size();++i)equivalent&=a.cards[i].id==b.cards[i].id&&std::abs(a.cards[i].Bounds().x-b.cards[i].Bounds().x)<.01f;
            a.EndTurn();b.EndTurn();Tick(a,2);for(int i=0;i<200;++i)Tick(b,.01f);
            equivalent&=a.cards.size()==5&&b.cards.size()==5&&a.state.hand==b.state.hand&&a.state.player.hp==b.state.player.hp;
        }
        Check(equivalent,"card snapshots and combat agree across fine/coarse cadence and all speeds");
    }
    {
        Game g(config,timeline);g.NewRun();g.state.hand={"shield"};g.Play(0);
        Tick(g,.449f);Check(!g.CanAct(),"short support recovery stays locked before end");
        Tick(g,.002f);Check(g.CanAct(),"support recovery unlocks at .45");
        g.NewRun();g.Play(0);Tick(g,.31f);auto x=g.ActorOffset(false);auto frame=g.PoseFrame(false);g.SetSpeed(3);
        Check(x==g.ActorOffset(false)&&frame==g.PoseFrame(false),"speed change preserves action sample");
        Tick(g,.10f);Check(g.poses[1].age>.08f&&g.popups[0].age>.08f,"contact effects include update overshoot");
    }
    {
        Game g(config,timeline);g.NewRun();g.state.hand={"punch"};g.state.enemy.block=2;g.Play(0);Tick(g,.321f);
        Check(g.popups.size()==2&&g.popups[0].amount==2&&g.popups[1].amount==4&&g.popups[0].lane!=g.popups[1].lane,"partial block reports both amounts in separate lanes");
        Check(!g.particles.empty()&&g.particles.size()<=64&&g.visual.shake_left>0,"ordinary contact has bounded feedback");
        Tick(g,2);Check(g.particles.empty()&&g.popups.empty()&&g.visual.shake_left==0&&g.visual.flash_left==0,"ordinary effects expire outside cinematic");
        g.NewRun();g.state.hand={"shield","cookie"};g.Play(0);Tick(g,.46f);g.Play(0);
        Check(!g.particles.empty(),"shield and heal produce pulses");
        g.NewRun();Check(g.particles.empty()&&g.popups.empty()&&g.visual.shake_left==0,"reset clears feedback");
        for(int i=0;i<100;++i){g.state.events.push_back({EventType::Block,false,5});}
        g.EndTurn();
        Check(g.particles.size()==64&&g.popups.size()==8,"feedback caps particles and popup lanes under burst load");
        g.visual.flash_left=.1f;g.visual.flash_duration=.1f;Tick(g,.2f);
        Check(g.visual.flash_left==0,"flash expires without active cinematic");
        bool same=true;
        for(int speed=1;speed<=3;++speed){Game a(config,timeline),b(config,timeline);a.NewRun(12);b.NewRun(12);a.SetSpeed(speed);b.SetSpeed(speed);a.reduced_feedback=true;
            a.Play(0);b.Play(0);Tick(a,1);for(int i=0;i<100;++i)Tick(b,.01f);a.EndTurn();b.EndTurn();Tick(a,2);Tick(b,2);
            same&=a.state.hand==b.state.hand&&a.state.intent.value==b.state.intent.value&&a.state.player.hp==b.state.player.hp&&a.particles.empty()&&b.particles.empty();}
        Check(same,"feedback settings and cadence preserve combat RNG at all speeds");
    }
    {
        Combat c(config);c.difficulty=Difficulty::Challenge;c.Reset(17);const auto advertised=c.intent;
        c.enemy.hp=72;c.hand={"punch"};c.Play(0);
        Check(c.intent.attack==advertised.attack&&c.intent.value==advertised.value&&c.stage_entries==0,"crossing threshold preserves advertised intent");
        const int hp=c.player.hp;c.EndPlayer();c.ActEnemy();
        Check(c.player.hp==hp-advertised.value&&c.stage_entries==1&&c.intent.prepare&&!c.intent.attack,"phase begins once after advertised action resolves");
        const int prepared_hp=c.player.hp;c.EndPlayer();c.ActEnemy();
        Check(c.player.hp==prepared_hp&&c.intent.attack&&c.intent.charged&&c.intent.value==34,"preparation is harmless and announces exact charged damage");
        c.player.block=10;c.EndPlayer();c.ActEnemy();
        Check(c.player.hp==prepared_hp-24&&c.stage_entries==1,"charged hit uses ordinary block once");
        c.player.hp=500;for(int i=0;i<12;++i){c.EndPlayer();c.ActEnemy();}
        Check(c.stage_entries==1,"repeating pattern never reenters stage");
        c.Reset(17);Check(c.stage_entries==0&&!c.intent.prepare&&!c.intent.charged,"reset clears stage and pattern");
        bool reproducible=true,isolated=true;
        for(auto d:{Difficulty::Gentle,Difficulty::Standard,Difficulty::Challenge}){
            Combat a(config),b(config);a.difficulty=b.difficulty=d;a.Reset(42);b.Reset(42);a.enemy.hp=b.enemy.hp=60;a.player.hp=b.player.hp=1000;
            for(int i=0;i<20;++i){reproducible&=a.intent.attack==b.intent.attack&&a.intent.value==b.intent.value&&a.intent.prepare==b.intent.prepare&&a.intent.charged==b.intent.charged;
                a.Draw(2);a.EndPlayer();b.EndPlayer();a.ActEnemy();b.ActEnemy();}
            isolated&=reproducible;
        }
        Check(reproducible&&isolated,"profile intents reproducible and independent of extra shuffle draws");
        Combat standard(config),challenge(config);challenge.difficulty=Difficulty::Challenge;challenge.Reset(1337);
        Check(standard.hand==challenge.hand&&standard.intent.value==challenge.intent.value,"profile choice preserves shuffle stream and challenge opening");
        standard.enemy.hp=1;standard.EndPlayer();standard.ActEnemy();
        Check(standard.stage_entries==0&&!standard.intent.prepare&&!standard.intent.charged,"standard never escalates even below threshold");
        Game g(config,timeline);g.NewRun(42);
        Check(!g.SelectDifficulty(static_cast<Difficulty>(99)),"profile selection rejects out of range");
        Check(g.SelectDifficulty(Difficulty::Gentle)&&g.Seed()==42&&g.state.difficulty==Difficulty::Gentle,"profile selection retries explicit seed");
        g.Play(0);Check(!g.SelectDifficulty(Difficulty::Challenge),"profile selection respects action input lock");
        g.NewRun(42);g.Retry();Check(g.state.difficulty==Difficulty::Gentle,"retry retains selected profile");
        auto invalid=config;invalid.enemy.stage_threshold_percent=100;bool rejected=false;
        try{Combat bad(invalid);}catch(const std::runtime_error&){rejected=true;}
        Check(rejected,"profile threshold bounds validated");
        invalid=config;invalid.enemy.challenge_charge=0;rejected=false;
        try{Combat bad(invalid);}catch(const std::runtime_error&){rejected=true;}
        Check(rejected,"charged damage bounds validated");
        bool cadence=true;
        for(int speed=1;speed<=3;++speed){Game a(config,timeline),b(config,timeline);a.SelectDifficulty(Difficulty::Challenge);b.SelectDifficulty(Difficulty::Challenge);a.state.enemy.hp=b.state.enemy.hp=60;a.SetSpeed(speed);b.SetSpeed(speed);
            for(int turn=0;turn<2;++turn){a.EndTurn();b.EndTurn();Tick(a,1.4f);for(int i=0;i<140;++i)Tick(b,.01f);}
            cadence&=a.state.hand==b.state.hand&&a.state.intent.charged&&b.state.intent.charged&&a.state.player.hp==b.state.player.hp&&a.state.stage_entries==1&&b.state.stage_entries==1;}
        Check(cadence,"stage preparation preserves fine/coarse and speed equivalence");
    }
    {
        Combat c(config);c.difficulty=Difficulty::Challenge;c.Reset(33);c.enemy.hp=73;c.EndPlayer();c.ActEnemy();
        Check(c.stage_entries==0,"stage stays inactive immediately above threshold");
        c.enemy.hp=72;c.EndPlayer();c.ActEnemy();Check(c.stage_entries==1&&c.intent.prepare,"stage enters at exact threshold");
        bool once=true,clean=true;
        for(auto d:{Difficulty::Gentle,Difficulty::Standard,Difficulty::Challenge})for(int speed=1;speed<=3;++speed){
            Game g(config,timeline);g.SelectDifficulty(d);g.SetSpeed(speed);g.state.rage=100;g.Ultimate();Tick(g,2.61f);g.SkipCinematic();g.SkipCinematic();
            once&=g.state.enemy.hp==80&&g.state.stage_entries==0&&g.particles.empty()&&g.visual.shake_left==0;
            g.state.enemy.hp=60;g.EndTurn();Tick(g,1.4f);const int hp=g.state.player.hp;g.EndTurn();Tick(g,2);Tick(g,2);
            once&=d==Difficulty::Standard||g.state.player.hp==hp;
            g.NewRun();clean&=g.CanAct()&&g.state.stage_entries==0&&g.particles.empty()&&g.popups.empty()&&!g.visual.spear.visible;
        }
        Check(once,"ultimate and preparation remain exactly once across profiles and speeds");
        Check(clean,"all profile resets clean locks and presentation");
    }
    {
        Game g(config,timeline);g.NewRun(71);g.state.enemy.hp=1;g.state.hand={"punch"};g.Play(0);Tick(g,.32f);
        Check(g.state.phase==Phase::Won&&g.terminal.active&&g.Busy()&&!g.Result(),"lethal core immediate terminal presentation locked");
        Check(g.summary.cards==1&&g.summary.damage==1&&g.summary.turns==1&&g.summary.hp==50&&g.summary.seed==71,"terminal summary counts resolved values");
        g.Play(0);g.EndTurn();g.Ultimate();Check(g.summary.cards==1&&g.terminal.entries==1,"terminal rejects further actions");
        Tick(g,2);Check(g.Result()&&!g.Busy()&&g.ResultAlpha()==1,"results settle and fully fade in");
        g.NewRun();Check(!g.terminal.active&&g.summary.cards==0&&g.summary.damage==0&&g.CanAct(),"reset clears terminal and summary");
        for(bool after:{false,true}){g.NewRun();g.state.enemy.hp=1;g.state.rage=100;g.Ultimate();if(after)Tick(g,2.61f);g.SkipCinematic();g.SkipCinematic();
            Check(g.terminal.entries==1&&g.summary.ultimates==1&&g.summary.damage==1&&g.Busy()&&g.visual.zoom==1,"lethal skip before or after impact enters terminal once");}
        g.NewRun();g.state.player.hp=1;g.state.player.block=2;g.EndTurn();Tick(g,.77f);
        Check(g.state.phase==Phase::Lost&&!g.terminal.enemy&&g.summary.blocked==2&&g.summary.hp==0,"enemy lethal summary and defeated actor");
        g.NewRun();g.state.hand={"cookie","shield"};g.state.player.hp=49;g.Play(0);Tick(g,1);g.Play(0);Tick(g,1);
        Check(g.summary.healed==1&&g.summary.cards==2,"support summary uses effective healing and played cards");
    }
    {
        Game g(config,timeline);g.NewRun();g.state.hand={"heavy","shield","cookie"};g.state.enemy.block=5;g.state.enemy.hp=3;g.state.rage=98;g.state.player.hp=49;
        auto p=g.Preview(0);Check(p.cost==2&&p.damage==3&&p.absorbed==5&&p.rage==2&&p.unavailable.empty(),"detail preview respects armor lethal clamp cost and rage cap");
        p=g.Preview(1);Check(p.block==5&&p.rage==2,"shield preview matches resolver");
        p=g.Preview(2);Check(p.heal==1&&p.rage==0,"heal preview caps actual HP restoration");
        const auto hand=g.state.hand;const auto draw=g.state.draw;
        g.Preview(0);Check(g.state.hand==hand&&g.state.draw==draw&&g.state.enemy.hp==3&&g.state.rage==98&&g.summary.cards==0,"preview leaves live state and accounting untouched");
        g.state.energy=0;Check(g.Preview(0).unavailable=="Not enough energy","detail explains energy restriction");
        g.state.energy=3;g.Play(0);Check(g.Preview(1).unavailable=="Wait for the current action","detail explains action lock");
        Tick(g,.32f);Check(g.Preview(0).unavailable=="Match finished","detail explains terminal restriction");
        Check(g.Preview(-1).unavailable=="No card selected"&&g.Preview(99).cost==0,"detail rejects invalid selection");
        const auto counts=Game::PileCounts({"shield","punch","shield"});
        Check(counts==Game::PileCounts({"shield","shield","punch"})&&counts.at("shield")==2&&Game::PileCounts({}).empty(),"pile inspection exposes only sorted multiset counts");
        bool bounds=true;for(int n=1;n<=10;++n){float end=0;for(int j=0;j<n;++j){auto r=Game::HandBox(n,j);bounds&=r.x>=20&&r.x+r.w<=1420&&r.y+r.h<=960&&r.w>0&&r.x>=end;end=r.x+r.w;}}
        Check(bounds,"all supported hand layouts stay bounded and nonoverlapping");
        Check(Game::HandBox(0,-3).x==Game::HandBox(1,0).x&&Game::HandBox(20,99).x==Game::HandBox(10,9).x,"layout clamps invalid counts and slots");
        g.NewRun();g.state.hand.assign(10,"punch");g.FocusCard(-1);Check(g.focused_card==9,"keyboard focus reaches tenth card from left");
        g.FocusCard(1);bool reach=g.focused_card==0;for(int j=1;j<10;++j){g.FocusCard(1);reach&=g.focused_card==j;}Check(reach,"keyboard navigation covers all ten slots and wraps");
        g.state.hand.clear();g.FocusCard(1);Check(g.focused_card==-1,"empty hand safely clears focus");
        g.pile_view=2;g.NewRun();Check(g.focused_card==-1&&g.pile_view==0,"reset clears information overlays");
        auto combined=config;combined.cards[0].damage=6;combined.cards[0].block=5;combined.cards[0].heal=4;
        Game a(combined,timeline);a.NewRun();a.state.hand={combined.cards[0].id};a.state.rage=99;a.state.player.hp=49;
        auto q=a.Preview(0);Combat copy=a.state;copy.Play(0);
        Check(q.rage==copy.rage-99&&q.heal==copy.player.hp-49&&q.block==copy.player.block,"combined-effect preview follows actual resolver order");
    }
    {
        Timeline multi;multi.steps={{0,Cue::Zoom,1.2f,1000},{500,Cue::Zoom,1.4f,1000},{750,Cue::Zoom,1.f,500},{2000,Cue::Impact},{2200,Cue::End}};
        Game a(config,multi),b(config,multi);a.NewRun();b.NewRun();a.state.rage=b.state.rage=100;a.Ultimate();b.Ultimate();
        Tick(a,.5f);Check(std::abs(a.visual.zoom-1.15f)<.0001f&&std::abs(a.visual.zoom_start-1.15f)<.0001f,"consecutive zoom captures current interpolated zoom");
        Tick(a,.25f);const float second=1.15f+.25f*.4375f;
        Check(std::abs(a.visual.zoom-second)<.0001f&&std::abs(a.visual.zoom_start-second)<.0001f,"third zoom starts continuously at interrupted cue boundary");
        Tick(a,.25f);Tick(b,1.f);Check(std::abs(a.visual.zoom-b.visual.zoom)<.0001f&&std::abs(b.visual.zoom-(second+(1-second)*.75f))<.0001f,"single update crossing several cues matches boundary sampling");
        Tick(a,.3f);Check(a.visual.zoom==1&&a.Cinematic(),"camera returns neutral before cinematic ends");
        a.SkipCinematic();Check(a.visual.zoom==1&&a.visual.zoom_start==1&&a.visual.zoom_target==1&&!a.visual.spear.visible&&a.state.enemy.hp==80,"skip restores neutral camera and impacts once");
        b.NewRun();Check(b.visual.zoom==1&&b.visual.zoom_time==0&&b.CanAct(),"reset cancels interrupted zoom and clears segment");
        Game g(config,timeline);g.NewRun();g.state.rage=100;g.Ultimate();Tick(g,3.2f);Check(g.visual.zoom>1&&g.visual.zoom<1.12f,"stock ultimate eases home before ending");
        Tick(g,.36f);Check(g.visual.zoom==1&&g.Cinematic(),"stock zoom reaches neutral ahead of end cue");
        Tick(g,.1f);Check(g.visual.zoom==1&&!g.Cinematic(),"normal cinematic cleanup stays neutral");
        g.NewRun();g.reduced_feedback=true;g.state.rage=100;g.Ultimate();Tick(g,.8f);Check(g.visual.zoom==1&&g.visual.zoom_target==1,"shared reduced feedback suppresses cinematic camera motion");
        g.SkipCinematic();Check(g.visual.zoom==1&&g.visual.flash_left==0&&g.visual.shake_left==0&&g.CanAct(),"reduced feedback skip retains neutral cleanup");
        bool same=true;for(int speed=1;speed<=3;++speed){Game x(config,multi),y(config,multi);x.NewRun();y.NewRun();x.SetSpeed(speed);y.SetSpeed(speed);x.state.rage=y.state.rage=100;x.Ultimate();y.Ultimate();Tick(x,1.1f);for(int n=0;n<110;++n)Tick(y,.01f);same&=std::abs(x.visual.zoom-y.visual.zoom)<.0001f;x.SkipCinematic();y.SkipCinematic();same&=x.state.enemy.hp==y.state.enemy.hp&&x.visual.zoom==1&&y.visual.zoom==1;}
        Check(same,"zoom cadence and cleanup equivalent at every speed");
    }
    std::cout<<"RESULT "<<checks<<" checks, "<<failures<<" failures\n";return failures?1:0;
}
