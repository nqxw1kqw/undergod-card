#pragma once
#include "quest_core.hpp"
#include "quest_timeline.hpp"
#include <array>
#include <utility>

namespace undergod {
enum class Motion { None,PlayerAttack,PlayerOther,EnemyAttack,EnemyOther };
struct Pose { std::string name="idle"; float age=0; };
struct Popup { bool enemy; int amount; EventType type; float age=0; bool blocked=false; };
struct SpearVisual { bool visible=false,falling=false; float age=0,duration=1,from_x=0,from_y=0,to_x=0,to_y=0; };
struct CinematicVisual { float dim=0,zoom=1,zoom_target=1,zoom_duration=0,zoom_age=0,vfx_age=-1,shake=0,shake_left=0,flash_left=0,flash_duration=1; int vfx_frames=4; bool vfx_enemy=true; SpearVisual spear; };
class Game {
    Motion motion_=Motion::None;
    int pending_card_=-1;
    float motion_age_=0,cinematic_age_=0;
    bool motion_impact_=false;
    int speed_=1;
    unsigned seed_=1337,session_seed_=1337;
    Timeline timeline_;
    TimelinePlayer cinematic_;
    void ConsumeEvents();
    void ApplyMotion();
    void CueStep(const Step& step);
public:
    static constexpr float PlayerHome=320,EnemyHome=1120;
    static constexpr float BasePlaybackRate=.65f;
    Combat state;
    std::array<Pose,2> poses;
    std::vector<Popup> popups;
    CinematicVisual visual;
    float time=0,toast_age=0;
    std::string toast;
    explicit Game(Config config,Timeline timeline):timeline_(std::move(timeline)),state(std::move(config)){}
    void NewRun(unsigned seed=1337);
    unsigned Seed()const{return seed_;}
    void SetSessionSeed(unsigned seed){session_seed_=seed;}
    void NewMatch(){session_seed_+=0x9e3779b9u;NewRun(session_seed_);}
    void Retry(){NewRun(seed_);}
    bool CanAct()const{return motion_==Motion::None&&!cinematic_.Active()&&state.phase==Phase::Player;}
    bool Busy()const{return motion_!=Motion::None||cinematic_.Active();}
    bool Cinematic()const{return cinematic_.Active();}
    bool Result()const{return !Busy()&&(state.phase==Phase::Won||state.phase==Phase::Lost);}
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
    void Toast(const std::string& text){toast=text;toast_age=1.8f;}
};
}
