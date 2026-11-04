#pragma once
#include "game.hpp"
#include <unordered_map>
namespace undergod {
struct Rect { int x,y,w,h; bool Contains(int mx,int my)const{return mx>=x&&mx<x+w&&my>=y&&my<y+h;} };
class Ui {
    int font_=-1,small_=-1,heading_=-1,brand_=-1,stage_=-1;
    std::unordered_map<std::string,int> images_;
    void Text(int x,int y,const std::string& text,unsigned color,int font=-1)const;
    void Center(int x,int y,const std::string& text,unsigned color,int font=-1)const;
    void Box(Rect r,unsigned fill,int radius=12)const;
    void Button(Rect r,const std::string& text,bool active,int mx,int my)const;
    void World(const Game& game);
    void Actor(const Game& game,bool enemy)const;
    void Card(const CardDef& card,Rect r,int index,bool affordable,bool locked,int mx,int my)const;
    int Image(const std::string& key)const;
public:
    static constexpr int Width=1440,Height=960,ExpectedAssets=50;
    int loaded=0;
    bool Init(const Config& config);
    void Shutdown();
    void Draw(const Game& game,int mx,int my);
    void Click(Game& game,int mx,int my);
    static Rect CardRect(int count,int index){const int w=std::min(176,(1300-12*(count-1))/std::max(1,count)),total=count*w+(count-1)*12;return {(Width-total)/2+index*(w+12),710,w,195};}
    static Rect EndButton(){return {1220,635,170,48};}
    static Rect UltimateButton(){return {1040,635,160,48};}
    static Rect RestartButton(){return {1230,42,150,40};}
    static Rect SpeedButton(int speed){return {520+(speed-1)*70,42,60,40};}
    static Rect SkipButton(){return {1130,115,250,42};}
    static Rect ResultButton(){return {570,540,300,54};}
    static Rect RewardRect(int index){const int w=250,h=230,gap=24,total=3*w+2*gap;return {(Width-total)/2+index*(w+gap),360,w,h};}
    static Rect MapNodeRect(int floor,int column,bool boss){const int x=boss?675:400+column*240;const int y=730-floor*108;return {x,y,96,62};}
    static Rect MapChoiceRect(int index,int count){const int w=250,h=48,gap=16,total=std::max(1,count)*w+(std::max(1,count)-1)*gap;return {(Width-total)/2+index*(w+gap),848,w,h};}
    static Rect ShopRect(int index){return {80,150+index*66,860,58};}
    static Rect LeaveButton(){return {1000,848,300,52};}
    static Rect DeckRect(int index){return {80+(index%8)*150,780,140,42};}
    static Rect HealButton(){return {120,180,420,64};}
    static Rect EventChoiceRect(int index){return {320,480+index*110,800,88};}
    static Rect PotionRect(int index){return {36,300+index*54,168,46};}
};
}
