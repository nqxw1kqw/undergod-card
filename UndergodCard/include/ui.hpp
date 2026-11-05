#pragma once
#include "game.hpp"
#include <unordered_map>
namespace undergod {
struct Rect { int x,y,w,h; bool Contains(int mx,int my)const{return mx>=x&&mx<x+w&&my>=y&&my<y+h;} };
class Ui {
    int font_=-1,small_=-1,heading_=-1,brand_=-1,stage_=-1;
    std::unordered_map<std::string,int> images_;
    int map_scroll_=0,map_node_=-999,deck_page_=0,screen_node_=-999;
    Place screen_place_=Place::Fight;
    void Text(int x,int y,const std::string& text,unsigned color,int font=-1)const;
    void Center(int x,int y,const std::string& text,unsigned color,int font=-1)const;
    void Box(Rect r,unsigned fill,int radius=12)const;
    void Button(Rect r,const std::string& text,bool active,int mx,int my)const;
    void Wrapped(Rect r,const std::string& text,unsigned color,int font=-1)const;
    void World(const Game& game);
    void Actor(const Game& game,bool enemy,int target=0)const;
    bool DrawExtensions(const Game& game,int mx,int my);
    void DrawTargets(const Game& game,int mx,int my)const;
    void Card(const CardDef& card,Rect r,int index,bool affordable,bool locked,int mx,int my)const;
    int Image(const std::string& key)const;
public:
    static constexpr int Width=1440,Height=960;
    int loaded=0,expected=0;
    bool Init(const Config& config);
    void Shutdown();
    void Draw(const Game& game,int mx,int my);
    void Click(Game& game,int mx,int my);
    void Page(const Game& game,int direction);
    void ResetView(){map_node_=screen_node_=-999;deck_page_=map_scroll_=0;}
    void ChooseUpgrade(Game& game,int local_index){game.UpgradeCard(deck_page_*8+local_index);}
    int DeckPage()const{return deck_page_;}
    void ChooseSelection(Game& game,int index){game.ChooseCard(deck_page_*8+index);}
    void ChooseTrade(Game& game,int index){game.TradeRelic(deck_page_*8+index);}
    static Rect CardRect(int count,int index){const int w=std::min(176,(1300-12*(count-1))/std::max(1,count)),total=count*w+(count-1)*12;return {(Width-total)/2+index*(w+12),710,w,195};}
    static Rect EndButton(){return {1220,635,170,48};}
    static Rect UltimateButton(){return {1040,635,160,48};}
    static Rect RestartButton(){return {1230,42,150,40};}
    static Rect SpeedButton(int speed){return {520+(speed-1)*70,42,60,40};}
    static Rect SaveButton(){return {840,42,140,40};}
    static Rect LoadButton(){return {1000,42,140,40};}
    static Rect SkipButton(){return {1130,115,250,42};}
    static Rect ResultButton(){return {570,540,300,54};}
    static Rect RewardRect(int index){const int w=250,h=230,gap=24,total=3*w+2*gap;return {(Width-total)/2+index*(w+gap),360,w,h};}
    static Rect MapNodeRect(int floor,int column,bool boss,int columns=3){return {boss?660:660-(columns-1)*105+column*210,728-floor*88,120,60};}
    static Rect MapUpButton(){return {1150,230,200,48};}
    static Rect MapDownButton(){return {1150,294,200,48};}
    static Rect MapChoiceRect(int index,int count){const int w=250,h=48,gap=16,total=std::max(1,count)*w+(std::max(1,count)-1)*gap;return {(Width-total)/2+index*(w+gap),848,w,h};}
    static Rect ShopRect(int index){return {80,150+index*66,860,58};}
    static Rect LeaveButton(){return {1000,848,300,52};}
    static Rect DeckRect(int index){return {80+(index%4)*314,700+(index/4)*64,300,52};}
    static Rect UpgradeRect(int index){return {120+(index%4)*300,350+(index/4)*90,280,64};}
    static Rect DeckPrevButton(){return {80,848,160,48};}
    static Rect DeckNextButton(){return {260,848,160,48};}
    static Rect HealButton(){return {120,180,420,64};}
    static Rect EventChoiceRect(int index){return {320,480+index*110,800,88};}
    static Rect PotionRect(int index){return {36,300+index*54,168,46};}
    static Rect ClassRect(int index){return {120+index*410,190,380,420};}
    static Rect StartButton(){return {940,800,360,60};}
    static Rect BackButton(){return {80,840,220,52};}
    static Rect SkillsButton(){return {120,650,300,55};}
    static Rect AchievementsButton(){return {440,650,300,55};}
    static Rect ChallengeMinus(){return {790,650,60,55};}
    static Rect ChallengePlus(){return {1240,650,60,55};}
    static Rect SkillRect(int branch,int tier){return {80+branch*430,210+tier*78,400,66};}
    static Rect AudioButton(){return {1150,42,60,40};}
};
}
