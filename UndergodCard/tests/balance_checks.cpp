#include "quest_core.hpp"
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <set>
#include <tuple>
#include <map>
using namespace undergod;
int checks=0,failures=0;
void Check(bool v){++checks;if(!v)++failures;}
struct Result {bool win;int turns,hp,ults,unspent; int opening_shields=0,exposures=0,unavoidable=0,charged_exposures=0,charged_unavoidable=0;std::string opening; uint64_t trace=0;bool operator==(const Result& b)const{return std::tie(win,turns,hp,ults,unspent,opening_shields,exposures,unavoidable,charged_exposures,charged_unavoidable,opening,trace)==std::tie(b.win,b.turns,b.hp,b.ults,b.unspent,b.opening_shields,b.exposures,b.unavoidable,b.charged_exposures,b.charged_unavoidable,b.opening,b.trace);}};
std::multiset<std::string> Deck(const Combat& c){std::multiset<std::string> d(c.hand.begin(),c.hand.end());d.insert(c.draw.begin(),c.draw.end());d.insert(c.discard.begin(),c.discard.end());return d;}
Result Run(Config cfg,unsigned seed,int policy,Difficulty difficulty=Difficulty::Standard){
    Combat c(cfg);c.difficulty=difficulty;c.Reset(seed);const auto deck=Deck(c);int ults=0,unspent=0;
    Result result{};auto opening=c.hand;std::sort(opening.begin(),opening.end());for(const auto& id:opening)result.opening+=id+",";for(const auto& id:c.hand)result.opening_shields+=c.Definition(id).block>0;

    while(c.phase==Phase::Player&&c.turn<100){
        result.trace=result.trace*1099511628211ull+unsigned(c.intent.value*8+c.intent.attack+2*c.intent.prepare+4*c.intent.charged);
        for(const auto& id:c.hand)for(unsigned char ch:id)result.trace=result.trace*1099511628211ull+ch;

        if(c.intent.attack){
            std::vector<int> blocks(c.energy+1,0);
            for(const auto& id:c.hand){const auto& d=c.Definition(id);for(int e=c.energy;e>=d.cost;--e)blocks[e]=std::max(blocks[e],blocks[e-d.cost]+d.block);}
            const int loss=std::max(0,c.intent.value-*std::max_element(blocks.begin(),blocks.end()));
            ++result.exposures;result.unavoidable+=loss;
            if(c.intent.charged){++result.charged_exposures;result.charged_unavoidable+=loss;}
        }

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
    result.win=c.phase==Phase::Won;result.turns=c.turn;result.hp=c.player.hp;result.ults=ults;result.unspent=unspent;return result;
}
int main(int argc,char** argv){
    std::filesystem::current_path(std::filesystem::absolute(argv[0]).parent_path());auto cfg=LoadConfig();
    if(argc>1)for(auto& c:cfg.cards)if(c.id=="heavy")c.damage=std::stoi(argv[1]);
    if(argc>2)cfg.enemy.challenge_charge=std::stoi(argv[2]);
    if(argc>3)cfg.enemy.stage_threshold_percent=std::stoi(argv[3]);
    if(argc>4)cfg.enemy.gentle_reduction=std::stoi(argv[4]);
    if(argc>5)cfg.enemy.gentle_charge=std::stoi(argv[5]);
    std::cout<<std::fixed<<std::setprecision(2);
    for(auto difficulty:{Difficulty::Gentle,Difficulty::Standard,Difficulty::Challenge})for(int p=0;p<3;++p){double wins=0,turns=0,hp=0,ults=0,energy=0;

        double exposures=0,unavoidable=0,charged_exposures=0,charged_unavoidable=0;int hand_count[4]={},hand_wins[4]={};std::map<std::string,std::pair<int,int>> openings;
        for(unsigned seed=0;seed<1000;++seed){auto r=Run(cfg,seed,p,difficulty);Check(r==Run(cfg,seed,p,difficulty));wins+=r.win;turns+=r.turns;hp+=r.hp;ults+=r.ults;energy+=r.unspent;exposures+=r.exposures;unavoidable+=r.unavoidable;charged_exposures+=r.charged_exposures;charged_unavoidable+=r.charged_unavoidable;++hand_count[r.opening_shields];hand_wins[r.opening_shields]+=r.win;++openings[r.opening].first;openings[r.opening].second+=r.win;}
        if(difficulty==Difficulty::Standard&&argc==1)Check(wins==(p==0?820:1000));
        std::cout<<DifficultyName(difficulty)<<" policy="<<p<<" win%="<<wins/10<<" turns="<<turns/1000<<" hp="<<hp/1000<<" ultimates="<<ults/1000<<" unspent="<<energy/1000<<" unavoidable/attack="<<unavoidable/std::max(1.,exposures)<<" charged/encounter="<<charged_exposures/1000<<" unavoidable/charged="<<charged_unavoidable/std::max(1.,charged_exposures)<<"\n";
        std::cout<<"  opening shields wins/count:";for(int i=0;i<4;++i)std::cout<<" "<<i<<"="<<hand_wins[i]<<"/"<<hand_count[i];std::cout<<"\n";
        double lo=101,hi=-1;std::string worst,best;int low_n=0,high_n=0;
        for(const auto& entry:openings){const double rate=100.*entry.second.second/entry.second.first;if(rate<lo){lo=rate;worst=entry.first;low_n=entry.second.first;}if(rate>hi){hi=rate;best=entry.first;high_n=entry.second.first;}}
        std::cout<<"  exact opening multisets="<<openings.size()<<" win range="<<lo<<".."<<hi<<" worst="<<worst<<" n="<<low_n<<" best="<<best<<" n="<<high_n<<"\n";
    }
    std::cout<<"RESULT "<<checks<<" checks, "<<failures<<" failures\n";return failures?1:0;
}
