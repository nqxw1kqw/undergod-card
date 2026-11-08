#pragma once
#include "game.hpp"
#include <unordered_map>
namespace undergod {
struct Rect { int x,y,w,h; bool Contains(int mx,int my)const{return mx>=x&&mx<x+w&&my>=y&&my<y+h;} };
class Ui {
    int font_=-1,small_=-1,heading_=-1,brand_=-1,stage_=-1;
    std::unordered_map<std::string,int> images_;
    void Text(int x,int y,const std::string& text,unsigned color,int font=-1)const;
    void Wrapped(int x,int y,int width,const std::string& text,unsigned color)const;
    void Center(int x,int y,const std::string& text,unsigned color,int font=-1)const;
    void Box(Rect r,unsigned fill,int radius=12)const;
    void Button(Rect r,const std::string& text,bool active,int mx,int my)const;
    void World(const Game& game);
    void Actor(const Game& game,bool enemy)const;
    void Card(const Combat& state,const CardDef& card,Rect r,int index,bool affordable,bool locked,int mx,int my)const;
    int Image(const std::string& key)const;
public:
    static constexpr int Width=1440,Height=960,ExpectedAssets=50;
    int loaded=0;
    bool Init(const Config& config);
    void Shutdown();
    void Draw(const Game& game,int mx,int my);
    void Click(Game& game,int mx,int my);
    static Rect CardRect(int count,int index){auto r=Game::HandBox(count,index);return {int(r.x),int(r.y),int(r.w),int(r.h)};}
    static Rect PileButton(int pile){return {275,630+(pile-1)*28,175,26};}
    static Rect DetailPlayButton(){return {910,552,300,40};}
    static Rect EndButton(){return {1220,635,170,48};}
    static Rect UltimateButton(){return {1040,635,160,48};}
    static Rect RestartButton(){return {1230,42,150,40};}
    static Rect SpeedButton(int speed){return {520+(speed-1)*70,42,60,40};}
    static Rect DifficultyButton(){return {880,42,310,40};}
    static Rect FeedbackButton(){return {740,42,130,40};}
    static Rect SkipButton(){return {1130,115,250,42};}
    static Rect RetryButton(){return {570,600,300,30};}
    static Rect ResultButton(){return {570,540,300,54};}
};
}
