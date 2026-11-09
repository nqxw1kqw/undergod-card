#include "game.hpp"
#include "quest_json.hpp"
#include <chrono>
#include <iostream>
#include <numeric>
using namespace undergod;
extern void Check(bool,const char*);
namespace {
void Advance(Game& g,float logical){g.Update(logical/g.PlaybackRate());}
void Settle(Game& g){for(int i=0;i<6000&&g.Busy();++i)Advance(g,.01f);}
Intent Attack(int amount){Intent i;i.kind=IntentKind::Attack;i.attack=true;i.value=amount;return i;}
void Fixture(Game& g){
    g.StartBossPractice("tiger",1337);Settle(g);g.state.player.hp=480;g.state.player.max_hp=500;g.state.enemy.hp=g.state.enemy.max_hp=500;
    g.state.hand={"shield","punch","cookie"};g.state.live.locked=-1;g.state.stars.enemy={{0,Attack(8),false},{0,Attack(6),false},{0,{},true}};Advance(g,.3f);
}
std::string Snapshot(const Game& g){return nlohmann::json(g.state.Snapshot()).dump();}
int OldIncoming(const Game& g){
    Combat copy=g.state;copy.events.clear();while(copy.stars.player.size()<3)copy.QueueWait();if(!copy.BeginStarAttack())return 0;
    for(int n=0;n<6&&copy.phase==Phase::Player;++n)copy.ResolveStarAction();
    int loss=0;for(const auto& e:copy.events)if(e.type==EventType::Damage&&!e.enemy)loss+=e.amount;return loss;
}
double PerCall(std::vector<double> v,int percentile){std::sort(v.begin(),v.end());return v[std::min(v.size()-1,v.size()*percentile/100)];}
}
void ForecastChecks(const Config& config,const Timeline& timeline){
    {
        Game g(config,timeline);Fixture(g);const auto before=Snapshot(g);const auto builds=g.ForecastStats().round_builds;
        for(int i=0;i<600;++i){g.IncomingLoss();g.Preview(0);Advance(g,.016f);}
        Check(g.ForecastStats().round_builds==builds+1&&g.ForecastStats().card_builds==1,"unchanged frames reuse round and card calculations");
        Check(Snapshot(g)==before,"cache keys and repeated forecasts preserve every snapshot field and RNG");
        const auto warm=g.ForecastStats().round_builds;g.ToggleFeedback();g.SetSpeed(3);g.IncomingLoss();Check(g.ForecastStats().round_builds==warm,"cosmetic FX speed and visual time do not invalidate math");
        g.state.enemy.block=5;auto p=g.Preview(1);Check(p.absorbed==5&&p.damage==1,"direct armor changes invalidate cached card effects");
        g.state.player.hp=499;Check(g.Preview(2).heal==1,"direct HP changes invalidate effective healing");
        g.state.stars.enemy[0].action.value=17;Check(g.IncomingLoss()==OldIncoming(g),"changed published boss plan invalidates round cache");
        const auto last=g.ForecastStats().round_builds;g.state.live.thorns=4;g.IncomingLoss();Check(g.ForecastStats().round_builds==last+1,"battle runtime changes are included in the exact cache key");
        g.state.hand[1]="heavy";Check(g.Preview(1).damage==7,"changed hand identity invalidates cached card");
        const auto key_before=[&](){std::string s;g.state.AppendForecastKey(s);return s;}();g.state.Draw(1);std::string key_after;g.state.AppendForecastKey(key_after);Check(key_before!=key_after,"draw piles and shuffle stream participate in cache identity");
    }
    {
        Game g(config,timeline);Fixture(g);g.Play(0);g.Play(1);g.Play(2);const auto before=Snapshot(g);const auto forecast=g.Forecast();
        Check(forecast.valid&&!forecast.incomplete&&forecast.damage_taken==9,"selected shield order forecasts actual absorbed HP loss");
        Check(forecast.steps[1].hp==477&&forecast.steps[1].absorbed==5&&forecast.steps[4].hp==479,"six-step projection tracks shield expiry damage and effective heal");
        Check(Snapshot(g)==before,"six-step projection does not commit cards damage or RNG");
        g.ToggleForecast();g.EndTurn();g.DrinkPotion(1);Check(!g.ResolvingStars()&&Snapshot(g)==before,"forecast modal prevents gameplay click-through");g.ToggleForecast();g.EndTurn();
        std::array<bool,6> checked{};int comparisons=0;
        for(int i=0;i<6000&&g.ResolvingStars();++i){Advance(g,.01f);const int at=g.ActiveStep();
            if(g.Recovering()&&!checked[at]){checked[at]=true;++comparisons;Check(g.state.player.hp==forecast.steps[at].hp&&g.state.player.block==forecast.steps[at].block,"each live contact matches its forecast HP and armor");Check(g.Forecast().steps[at].resolved,"live trace replaces completed projection with actual effects");}
            if(at==0&&g.Recovering())Check(g.ActiveStep()==0&&g.state.stars.cursor==1,"active chip remains on the recovering action after core cursor advances");
        }
        Check(comparisons==6,"all six alternating actions were inspected at contact");
    }
    {
        Game g(config,timeline);Fixture(g);g.state.enemy.hp=1;g.state.hand={"punch"};g.Play(0);g.QueueWait();g.QueueWait();const auto f=g.Forecast();
        Check(f.steps[0].damage_dealt==1&&f.steps[1].cancelled&&f.steps[5].cancelled,"lethal projection cancels all remaining actions");
        g.NewRun();Check(!g.Forecast().valid&&!g.forecast_view&&g.ActiveStep()==-1,"new encounter clears the prior round trace");
    }
    for(int speed=1;speed<=3;++speed){
        Game g(config,timeline);Fixture(g);g.SetSpeed(speed);g.state.hand={"heavy"};g.Play(0);g.QueueWait();g.QueueWait();g.EndTurn();
        Advance(g,.43f);Check(g.state.enemy.hp==500&&std::find(g.sound_events.begin(),g.sound_events.end(),"hit")==g.sound_events.end(),"heavy windup has neither early damage nor hit sound at any speed");
        Advance(g,.02f);Check(g.state.enemy.hp==488&&std::count(g.sound_events.begin(),g.sound_events.end(),"hit")==1,"heavy contact and its one sound stay synchronized at x1 x2 x3");
        Settle(g);
    }
    std::cout<<"FORECAST exact-cache six-actions timing passed\n";
}
void ForecastBenchmark(const Config& config,const Timeline& timeline,int iterations){
    Game g(config,timeline);Fixture(g);g.Play(0);g.Play(1);g.Play(2);const auto before=Snapshot(g);volatile int checksum=0;std::vector<double> old_times,new_times;
    auto sample=[&](bool cached){const auto start=std::chrono::steady_clock::now();for(int i=0;i<iterations;++i)checksum+=cached?g.IncomingLoss():OldIncoming(g);return std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count()/iterations;};
    const int expected=OldIncoming(g);Check(g.IncomingLoss()==expected,"benchmark compares the same HP result");const auto builds=g.ForecastStats().round_builds;
    for(int block=0;block<15;++block){if(block%2){new_times.push_back(sample(true));old_times.push_back(sample(false));}else{old_times.push_back(sample(false));new_times.push_back(sample(true));}}
    Check(g.ForecastStats().round_builds==builds&&Snapshot(g)==before,"benchmark reuses cached simulation without changing combat");
    std::cout<<"FORECAST_BENCH "<<nlohmann::json{{"iterations_per_block",iterations},{"blocks",15},{"baseline_median_us",PerCall(old_times,50)},{"baseline_p95_block_us",PerCall(old_times,95)},{"cached_median_us",PerCall(new_times,50)},{"cached_p95_block_us",PerCall(new_times,95)},{"round_builds",g.ForecastStats().round_builds},{"checksum",int(checksum)},{"scope","CPU incoming forecast only; not total FPS or GPU"}}.dump()<<"\n";
}
