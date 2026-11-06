#pragma once
#include "quest_run.hpp"
#include "quest_timeline.hpp"
#include "quest_save.hpp"
#include "quest_meta.hpp"
#include <array>
#include <utility>

namespace undergod {
enum class Motion { None,PlayerAttack,PlayerOther,EnemyAttack,EnemyOther };
enum class Arrival { None,Travel,FadeOut,BlackHold,FadeIn,Fight,Deal };
struct Pose { std::string name="idle"; float age=0; };
struct Popup { bool enemy; int amount; EventType type; float age=0; bool blocked=false; StatusId status=StatusId::Poison; int target=0; };
struct SpearVisual { bool visible=false,falling=false; float age=0,duration=1,from_x=0,from_y=0,to_x=0,to_y=0; };
struct CinematicVisual { float dim=0,zoom=1,zoom_target=1,zoom_duration=0,zoom_age=0,vfx_age=-1,shake=0,shake_left=0,flash_left=0,flash_duration=1; int vfx_frames=4; bool vfx_enemy=true; SpearVisual spear; };
class Game {
    bool turn_stars_=true;
    bool star_sequence_=false;
    bool active_charge_=false;
    CardDef active_card_;
    int active_actor_=-1;
    Arrival arrival_=Arrival::None;
    bool arrival_banner_=true;
    float arrival_age_=0;
    int travel_from_=-1,travel_to_=-1,pending_option_=-1;
    void BeginArrival(bool banner=true);
    void UpdateArrival(float dt);
    void BeginStarMotion();
    void FinishStarMotion();
    Motion motion_=Motion::None;
    int pending_card_=-1;
    float motion_age_=0,cinematic_age_=0;
    bool motion_impact_=false;
    int speed_=1;
    bool practice_=false;
    Profile practice_profile_;
    std::filesystem::path save_file_;
    bool save_dirty_=false;
    std::filesystem::path profile_file_;
    bool profile_dirty_=false;
    int menu_=0,selected_challenge_=0;
    Timeline timeline_;
    TimelinePlayer cinematic_;
    void ConsumeEvents();
    void ApplyMotion();
    void AfterResolve();
    void CueStep(const Step& step);
public:
    static constexpr float PlayerHome=320,EnemyHome=1120;
    static constexpr float BasePlaybackRate=.65f;
    Combat state;
    RunState run;
    Profile profile;
    std::vector<std::string> sound_events;
    std::array<Pose,2> poses;
    std::vector<Popup> popups;
    CinematicVisual visual;
    float time=0,toast_age=0;
    std::string toast;
    explicit Game(Config config,Timeline timeline,bool turn_stars=true):turn_stars_(turn_stars),timeline_(std::move(timeline)),state(std::move(config)){}
    void NewRun(unsigned seed=1337,std::string class_id="chick",int challenge=0);
    void StartBossPractice(std::string enemy_id,unsigned seed=1337,std::string class_id="chick");
    bool Practice()const{return practice_;}
    bool SetProfileFile(std::filesystem::path file);
    void OpenClasses(){if(!Busy()){menu_=1;selected_challenge_=std::min(selected_challenge_,std::clamp(profile.highest_challenge+1,0,10));}}
    void OpenSkills(){if(!Busy()&&!practice_)menu_=2;}
    void OpenAchievements(){if(!Busy())menu_=3;}
    void BackMenu(){menu_=menu_>1?1:0;}
    int MenuPage()const{return menu_;}
    bool Menu()const{return menu_>0;}
    const ClassDef& SelectedClass()const{return FindClass(state.config,profile.selected_class);}
    void SelectClass(int index);
    bool BuySkill(const std::string& id);
    void SetChallenge(int value){selected_challenge_=std::clamp(value,0,std::clamp(profile.highest_challenge+1,0,10));}
    int SelectedChallenge()const{return selected_challenge_;}
    void BeginSelected(unsigned seed){NewRun(seed,profile.selected_class,selected_challenge_);}
    void PracticeSelected(unsigned seed){StartBossPractice("tiger",seed,profile.selected_class);}
    void ToggleAudio(){profile.muted=!profile.muted;profile_dirty_=true;}
    void SetSaveFile(std::filesystem::path file){save_file_=std::move(file);save_dirty_=false;}
    bool Save(bool notify=true);
    bool Load();
    bool CanAct()const{return !Menu()&&!Busy()&&run.place==Place::Fight&&state.phase==Phase::Player&&!run.victory&&!run.defeat;}
    bool Busy()const{return motion_!=Motion::None||cinematic_.Active()||arrival_!=Arrival::None||star_sequence_;}
    bool TurnStarRules()const{return turn_stars_;}
    Arrival ArrivalStage()const{return arrival_;}
    float ArrivalTime()const{return arrival_age_;}
    bool ShowingMap()const{return !Menu()&&run.place==Place::Map&&!run.victory&&!run.defeat;}
    bool Traveling()const{return arrival_==Arrival::Travel;}
    bool MapTravelPending()const{return arrival_==Arrival::Travel||arrival_==Arrival::FadeOut;}
    int MapAvatarFrom()const{return MapTravelPending()?travel_from_:run.node;}
    int MapAvatarTo()const{return MapTravelPending()?travel_to_:run.node;}
    float MapAvatarProgress()const{return Traveling()?TravelProgress():1.f;}
    int TravelFrom()const{return travel_from_;}
    int TravelTo()const{return travel_to_;}
    float TravelProgress()const{return std::clamp(arrival_age_/.85f,0.f,1.f);}
    float FadeOpacity()const;
    float FightBannerX()const;
    float DealtCards()const;
    bool ResolvingStars()const{return star_sequence_;}
    const CardDef& ActiveCard()const{return active_card_;}
    int ActiveActor()const{return active_actor_;}
    int ActiveSlot()const{return std::max(0,(state.stars.cursor-(motion_impact_?1:0))/2);}
    bool CanAttack()const{return CanAct()&&(!turn_stars_||state.CanAttackStars());}
    bool QueueWait();
    bool UnqueueCard(int slot);
    bool Cinematic()const{return cinematic_.Active();}
    bool Map()const{return !Menu()&&!Busy()&&run.place==Place::Map&&!run.victory&&!run.defeat;}
    bool Reward()const{return !Busy()&&run.place==Place::Reward;}
    bool Event()const{return !Busy()&&run.place==Place::Event;}
    bool Shop()const{return !Busy()&&run.place==Place::Shop;}
    bool Rest()const{return !Busy()&&run.place==Place::Rest;}
    bool RelicReward()const{return !Menu()&&!Busy()&&run.place==Place::RelicReward;}
    bool CardSelection()const{return !Menu()&&!Busy()&&run.place==Place::CardSelect;}
    bool RelicSelection()const{return !Menu()&&!Busy()&&run.place==Place::RelicSelect;}
    bool Result()const{return !Busy()&&(run.victory||run.defeat);}
    void Play(int index);
    void EndTurn();
    void Ultimate();
    void ChooseNode(int option);
    void ChooseReward(int index);
    bool ChooseRelic(int index);
    bool ChooseCard(int index);
    bool TradeRelic(int index);
    void CancelSelection(){undergod::CancelSelection(run);save_dirty_=true;}
    bool SelectTarget(int index){if(!CanAct())return false;const bool changed=state.SelectTarget(index);save_dirty_|=changed;return changed;}
    void ChooseEvent(int index);
    bool Buy(int index);
    bool RemoveCard(int index);
    void LeaveShop();
    bool RestHeal();
    bool UpgradeCard(int index);
    bool DrinkPotion(int index);
    bool Act(const AutoStep& step);
    int SuggestCard()const;
    int SuggestReward()const{return undergod::SuggestReward(run,state.config);}
    AutoStep Suggest()const;
    void SkipCinematic();
    void Update(float dt);
    int Speed()const{return speed_;}
    float PlaybackRate()const{return BasePlaybackRate*speed_;}
    bool SetSpeed(int multiplier){if(multiplier<1||multiplier>3)return false;speed_=multiplier;save_dirty_=true;return true;}
    std::pair<float,float> SpearTip()const;
    float ActorOffset(bool enemy,int target=0)const;
    float TargetX(int target)const;
    float CinematicTime()const{return cinematic_age_;}
    void Toast(const std::string& text){toast=text;toast_age=1.8f;}
};
}
