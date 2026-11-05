#include "game.hpp"
#include "quest_json.hpp"
#include <fstream>
#include <iostream>
#include <set>
using namespace undergod;
extern void Check(bool,const char*);
namespace {
void Advance(Game& game,float seconds){game.Update(seconds/game.PlaybackRate());}
void SettleStars(Game& game){for(int n=0;n<800&&game.Busy();++n)game.Update(.05f);Check(!game.Busy(),"Turn Star controller finishes without stalling");}
Combat Duel(const Config& config){Combat c(config);c.BeginEncounter(FindEnemy(config,"tiger"),1337,500,500,0,StartingDeck(config));c.EnableTurnStars();return c;}
Intent Attack(int amount){Intent i;i.attack=true;i.kind=IntentKind::Attack;i.value=amount;return i;}
Intent Guard(int amount){Intent i;i.kind=IntentKind::Defend;i.value=amount;return i;}
void Finish(Combat& c){for(int n=0;n<6&&c.stars.resolving&&c.phase==Phase::Player;++n)c.ResolveStarAction();if(c.phase==Phase::Player)c.FinishStarRound();}
std::string Read(const std::filesystem::path& p){std::ifstream in(p);return {std::istreambuf_iterator<char>(in),{}};}
void Write(const std::filesystem::path& p,const nlohmann::json& j){std::ofstream out(p);out<<j.dump(2);}
}
void StarChecks(const Config& config,const Timeline& timeline){
    {
        auto c=Duel(config);c.hand={"heavy","shield","snare","mud"};c.live.locked=-1;c.energy=0;
        const int hp=c.enemy.hp;const auto enemy_plan=nlohmann::json(c.stars.enemy).dump();const auto rng=c.Snapshot().intent_rng;
        Check(c.QueueStar(0)&&c.QueueStar(1)&&c.QueueStar(2),"three cards reserve three stars regardless of legacy cost");
        Check(c.enemy.hp==hp&&c.player.block==0&&c.hand.size()==4,"planning has no damage block or card consumption");
        Check(!c.QueueStar(0)&&!c.QueueStar(3)&&!c.Play(0),"full queue curses duplicates and direct play cannot bypass planning");
        Check(c.UnqueueStar(1)&&c.stars.player[1].id=="snare","unqueue keeps remaining order");
        Check(!c.QueueStar(3)&&c.QueueStar(1),"curse rejected and removed card can be queued again");
        Check(nlohmann::json(c.stars.enemy).dump()==enemy_plan&&c.Snapshot().intent_rng==rng,"editing the player queue never rerolls enemy cards");
        c.UnqueueStar(1);c.UnqueueStar(1);c.QueueStar(1);c.QueueStar(2);
        auto attack=Attack(4),guard=Guard(7);Intent debuff;debuff.kind=IntentKind::Debuff;debuff.status=StatusId::Vulnerable;debuff.status_amount=1;
        c.stars.enemy={{0,attack,false},{0,guard,false},{0,debuff,false}};
        Check(c.BeginStarAttack()&&!c.QueueStar(0)&&!c.UnqueueStar(0),"Attack commits and locks both queues");
        Check(c.ResolveStarAction()&&c.enemy.hp==hp-12&&c.player.hp==500,"first player card resolves first");
        Check(c.ResolveStarAction()&&c.player.hp==496,"enemy card one responds before player card two");
        Check(c.ResolveStarAction()&&c.player.block==5,"second player card grants armor at its own action");
        Check(c.ResolveStarAction()&&c.enemy.block==7,"second enemy card grants armor before third player card");
        Check(c.ResolveStarAction()&&c.enemy.hp==hp-12&&c.enemy.block==1&&c.enemy.status.weak==2,"third player card observes previous enemy armor");
        Check(c.ResolveStarAction()&&c.player.status.vulnerable==1,"third enemy card applies its debuff last");
        Check(!c.ResolveStarAction(),"a seventh action cannot replay the last card");
        c.FinishStarRound();Check(c.turn==2&&!c.stars.resolving&&c.stars.player.empty()&&c.stars.enemy.size()==3,"new round rebuilds exactly three enemy cards");
        Check(c.player.status.vulnerable==1&&c.enemy.status.weak==2,"newly applied debuffs do not decay prematurely");
    }
    {
        Combat c(config);c.BeginEncounter(FindEnemy(config,"undergod"),4,500,500,0,StartingDeck(config));c.EnableTurnStars();c.enemy.status.strength=2;c.player.status.vulnerable=1;
        Intent mimic;mimic.kind=IntentKind::Mimic;mimic.attack=true;mimic.card_id="punch";c.stars.enemy[0]={0,mimic,false};
        const int expected=ScaleDamage(6,c.enemy.status,c.player.status);Check(c.EnemyStarView(0).damage==expected,"mimic tooltip includes actual strength weak and vulnerable scaling");
        mimic.card_id="mana_focus";c.stars.enemy[0].action=mimic;Check(c.EnemyStarView(0).description.find("Sức Mạnh")!=std::string::npos,"mimic tooltip explains enemy conversion of draw and energy effects");
    }
    {
        auto c=Duel(config);c.hand={"punch"};c.draw.clear();c.discard.clear();
        Check(!c.CanAttackStars()&&c.QueueStar(0)&&!c.CanAttackStars(),"Attack needs three occupied star slots");
        Check(c.QueueWait()&&c.QueueWait()&&c.CanAttackStars(),"wait actions prevent a thin deck softlock");
        c.stars.enemy={{0,Guard(0),false},{0,Guard(0),false},{0,Guard(0),false}};
        c.BeginStarAttack();Finish(c);Check(c.turn==2&&c.hand.size()==1,"wait does not create or duplicate cards");
        c.live.locked=0;Check(!c.QueueStar(0),"frog locked card cannot be reserved");
    }
    {
        auto c=Duel(config);c.hand={"crow+","shield","punch"};c.live.locked=-1;c.player.block=0;
        c.QueueStar(0);c.QueueStar(1);c.QueueStar(2);c.BeginStarAttack();c.ResolveStarAction();
        Check(c.player.block==3&&c.player.status.strength==2,"cost-only upgrade becomes three armor under one-star pricing");
        Check(c.StarCardView(c.Definition("crow+")).description.find("3 giáp")!=std::string::npos,"cost-only upgrade tooltip matches its new effect");
    }
    {
        auto c=Duel(config);c.hand={"mana_focus","punch","shield"};c.draw={"cookie","heavy"};c.discard.clear();
        c.QueueStar(0);c.QueueStar(1);c.QueueStar(2);c.stars.enemy={{0,Guard(0),false},{0,Guard(0),false},{0,Guard(0),false}};
        c.BeginStarAttack();c.ResolveStarAction();Check(c.hand.size()==3&&c.hand.back()=="heavy","energy card becomes draw in Turn Star mode");
        Finish(c);Check(std::find(c.hand.begin(),c.hand.end(),"heavy")!=c.hand.end(),"cards drawn during execution carry into next planning hand");
        Check(c.StarCardView(c.Definition("mana_focus")).description.find("Rút")!=std::string::npos&&c.StarCardView(c.Definition("heavy")).cost==1,"card display matches one star economy");
    }
    {
        auto c=Duel(config);c.hand={"punch","shield","punch"};c.enemy.hp=101;c.enemy.status.poison=2;
        c.QueueStar(0);c.QueueStar(1);c.QueueStar(2);const auto frozen=nlohmann::json(c.stars.enemy).dump();
        c.BeginStarAttack();c.ResolveStarAction();Check(c.PhaseIndex()==1&&nlohmann::json(c.stars.enemy).dump()==frozen,"phase transition does not secretly replace committed enemy cards");
        Finish(c);Check(c.live.charge_left==2&&c.stars.enemy[0].action.kind==IntentKind::Charge,"new phase charge starts in the next round");
        const int hp=c.player.hp;for(int i=0;i<3;++i)c.QueueWait();c.BeginStarAttack();Finish(c);
        Check(c.player.hp==hp&&c.live.charge_left==1,"charge counts once per round and waits before damage");
        for(int i=0;i<3;++i)c.QueueWait();c.BeginStarAttack();Finish(c);Check(c.player.hp<hp&&c.live.charge_left==2,"charge hits next round and restarts once");
    }
    {
        Combat c(config);c.BeginEncounter(FindEnemy(config,"frog"),4,500,500,0,StartingDeck(config));c.EnableTurnStars();c.hand={"punch","punch","punch"};c.live.locked=-1;c.enemy.hp=76;
        c.QueueStar(0);c.QueueStar(1);c.QueueStar(2);c.BeginStarAttack();c.ResolveStarAction();
        Check(c.PhaseIndex()==1&&c.live.locked==-1,"frog phase does not invalidate already committed player cards");Finish(c);
    }
    {
        Combat c(config);c.BeginEncounter(FindEnemy(config,"frog"),4,500,500,100,StartingDeck(config));c.EnableTurnStars();c.hand={"punch","shield","heavy"};c.live.locked=-1;c.enemy.hp=76;
        c.QueueStar(0);c.QueueStar(1);c.QueueStar(2);c.StartUltimate();c.UltimateImpact();c.FinishUltimate();
        Check(c.PhaseIndex()==1&&c.live.locked==-1&&c.CanAttackStars(),"ultimate phase transition never locks an already reserved card");
    }
    {
        Game game(config,timeline);game.NewRun(1337);game.ChooseNode(0);
        Check(game.Traveling()&&game.ShowingMap()&&!game.Map()&&game.run.node==-1,"map movement precedes actual node entry and locks input");
        Advance(game,.4f);Check(game.TravelProgress()>0&&game.TravelProgress()<1,"map avatar has interpolated travel progress");
        Advance(game,.70f);Check(game.ArrivalStage()==Arrival::FadeOut&&game.FadeOpacity()>0&&game.run.node==-1,"screen fades out before entering node");
        Advance(game,.24f);Check(game.run.node>=0&&game.ArrivalStage()==Arrival::BlackHold&&game.FadeOpacity()==1,"node changes behind a fully black held transition");
        SettleStars(game);Check(game.CanAct()&&game.state.stars.enabled&&game.DealtCards()==float(game.state.hand.size()),"battle unlocks only after fight banner and dealing");
        Game preview(config,timeline);preview.StartBossPractice("tiger",1337);Advance(preview,.46f);
        Check(preview.ArrivalStage()==Arrival::Fight&&preview.FightBannerX()>1440&&preview.DealtCards()==0,"Fight enters from the right before any hand is visible");
        Advance(preview,.39f);Check(std::abs(preview.FightBannerX()-720)<1,"Fight holds at the center");
        Advance(preview,.65f);Check(preview.FightBannerX()<720,"Fight exits toward the left");
        Advance(preview,.45f);Check(preview.ArrivalStage()==Arrival::Deal&&preview.DealtCards()>0&&preview.DealtCards()<5&&!preview.CanAct(),"opening hand is dealt progressively and remains locked");SettleStars(preview);
        preview.state.hand={"punch","shield","heavy"};preview.state.live.locked=-1;
        preview.Play(0);preview.Play(1);preview.Play(2);const int hp=preview.state.enemy.hp;
        Check(preview.CanAttack()&&preview.state.enemy.hp==hp&&!preview.Busy(),"selecting three cards prepares Attack without acting");
        preview.EndTurn();Advance(preview,.30f);Check(preview.Busy()&&preview.state.enemy.hp==hp,"queued attack has windup before contact");
        Advance(preview,.03f);Check(preview.state.enemy.hp<hp,"queued player attack resolves at contact");
        Check(!preview.Save(false)&&!preview.DrinkPotion(0),"saving and potions cannot interrupt execution");
        SettleStars(preview);Check(preview.state.turn==2&&preview.CanAct(),"six actions and next dealing return to planning");
    }
    {
        Game game(config,timeline);game.NewRun(1337);game.ChooseNode(0);SettleStars(game);game.run.sequence={"froglet"};
        game.state.enemy.hp=1;game.state.hand={"punch","shield","shield"};game.state.live.locked=-1;game.Play(0);game.Play(1);game.Play(2);game.EndTurn();SettleStars(game);
        Check(game.state.EnemyId()=="froglet"&&game.state.stars.enabled&&game.state.stars.player.empty()&&game.state.turn==1&&game.CanAct(),"chained encounter discards old resolution and starts a fresh announced plan");
        game.run.sequence={"froglet"};game.state.enemy.hp=1;game.state.rage=100;game.Ultimate();SettleStars(game);
        Check(!game.Cinematic()&&game.state.EnemyId()=="froglet"&&game.state.stars.enabled&&game.state.turn==1&&game.CanAct(),"chained ultimate stops its old cinematic before the next Fight banner");
    }
    {
        std::string expected;
        for(int speed=1;speed<=3;++speed){
            Game game(config,timeline);game.StartBossPractice("tiger",1337);SettleStars(game);game.SetSpeed(speed);
            game.state.hand={"punch","shield","heavy"};game.state.live.locked=-1;
            game.state.stars.enemy={{0,Guard(0),false},{0,Guard(0),false},{0,Guard(0),false}};
            game.Play(0);game.Play(1);game.Play(2);game.EndTurn();SettleStars(game);
            const auto snapshot=nlohmann::json(game.state.Snapshot()).dump();if(expected.empty())expected=snapshot;
            Check(snapshot==expected,"one two three speed share the same ordered combat outcome");
        }
    }
    {
        const std::filesystem::path file="artifacts/turn-star-save.json",bad="artifacts/turn-star-invalid.json",legacy="artifacts/turn-star-v2-legacy.json";
        std::filesystem::create_directories("artifacts");Game a(config,timeline),b(config,timeline);a.NewRun(1337);a.ChooseNode(0);SettleStars(a);
        a.SetSaveFile(file);a.Play(0);a.Play(1);const auto enemy_plan=nlohmann::json(a.state.stars.enemy).dump();
        Check(a.Save(false),"partial Turn Star plan saves with conserved original hand");b.SetSaveFile(file);Check(b.Load(),"Turn Star plan loads");
        Check(nlohmann::json(a.state.Snapshot()).dump()==nlohmann::json(b.state.Snapshot()).dump(),"save restore preserves queue targets order all RNG and enemy preview");
        const auto good=nlohmann::json::parse(Read(file));auto invalid=good;invalid["combat"]["stars"]["player"][1]=invalid["combat"]["stars"]["player"][0];Write(bad,invalid);b.SetSaveFile(bad);
        Check(!b.Load()&&nlohmann::json(b.state.stars.enemy).dump()==enemy_plan,"duplicate reservation rejected transactionally");
        auto old=good;old["combat"].erase("stars");Write(legacy,old);b.SetSaveFile(legacy);
        Check(b.Load()&&b.state.stars.enabled&&b.state.stars.enemy.size()==3&&b.state.hand==a.state.hand,"old session v2 migrates into Turn Star planning without losing cards");
    }
    int stalls=0,wins=0,losses=0;
    for(int seed=0;seed<90;++seed){
        const auto& character=config.classes[seed%3];Game game(config,timeline);game.StartBossPractice(seed%2?"tiger":"frog",seed,character.id);SettleStars(game);
        for(int n=0;n<600&&!game.Result();++n){const auto step=game.Suggest();if(step.kind==AutoKind::None)break;game.Act(step);SettleStars(game);}
        if(!game.Result())++stalls;else if(game.run.victory)++wins;else ++losses;
    }
    Check(stalls==0,"90 new-controller boss battles across three classes end without stalls");
    std::cout<<"TURN_STAR_BOSSES wins="<<wins<<" losses="<<losses<<" stalls="<<stalls<<"\n";
}
void SimulateStarRuns(const Config& config,const Timeline& timeline,int count){
    int wins=0,losses=0,stalls=0;std::array<int,3> class_wins{},act_losses{};
    for(int seed=0;seed<count;++seed){
        Game game(config,timeline);game.NewRun(seed,config.classes[seed%3].id);
        for(int n=0;n<6000&&!game.Result();++n){
            const auto action=game.Suggest();if(action.kind==AutoKind::None)break;game.Act(action);
            for(int frame=0;frame<1000&&game.Busy();++frame)game.Update(.1f);
            if(game.Busy())break;
        }
        if(!game.Result())++stalls;
        else if(game.run.victory){++wins;++class_wins[seed%3];}
        else{++losses;++act_losses[game.run.act];}
    }
    nlohmann::json report={{"rules","turn-stars"},{"runs",count},{"wins",wins},{"losses",losses},{"stalls",stalls},{"class_wins",class_wins},{"losses_by_act",act_losses}};
    std::filesystem::create_directories("artifacts");std::ofstream("artifacts/turn-star-simulation.json")<<report.dump(2)<<"\n";
    std::cout<<"TURN_STAR_CAMPAIGNS "<<report.dump()<<"\n";Check(stalls==0,"new Turn Star campaign bot reaches results without stalling");
}
