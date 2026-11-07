#include "quest_core.hpp"
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <set>
#include <tuple>
using namespace undergod;
int checks=0,failures=0;
void Check(bool v){++checks;if(!v)++failures;}
struct Result {bool win;int turns,hp,ults,unspent;bool operator==(const Result& b)const{return std::tie(win,turns,hp,ults,unspent)==std::tie(b.win,b.turns,b.hp,b.ults,b.unspent);}};
std::multiset<std::string> Deck(const Combat& c){std::multiset<std::string> d(c.hand.begin(),c.hand.end());d.insert(c.draw.begin(),c.draw.end());d.insert(c.discard.begin(),c.discard.end());return d;}
Result Run(Config cfg,unsigned seed,int policy){
    Combat c(cfg);c.Reset(seed);const auto deck=Deck(c);int ults=0,unspent=0;
    while(c.phase==Phase::Player&&c.turn<100){
        for(int plays=0;plays<20&&c.phase==Phase::Player;++plays){
            if(c.StartUltimate()){c.FinishUltimate();++ults;continue;}
            int best=-1;double score=0;
            for(int i=0;i<int(c.hand.size());++i){
                if(!c.CanPlay(i))continue;
                const auto& d=c.Definition(c.hand[i]);double value=d.damage;
                if(d.damage>=c.enemy.hp+c.enemy.block)value+=1000;
                const int incoming=c.intent.attack?std::max(0,c.intent.value-c.player.block):0;
                if(policy>=1)value+=std::min(d.block,incoming)*1.3;
                if(policy>=2)value+=std::min(d.heal,c.player.max_hp-c.player.hp)*(c.player.hp<25?2.:.8);
                value/=std::max(1,d.cost);if(value>score){score=value;best=i;}
            }
            if(best<0)break;
            c.Play(best);Check(Deck(c)==deck);
        }
        if(c.phase!=Phase::Player)break;
        unspent+=c.energy;c.EndPlayer();Check(Deck(c)==deck);c.ActEnemy();Check(Deck(c)==deck);
    }
    Check(c.phase==Phase::Won||c.phase==Phase::Lost);
    return {c.phase==Phase::Won,c.turn,c.player.hp,ults,unspent};
}
int main(int argc,char** argv){
    std::filesystem::current_path(std::filesystem::absolute(argv[0]).parent_path());auto cfg=LoadConfig();
    if(argc>1)for(auto& c:cfg.cards)if(c.id=="heavy")c.damage=std::stoi(argv[1]);
    std::cout<<std::fixed<<std::setprecision(2);
    for(int p=0;p<3;++p){double wins=0,turns=0,hp=0,ults=0,energy=0;
        for(unsigned seed=0;seed<1000;++seed){auto r=Run(cfg,seed,p);Check(r==Run(cfg,seed,p));wins+=r.win;turns+=r.turns;hp+=r.hp;ults+=r.ults;energy+=r.unspent;}
        std::cout<<("policy="+std::to_string(p))<<" win%="<<wins/10<<" turns="<<turns/1000<<" hp="<<hp/1000<<" ultimates="<<ults/1000<<" unspent="<<energy/1000<<"\n";
    }
    std::cout<<"RESULT "<<checks<<" checks, "<<failures<<" failures\n";return failures?1:0;
}
