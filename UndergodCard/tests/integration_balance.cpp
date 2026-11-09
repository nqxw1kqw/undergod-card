#include "game.hpp"
#include "quest_json.hpp"
#include <iostream>
#include <tuple>
#include <set>
using namespace undergod;
extern void Check(bool,const char*);
namespace {
struct Outcome {bool win=false,stall=false;int hp=0,turns=0,announced=0,unavoidable=0;uint64_t trace=1469598103934665603ull;std::string final;bool operator==(const Outcome& b)const{return std::tie(win,stall,hp,turns,announced,unavoidable,trace,final)==std::tie(b.win,b.stall,b.hp,b.turns,b.announced,b.unavoidable,b.trace,b.final);}};
std::multiset<std::string> Piles(const Combat& c){std::multiset<std::string> out;for(const auto* pile:{&c.hand,&c.draw,&c.discard,&c.exhaust})out.insert(pile->begin(),pile->end());return out;}
Outcome Run(const Config& config,const Timeline& timeline,const std::string& boss,const std::string& cls,unsigned seed,PracticeDifficulty level,int policy){
    Game game(config,timeline);game.StartBossPractice(boss,seed,cls);game.Update(20/game.PlaybackRate());
    if(level!=PracticeDifficulty::Standard){game.SelectPracticeDifficulty(level);game.Update(20/game.PlaybackRate());}
    Combat c=game.state;const auto initial=Piles(c);Outcome result;
    auto conserve=[&](){auto expected=initial;expected.insert(c.created.begin(),c.created.end());Check(Piles(c)==expected,"multi-policy actions conserve original and created card multisets");};
    auto trace=[&](){const auto s=c.Snapshot();for(unsigned v:{unsigned(c.turn),unsigned(c.player.hp),unsigned(c.enemy.hp),s.shuffle_rng,s.intent_rng,s.target_rng})result.trace=(result.trace^v)*1099511628211ull;for(const auto& id:c.hand)for(unsigned char ch:id)result.trace=(result.trace^ch)*1099511628211ull;};
    while(c.phase==Phase::Player&&c.turn<120){
        if(c.CanUltimate()){c.StartUltimate();c.UltimateImpact();c.FinishUltimate();conserve();trace();if(c.phase!=Phase::Player)break;}
        int incoming=0;for(int i=0;i<int(c.stars.enemy.size());++i)incoming+=c.EnemyStarView(i).damage;
        std::vector<int> blocks;for(int i=0;i<int(c.hand.size());++i)if(c.CanQueueStar(i))blocks.push_back(c.Definition(c.hand[i]).block);
        std::sort(blocks.rbegin(),blocks.rend());int available=c.player.block;for(int i=0;i<std::min(3,int(blocks.size()));++i)available+=blocks[i];
        result.announced+=incoming;result.unavoidable+=std::max(0,incoming-available);
        for(int n=0;n<3;++n){
            int best=-1;float score=-100000;
            for(int i=0;i<int(c.hand.size());++i)if(c.CanQueueStar(i)){
                const auto& d=c.Definition(c.hand[i]);float value=float(d.damage)+d.poison*2+d.strength*3+d.cards_drawn*2;
                if(c.live.airborne&&!d.ranged)value-=d.damage*.5f;
                if(d.damage>=c.enemy.hp+c.enemy.block)value+=300;
                if(policy>=1)value+=std::min(d.block,std::max(0,incoming-c.player.block))*1.3f;
                if(policy>=2)value+=std::min(d.heal,c.player.max_hp-c.player.hp)*1.8f;
                if(value>score){score=value;best=i;}
            }
            if(best<0)c.QueueWait();else c.QueueStar(best);
        }
        Check(c.BeginStarAttack(),"policy produces a valid three-star plan");
        for(int i=0;i<6&&c.phase==Phase::Player;++i){c.ResolveStarAction();conserve();trace();}
        if(c.phase==Phase::Player){c.FinishStarRound();conserve();trace();}
    }
    result.win=c.phase==Phase::Won;result.stall=c.phase!=Phase::Won&&c.phase!=Phase::Lost;result.hp=c.player.hp;result.turns=c.turn;result.final=nlohmann::json(c.Snapshot()).dump();return result;
}
}
void IntegratedBalanceChecks(const Config& config,const Timeline& timeline,int seeds){
    nlohmann::json rows=nlohmann::json::array();int total=0,stalls=0;
    for(int level=0;level<3;++level)for(int policy=0;policy<3;++policy){
        int wins=0,losses=0,turns=0,hp=0,announced=0,unavoidable=0;
        for(const auto& cls:config.classes)for(const auto& boss:config.bestiary)if(boss.role=="boss")for(int seed=0;seed<seeds;++seed){
            const auto a=Run(config,timeline,boss.id,cls.id,seed,PracticeDifficulty(level),policy);
            const auto b=Run(config,timeline,boss.id,cls.id,seed,PracticeDifficulty(level),policy);
            Check(a==b,"policy replay matches outcomes ordered trace and all RNG streams");Check(!a.stall,"integrated boss policy terminates");
            ++total;stalls+=a.stall;wins+=a.win;losses+=!a.win;turns+=a.turns;hp+=a.hp;announced+=a.announced;unavoidable+=a.unavoidable;
        }
        rows.push_back({{"profile",level},{"policy",policy},{"wins",wins},{"losses",losses},{"turns",turns},{"terminal_hp",hp},{"announced_damage",announced},{"shield_budget_gap",unavoidable}});
    }
    std::cout<<"INTEGRATED_BALANCE "<<nlohmann::json{{"seeds_per_boss_class",seeds},{"bosses",6},{"classes",3},{"policies",3},{"profiles",3},{"fights",total},{"replays",total},{"stalls",stalls},{"rows",rows}}.dump()<<"\n";
}
