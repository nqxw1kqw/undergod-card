#pragma once
#include "quest_core.hpp"
#include "quest_timeline.hpp"
#include <array>
#include <utility>

namespace undergod {
enum class Motion { None,PlayerAttack,PlayerOther,EnemyAttack,EnemyOther,TurnTransition };
struct ActionTiming { float start,contact,end; };
struct Pose { std::string name="idle"; float age=0; };
struct Popup { bool enemy; int amount; EventType type; float age=0; bool blocked=false; int lane=0; };
struct Particle { bool enemy; EventType type; float age,life,vx,vy; };
struct SpearVisual { bool visible=false,falling=false; float age=0,duration=1,from_x=0,from_y=0,to_x=0,to_y=0; };
struct CinematicVisual { float dim=0,zoom=1,zoom_target=1,zoom_duration=0,zoom_age=0,vfx_age=-1,shake=0,shake_left=0,flash_left=0,flash_duration=1; int vfx_frames=4; bool vfx_enemy=true; SpearVisual spear; };
struct MatchSummary { int turns=0,hp=0,cards=0,damage=0,blocked=0,healed=0,ultimates=0; unsigned seed=0; };
struct TerminalVisual { bool active=false,enemy=false; float age=0; unsigned entries=0; };
struct CardBox {float x=0,y=0,w=176,h=195;};
struct CardVisual {
    unsigned id=0;std::string card;int slot=-1;
    CardBox from,to;float age=0,duration=.2f,delay=0;bool leaving=false,selected=false,via_play=false;
    CardBox Bounds()const{
        if(via_play){
            CardVisual segment=*this;segment.via_play=false;
            if(age<.20f){segment.to={632,485,176,195};segment.duration=.20f;}
            else{segment.from={632,485,176,195};segment.age=age-.20f;segment.duration=.22f;}
            return segment.Bounds();
        }
        float t=std::clamp((age-delay)/std::max(.001f,duration),0.f,1.f);t=t*t*(3-2*t);
        return {from.x+(to.x-from.x)*t,from.y+(to.y-from.y)*t,from.w+(to.w-from.w)*t,from.h+(to.h-from.h)*t};
    }
};
class Game {
    Motion motion_=Motion::None;
    int pending_card_=-1;
    Intent resolving_intent_;
    unsigned next_visual_id_=1;
    void EnsureCards();
    void DrawCards(float age=0);
    void ResolveCardVisual();
    float motion_age_=0,cinematic_age_=0;
    bool motion_impact_=false;
    int speed_=1;
    unsigned seed_=1337,session_seed_=1337;
    Rng visual_rng_;
    void AgeEffects(float dt);
    void BeginTerminal(float late);
    Timeline timeline_;
    TimelinePlayer cinematic_;
    void ConsumeEvents(float late=0);
    void ApplyMotion();
    void CueStep(const Step& step);
public:
    static constexpr float PlayerHome=320,EnemyHome=1120;
    static constexpr float BasePlaybackRate=.65f;
    static CardBox HandBox(int count,int index){
        const int w=std::min(176,(1300-12*(count-1))/std::max(1,count)),total=count*w+(count-1)*12;
        return {float((1440-total)/2+index*(w+12)),710,float(w),195};
    }
    std::vector<CardVisual> cards;
    Combat state;
    MatchSummary summary;
    TerminalVisual terminal;
    std::array<Pose,2> poses;
    std::vector<Popup> popups;
    std::vector<Particle> particles;
    bool reduced_feedback=false;
    CinematicVisual visual;
    float time=0,toast_age=0;
    std::string toast;
    explicit Game(Config config,Timeline timeline):timeline_(std::move(timeline)),state(std::move(config)){}
    void NewRun(unsigned seed=1337);
    unsigned Seed()const{return seed_;}
    void SetSessionSeed(unsigned seed){session_seed_=seed;}
    void NewMatch(){session_seed_+=0x9e3779b9u;NewRun(session_seed_);}
    void Retry(){NewRun(seed_);}
    bool SelectDifficulty(Difficulty d){
        if(Busy()||(d!=Difficulty::Gentle&&d!=Difficulty::Standard&&d!=Difficulty::Challenge))return false;
        state.difficulty=d;NewRun(seed_);return true;
    }
    void CycleDifficulty(){SelectDifficulty(state.difficulty==Difficulty::Standard?Difficulty::Challenge:(state.difficulty==Difficulty::Challenge?Difficulty::Gentle:Difficulty::Standard));}
    bool CanAct()const{return motion_==Motion::None&&!cinematic_.Active()&&state.phase==Phase::Player;}
    bool Busy()const{return motion_!=Motion::None||cinematic_.Active()||(terminal.active&&terminal.age<1.5f);}
    Intent DisplayIntent()const{return EnemyResolving()?resolving_intent_:state.intent;}
    bool EnemyResolving()const{return motion_==Motion::EnemyAttack||motion_==Motion::EnemyOther;}
    bool Transition()const{return motion_==Motion::TurnTransition;}
    bool ShowHand()const{return !EnemyResolving();}
    int IncomingLoss()const{return EnemyResolving()?0:(state.intent.attack?std::max(0,state.intent.value-state.player.block):0);}
    const char* TurnStatus()const{return Transition()?"Your turn":(EnemyResolving()?(motion_impact_?"Enemy recovery":(resolving_intent_.prepare?"Enemy preparing":(resolving_intent_.charged?"Charged attack":(resolving_intent_.attack?"Enemy attack":"Enemy guard")))):(Busy()?"Resolving card":"Your turn"));}
    bool Cinematic()const{return cinematic_.Active();}
    float ResultAlpha()const{return terminal.active?std::clamp((terminal.age-.8f)/.7f,0.f,1.f):0;}
    bool Result()const{return terminal.active&&ResultAlpha()>0&&!Cinematic();}
    void Play(int index);
    void EndTurn();
    void Ultimate();
    void SkipCinematic();
    void Update(float dt);
    int Speed()const{return speed_;}
    float PlaybackRate()const{return BasePlaybackRate*speed_;}
    bool SetSpeed(int multiplier){if(multiplier<1||multiplier>3)return false;speed_=multiplier;return true;}
    std::pair<float,float> SpearTip()const;
    float ActorOffset(bool enemy)const;
    ActionTiming Timing()const;
    int PoseFrame(bool enemy)const;
    void Toast(const std::string& text){toast=text;toast_age=1.8f;}
};
}
