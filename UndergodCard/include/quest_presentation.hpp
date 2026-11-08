#pragma once
#include "quest_core.hpp"
#include <map>

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
struct MatchSummary { int turns=0,hp=0,cards=0,damage=0,blocked=0,healed=0,ultimates=0;unsigned seed=0; };
struct TerminalVisual { bool active=false;float age=0;unsigned entries=0;bool settled=false; };
struct ContactParticle { int unit=-1;EventType type=EventType::Damage;float x=0,y=0,age=0,life=.4f,vx=0,vy=0; };
enum class PracticeDifficulty { Gentle,Standard,Challenge };
inline const char* PracticeDifficultyName(PracticeDifficulty d){return d==PracticeDifficulty::Gentle?"Nhẹ":(d==PracticeDifficulty::Challenge?"Khó":"Chuẩn");}
}
