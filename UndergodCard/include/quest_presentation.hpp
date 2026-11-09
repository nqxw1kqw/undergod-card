#pragma once
#include "quest_core.hpp"
#include <map>
#include <array>

namespace undergod {
struct ActionTiming { float start=0,contact=.32f,end=.65f; };
struct CardBox { float x=0,y=0,w=176,h=150; };
enum class CardZone { Hand,Queue,Play,Discard,Exhaust };
struct CardVisual {
    unsigned id=0;std::string card;int hand_index=-1,queue_slot=-1;
    CardDef snapshot;
    CardBox from,to;float age=0,duration=.2f,delay=0;
    CardZone zone=CardZone::Hand;bool leaving=false,enemy=false;
    CardBox Bounds()const{
        float t=std::clamp((age-delay)/std::max(.001f,duration),0.f,1.f);t=t*t*(3-2*t);
        return {from.x+(to.x-from.x)*t,from.y+(to.y-from.y)*t,from.w+(to.w-from.w)*t,from.h+(to.h-from.h)*t};
    }
};
struct CardPreview { int cost=0,damage=0,absorbed=0,block=0,heal=0,rage=0,self_damage=0;std::string unavailable; };
struct ForecastStep {
    std::string name,effects;
    int actor=-1,hp=0,block=0,damage_taken=0,damage_dealt=0,healed=0,shield=0,absorbed=0;
    bool wait=false,assumed_wait=false,cancelled=false,resolved=false;
};
struct RoundForecast { std::array<ForecastStep,6> steps;int damage_taken=0,damage_dealt=0;bool valid=false,incomplete=false; };
ForecastStep DescribeForecastAction(const Combat& combat,int at);
void FinishForecastAction(ForecastStep& step,const Combat& combat);
struct ForecastMetrics { unsigned probes=0,round_builds=0,card_builds=0;double build_us=0; };
struct ForecastCache {
    std::string key,probe;
    RoundForecast round;
    std::array<CardPreview,10> cards;
    std::array<bool,10> card_valid{};
    bool round_valid=false;
    ForecastMetrics metrics;
};
struct MatchSummary { int turns=0,hp=0,cards=0,damage=0,blocked=0,healed=0,ultimates=0;unsigned seed=0; };
struct TerminalVisual { bool active=false;float age=0;unsigned entries=0;bool settled=false; };
struct ContactParticle { int unit=-1;EventType type=EventType::Damage;float x=0,y=0,age=0,life=.4f,vx=0,vy=0; };
enum class PracticeDifficulty { Gentle,Standard,Challenge };
inline const char* PracticeDifficultyName(PracticeDifficulty d){return d==PracticeDifficulty::Gentle?"Nhẹ":(d==PracticeDifficulty::Challenge?"Khó":"Chuẩn");}
}
