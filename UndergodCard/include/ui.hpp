#pragma once
#include "game.hpp"
#include <unordered_map>
namespace undergod {
struct Rect { int x,y,w,h; bool Contains(int mx,int my)const{return mx>=x&&mx<x+w&&my>=y&&my<y+h;} };
class Ui {
    int font_=-1,small_=-1,heading_=-1,brand_=-1,stage_=-1;
    std::unordered_map<std::string,int> images_;
    std::unordered_map<std::string,Rect> image_bounds_;
    int map_scroll_=0,map_node_=-999,deck_page_=0,screen_node_=-999;
    Place screen_place_=Place::Fight;
    void Text(int x,int y,const std::string& text,unsigned color,int font=-1)const;
    void Center(int x,int y,const std::string& text,unsigned color,int font=-1)const;
    void Box(Rect r,unsigned fill,int radius=12)const;
    void Button(Rect r,const std::string& text,bool active,int mx,int my)const;
    void Wrapped(Rect r,const std::string& text,unsigned color,int font=-1)const;
    void World(const Game& game);
    void Actor(const Game& game,bool enemy,int target=0)const;
    void SkillActor(const Game& game,bool enemy,int target)const;
    void DrawWard(const Game& game,int unit)const;
    void DrawSkillEffects(const Game& game)const;
    void DrawCardVisuals(const Game& game,int mx,int my)const;
    void DrawPresentation(const Game& game,int mx,int my)const;
    void DrawFx(const std::string& effect,float x,float y,float size,float progress,float angle=0,int alpha=255)const;
    bool DrawExtensions(const Game& game,int mx,int my);
    void DrawTargets(const Game& game,int mx,int my)const;
    void Card(const CardDef& card,Rect r,int index,bool affordable,bool locked,int mx,int my)const;
    int Image(const std::string& key)const;
    bool star_cards_=false;
    void DrawStarBattle(const Game& game,int mx,int my);
    void DrawMapPlayer(const Game& game)const;
    void PlanCard(const CardDef& card,Rect r,int number,bool filled,bool spent,int mx,int my)const;
    std::string RelicDescription(const Game& game,const RelicDef& relic)const;
public:
    static constexpr int Width=1600,Height=900;
    static constexpr int ContentOffset=(Width-1440)/2;
    static float SceneX(float x){return x+ContentOffset;}
    static Rect ContentRect(Rect r){r.x+=ContentOffset;return r;}
    int loaded=0,expected=0,grayscale=0;
    bool Init(const Config& config);
    void Shutdown();
    void Draw(const Game& game,int mx,int my);
    void DrawTransitions(const Game& game)const;
    void Click(Game& game,int mx,int my);
    void Page(Game& game,int direction);
    void ResetView(){map_node_=screen_node_=-999;deck_page_=map_scroll_=0;}
    void ChooseUpgrade(Game& game,int local_index){game.UpgradeCard(deck_page_*8+local_index);}
    int DeckPage()const{return deck_page_;}
    void ChooseSelection(Game& game,int index){game.ChooseCard(deck_page_*8+index);}
    void ChooseTrade(Game& game,int index){game.TradeRelic(deck_page_*8+index);}
    void ChooseHand(Game& game,int index);
    std::vector<int> HandIndices(const Game& game)const;
    std::pair<float,float> MapPlayerPosition(const Game& game)const;
    static Rect CardRect(int count,int index){const int w=std::min(176,(1300-12*(count-1))/std::max(1,count)),total=count*w+(count-1)*12;return {(Width-total)/2+index*(w+12),680,w,180};}
    static Rect EndButton(){return ContentRect({1220,635,170,48});}
    static Rect StarSlotRect(int slot){return ContentRect({450+slot*190,627,180,76});}
    static Rect EnemyStarRect(int slot){return ContentRect({955+slot*139,235,130,90});}
    static Rect WaitButton(){return ContentRect({70,674,180,30});}
    static Rect StarHandRect(int count,int index){auto r=CardRect(count,index);r.y=710;r.h=150;return r;}
    static Rect UltimateButton(){return ContentRect({1040,635,160,48});}
    static Rect RestartButton(){return ContentRect({1230,42,150,40});}
    static Rect SpeedButton(int speed){return ContentRect({520+(speed-1)*70,42,60,40});}
    static Rect SaveButton(){return ContentRect({840,42,140,40});}
    static Rect LoadButton(){return ContentRect({1000,42,140,40});}
    static Rect SkipButton(){return ContentRect({1130,115,250,42});}
    static Rect ResultButton(){return ContentRect({570,540,300,54});}
    static Rect RetrySeedButton(){return {540,650,250,44};}
    static Rect NewSeedButton(){return {810,650,250,44};}
    static Rect RewardRect(int index){const int w=250,h=230,gap=24,total=3*w+2*gap;return {(Width-total)/2+index*(w+gap),360,w,h};}
    static Rect MapNodeRect(int floor,int column,bool boss,int columns=3){return ContentRect({boss?660:660-(columns-1)*105+column*210,668-floor*88,120,60});}
    static Rect MapUpButton(){return ContentRect({1150,230,200,48});}
    static Rect MapDownButton(){return ContentRect({1150,294,200,48});}
    static Rect MapChoiceRect(int index,int count){const int w=250,h=48,gap=16,total=std::max(1,count)*w+(std::max(1,count)-1)*gap;return {(Width-total)/2+index*(w+gap),788,w,h};}
    static Rect ShopRect(int index){return ContentRect({80,150+index*66,860,58});}
    static Rect LeaveButton(){return ContentRect({1000,788,300,52});}
    static Rect DeckRect(int index){return ContentRect({80+(index%4)*314,660+(index/4)*64,300,52});}
    static Rect UpgradeRect(int index){return ContentRect({120+(index%4)*300,350+(index/4)*90,280,64});}
    static Rect DeckPrevButton(){return ContentRect({80,788,160,48});}
    static Rect DeckNextButton(){return ContentRect({260,788,160,48});}
    static Rect HealButton(){return ContentRect({120,180,420,64});}
    static Rect EventChoiceRect(int index){return ContentRect({320,480+index*110,800,88});}
    static Rect PotionRect(int index){return ContentRect({36,300+index*54,168,46});}
    static Rect ClassRect(int index){return ContentRect({120+index*410,190,380,420});}
    static Rect StartButton(){return ContentRect({940,800,360,60});}
    static Rect PracticeButton(){return ContentRect({120,720,300,48});}
    static Rect BackButton(){return ContentRect({80,810,220,52});}
    static Rect SkillsButton(){return ContentRect({120,650,300,55});}
    static Rect AchievementsButton(){return ContentRect({440,650,300,55});}
    static Rect ChallengeMinus(){return ContentRect({790,650,60,55});}
    static Rect ChallengePlus(){return ContentRect({1240,650,60,55});}
    static Rect SkillRect(int branch,int tier){return ContentRect({80+branch*430,210+tier*78,400,66});}
    static Rect AudioButton(){return ContentRect({1150,42,60,40});}
    static Rect FeedbackButton(){return {810,42,100,40};}
    static Rect DifficultyButton(){return {920,86,300,22};}
    static Rect DrawPileButton(){return {345,630,125,24};}
    static Rect DiscardPileButton(){return {345,657,125,24};}
    static Rect PilePrevButton(){return {470,574,150,32};}
    static Rect PileNextButton(){return {650,574,150,32};}
    static Rect PileCloseButton(){return {990,574,130,32};}
};
}
