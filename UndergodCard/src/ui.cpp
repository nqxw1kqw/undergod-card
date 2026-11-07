#include "ui.hpp"
#include <DxLib.h>
#include <cmath>

namespace undergod {
namespace {
unsigned C(int r,int g,int b){return GetColor(r,g,b);}
const unsigned Ink=0x343948,Muted=0x7c887c; // RGB packed values match GetColor on 32-bit mode.
void Blend(int alpha){SetDrawBlendMode(alpha==255?DX_BLENDMODE_NOBLEND:DX_BLENDMODE_ALPHA,alpha);}
}
int Ui::Image(const std::string& key)const{const auto it=images_.find(key);return it==images_.end()?-1:it->second;}
bool Ui::Init(const Config& config){
    SetDrawMode(DX_DRAWMODE_BILINEAR);
    font_=CreateFontToHandle("Segoe UI",18,5,DX_FONTTYPE_ANTIALIASING_4X4);
    small_=CreateFontToHandle("Segoe UI",14,4,DX_FONTTYPE_ANTIALIASING_4X4);
    heading_=CreateFontToHandle("Segoe UI",27,7,DX_FONTTYPE_ANTIALIASING_4X4);
    brand_=CreateFontToHandle("Segoe UI",31,8,DX_FONTTYPE_ANTIALIASING_4X4);
    for(int f:{font_,small_,heading_,brand_})if(f>=0)SetFontCharCodeFormatToHandle(DX_CHARCODEFORMAT_UTF8,f);
    stage_=MakeScreen(Width,360,TRUE);
    auto load=[&](const std::string& key,const std::string& file,int size){
        const int graph=LoadGraph(("assets/pastel/"+file).c_str());images_[key]=graph;
        if(graph>=0){int w=0,h=0;GetGraphSize(graph,&w,&h);if(w==size&&h==size)++loaded;}
    };
    for(const auto& actor:std::vector<std::string>{config.player_id,"frog"})for(const auto* state:{"idle","attack","hit","ult_charge","defend"})for(int f=0;f<4;++f){
        const std::string key=std::string(actor)+"_"+state+"_"+std::to_string(f);
        load(key,"characters/char_"+key+".png",512);
    }
    for(int f=0;f<4;++f){const auto key=config.player_id+"_ult_throw_"+std::to_string(f);load(key,"characters/char_"+key+".png",512);}
    for(const auto& card:config.cards)load("card_"+card.id,"cards/"+card.id+".png",256);
    load("spear","vfx/spear.png",512);load("spear_impact","vfx/spear_impact.png",512);
    return stage_>=0&&font_>=0&&small_>=0&&heading_>=0&&brand_>=0;
}
void Ui::Shutdown(){for(const auto& pair:images_)if(pair.second>=0)DeleteGraph(pair.second);images_.clear();if(stage_>=0)DeleteGraph(stage_);stage_=-1;for(int f:{font_,small_,heading_,brand_})if(f>=0)DeleteFontToHandle(f);}
void Ui::Text(int x,int y,const std::string& text,unsigned color,int font)const{DrawStringToHandle(x,y,text.c_str(),color,font<0?font_:font);}
void Ui::Center(int x,int y,const std::string& text,unsigned color,int font)const{if(font<0)font=font_;const int w=GetDrawStringWidthToHandle(text.c_str(),int(text.size()),font);Text(x-w/2,y,text,color,font);}
void Ui::Box(Rect r,unsigned fill,int radius)const{DrawRoundRect(r.x,r.y,r.x+r.w,r.y+r.h,radius,radius,fill,TRUE);}
void Ui::Button(Rect r,const std::string& text,bool active,int mx,int my)const{
    const bool hover=active&&r.Contains(mx,my);Box(r,active?(hover?C(93,125,105):C(79,106,91)):C(223,225,216));
    Center(r.x+r.w/2,r.y+(r.h-18)/2,text,active?C(255,255,246):C(151,159,147),small_);
}
void Ui::Actor(const Game& game,bool enemy)const{
    const auto& pose=game.poses[enemy?1:0];const std::string id=enemy?"frog":game.state.config.player_id;
    const float fps=pose.name=="ult_throw"?4.f:8.f;
    const int frame=pose.name=="idle"?int(pose.age*4)%4:std::min(3,int(pose.age*fps));
    int graph=Image(id+"_"+pose.name+"_"+std::to_string(frame));if(graph<0)graph=Image(id+"_idle_0");
    const float x=(enemy?Game::EnemyHome:Game::PlayerHome)+game.ActorOffset(enemy),y=320-std::sin(game.time*3.14f)*2;
    Blend(38);DrawOval(int(x),324,enemy?70:46,9,C(80,107,76),TRUE);Blend(255);
    if(graph>=0){
        DrawRotaGraph3F(x,y,256,456,.5,.5,0,graph,TRUE,enemy?TRUE:FALSE);
        if(pose.name=="hit"&&pose.age<.1f){SetDrawBlendMode(DX_BLENDMODE_ADD,150);DrawRotaGraph3F(x,y,256,456,.5,.5,0,graph,TRUE,enemy?TRUE:FALSE);Blend(255);}
    }else{DrawOval(int(x),int(y-72),enemy?65:43,72,enemy?C(165,197,135):C(255,229,139),TRUE);Text(int(x-32),int(y-74),enemy?"Ếch":"Gà",Ink,small_);}
    if(pose.name=="defend"&&pose.age<.6f){const int shield=Image("card_shield");if(shield>=0){Blend(int(255*(1-pose.age/.65f)));DrawRotaGraph3F(x,y-160-pose.age*30,128,128,.24,.24,0,shield,TRUE);Blend(255);}}
}
void Ui::World(const Game& game){
    SetDrawScreen(stage_);SetDrawArea(0,0,Width,360);ClearDrawScreen();
    for(int y=0;y<360;++y){const float f=y/360.f;DrawLine(0,y,Width,y,C(int(227+17*f),int(240-3*f),int(237-23*f)));}
    Blend(145);for(int i=0;i<5;++i)DrawOval(int(i*330+40+std::sin(game.time*.08f+i)*60),45+(i%2)*40,90,17,C(255,255,255),TRUE);Blend(255);
    for(int i=0;i<7;++i)DrawOval(int(i*280+std::sin(game.time*.1f)*8),330,240,170,C(196,217,191),TRUE);
    for(int i=0;i<6;++i)DrawOval(int(i*330-70-std::sin(game.time*.15f)*14),360,210,150,C(174,201,164),TRUE);
    DrawBox(0,306,Width,360,C(220,227,182),TRUE);DrawBox(0,306,Width,310,C(143,169,124),TRUE);
    for(int i=0;i<40;++i){const int x=i*37;DrawLine(x,337,x+18,337,C(173,188,146));DrawCircle(x+12,316+(i%4)*8,3,i%3?C(246,239,205):C(238,185,194),TRUE);}
    Actor(game,true);
    if(game.Cinematic()){Blend(int(game.visual.dim*255));DrawBox(0,0,Width,360,C(33,28,60),TRUE);Blend(255);}
    Actor(game,false);
    if(game.visual.spear.visible){const auto tip=game.SpearTip();const int spear=Image("spear");if(spear>=0)DrawRotaGraph3F(tip.first,tip.second,256,56,.45,.45,game.visual.spear.falling?3.141592653589793:0,spear,TRUE);}
    if(game.visual.vfx_age>=0&&game.visual.vfx_age<game.visual.vfx_frames/8.f){const int frame=std::min(game.visual.vfx_frames-1,int(game.visual.vfx_age*8)),vfx=Image("spear_impact");if(vfx>=0)DrawRectRotaGraph3F(game.visual.vfx_enemy?1120.f:320.f,195,(frame%2)*256,(frame/2)*256,256,256,128,128,1,1,0,vfx,TRUE);}
    for(const auto& popup:game.popups){const auto color=popup.type==EventType::Heal?C(90,142,98):((popup.type==EventType::Block||popup.blocked)?C(84,130,150):C(172,85,106));const auto suffix=popup.type==EventType::Heal?" HP":(popup.type==EventType::Block?" giáp":"");const std::string value=(popup.blocked?"Chặn ":(popup.type==EventType::Damage?"−":"+"))+std::to_string(popup.amount)+suffix;Blend(int(255*(1-popup.age/.95f)));Center(popup.enemy?1120:320,int(140-popup.age*70),value,color,heading_);Blend(255);}
    if(game.visual.flash_left>0){Blend(int(255*game.visual.flash_left/std::max(.01f,game.visual.flash_duration)));DrawBox(0,0,Width,360,C(255,255,255),TRUE);Blend(255);}
    SetDrawScreen(DX_SCREEN_BACK);SetDrawArea(20,250,Width-20,610);
    const float shake=game.visual.shake_left>0?std::sin(game.time*95)*game.visual.shake:0;
    DrawRotaGraph3F(Width/2.f+shake,430,Width/2.f,180,game.visual.zoom,game.visual.zoom,0,stage_,TRUE);
    SetDrawArea(0,0,Width,Height);
}
void Ui::Card(const CardDef& card,Rect r,int index,bool affordable,bool locked,int mx,int my)const{
    if(!locked&&r.Contains(mx,my))r.y-=8;
    Box({r.x,r.y+4,r.w,r.h},C(225,218,202));Box(r,C(221,214,201));Box({r.x+1,r.y+1,r.w-2,r.h-2},C(255,252,246));
    const auto bg=card.id=="shield"?C(227,236,235):(card.id=="heavy"?C(233,226,241):(card.id=="cookie"?C(244,236,204):C(242,230,223)));
    Box({r.x+12,r.y+20,r.w-24,90},bg,8);
    const int graph=Image("card_"+card.id);if(!affordable||locked)Blend(95);
    if(graph>=0)DrawRotaGraph3F(r.x+r.w/2.f,float(r.y+65),128,128,.42,.42,0,graph,TRUE);else Center(r.x+r.w/2,r.y+40,"✦",Muted,heading_);Blend(255);
    DrawCircle(r.x+21,r.y+20,15,C(255,255,255),TRUE);Center(r.x+21,r.y+10,std::to_string(card.cost),C(123,91,153),small_);
    Text(r.x+r.w-20,r.y+10,std::to_string(index+1),Muted,small_);
    Text(r.x+12,r.y+127,card.name,affordable&&!locked?Ink:Muted,small_);
    Text(r.x+12,r.y+156,card.description,Muted,small_);
}
void Ui::Draw(const Game& game,int mx,int my){
    const auto& s=game.state;DrawBox(0,0,Width,Height,C(232,233,227),TRUE);Box({20,20,Width-40,Height-40},C(251,249,243),22);
    Text(55,42,"✦ CARD QUEST",C(75,93,82),brand_);Text(82,78,"UNDERGOD  /  C++ · DXLIB",Muted,small_);
    Text(880,53,"01 / ĐỒNG CỎ ÊM ĐỀM",Muted,small_);Button(RestartButton(),"Trận mới ↻",!game.Busy(),mx,my);
    Text(430,53,"Tốc độ",Muted,small_);
    for(int speed=1;speed<=3;++speed){const Rect r=SpeedButton(speed);const bool selected=game.Speed()==speed;Box(r,selected?C(79,106,91):(r.Contains(mx,my)?C(226,229,218):C(239,237,229)));Center(r.x+r.w/2,r.y+11,"x"+std::to_string(speed),selected?C(255,255,246):Ink,small_);}
    DrawBox(20,110,Width-20,250,C(227,240,237),TRUE);
    auto health=[&](bool enemy){const auto& actor=enemy?s.enemy:s.player;int x=enemy?1100:80;
        Text(x,123,enemy?"NGƯỜI GIỮ KHU VƯỜN":"NGƯỜI LỮ HÀNH",Muted,small_);
        Text(x,148,enemy?s.config.enemy.name:s.config.player_name,Ink,heading_);
        Box({x,185,260,22},C(203,214,200),8);Box({x,185,std::max(1,260*actor.hp/actor.max_hp),22},enemy?C(157,187,131):C(237,159,166),8);
        Center(x+130,187,std::to_string(actor.hp)+" / "+std::to_string(actor.max_hp)+" HP",Ink,small_);
        Text(x,212,"Giáp: "+std::to_string(actor.block),Muted,small_);
    };health(false);health(true);
    Box({80,239,260,5},C(209,215,210),2);if(s.rage>0)Box({80,239,260*s.rage/s.config.balance.rage_max,5},C(173,146,212),2);
    Text(360,216,"Nộ "+std::to_string(s.rage)+" / "+std::to_string(s.config.balance.rage_max),C(123,91,153),small_);
    Center(720,131,"LƯỢT "+std::to_string(s.turn),Muted,small_);Center(720,162,game.Cinematic()?"Sao Rơi":(game.Busy()?"Đang hành động…":"Lượt của bạn"),Ink,font_);
    Center(720,198,"Lá nhỏ, chuyến phiêu lưu lớn.",Muted,small_);
    World(game);
    // Intent remains visible above the opponent, outside the zoomed arena.
    if(!game.Cinematic()){Box({1060,257,300,32},C(255,249,237),9);Center(1210,264,s.intent.attack?"Sắp tấn công: "+std::to_string(s.intent.value):"Sắp nhận "+std::to_string(s.intent.value)+" giáp",Ink,small_);}
    DrawLine(20,610,Width-20,610,C(219,219,202));
    DrawCircle(90,659,25,C(233,221,244),TRUE);Center(90,647,std::to_string(s.energy)+"/"+std::to_string(s.config.balance.energy),C(123,91,153),font_);
    Text(128,643,"NĂNG LƯỢNG",Muted,small_);Text(280,639,"Rút: "+std::to_string(s.draw.size()),Muted,small_);Text(280,663,"Bỏ: "+std::to_string(s.discard.size()),Muted,small_);
    Text(470,652,"1–5: bài  ·  E: hết lượt  ·  U: Sao Rơi",Muted,small_);
    Button(UltimateButton(),"✦ Sao Rơi",game.CanAct()&&s.rage>=s.config.balance.rage_max,mx,my);Button(EndButton(),"Kết thúc lượt →",game.CanAct(),mx,my);
    for(size_t i=0;i<s.hand.size();++i)Card(s.Definition(s.hand[i]),CardRect(int(s.hand.size()),int(i)),int(i),s.CanPlay(int(i)),!game.CanAct(),mx,my);
    Text(55,922,"Hãy đọc ý định của Ếch Vương trước khi chọn bài.",Muted,small_);Text(1090,922,"PHIÊN BẢN THỬ NGHIỆM",Muted,small_);
    if(game.toast_age>0){Box({420,570,600,43},C(55,72,62));Center(720,581,game.toast,C(255,255,246),small_);}
    if(game.Cinematic())Button(SkipButton(),"Bỏ qua · Space / Esc",true,mx,my);
    if(game.Result()){
        Blend(210);DrawBox(0,0,Width,Height,C(38,57,44),TRUE);Blend(255);
        Box({440,305,560,330},C(252,249,239),24);Center(720,338,"✦",C(174,144,199),brand_);
        Center(720,395,s.phase==Phase::Won?"Khu vườn nở hoa!":"Nghỉ một chút nhé!",Ink,brand_);
        Center(720,454,s.phase==Phase::Won?"Bạn đã vượt qua Ếch Vương.":"Ếch Vương thắng lần này. Hãy thử cách khác.",Muted,font_);
        Center(720,491,"Hành trình kéo dài "+std::to_string(s.turn)+" lượt.",Muted,small_);Button(ResultButton(),"Trận mới ↻",true,mx,my);Button(RetryButton(),"Thử lại seed "+std::to_string(game.Seed()),true,mx,my);
    }
}
void Ui::Click(Game& game,int mx,int my){
    for(int speed=1;speed<=3;++speed)if(SpeedButton(speed).Contains(mx,my)){game.SetSpeed(speed);return;}
    if(game.Cinematic()){if(SkipButton().Contains(mx,my))game.SkipCinematic();return;}
    if(game.Result()){if(RetryButton().Contains(mx,my)){game.Retry();return;}if(ResultButton().Contains(mx,my)||RestartButton().Contains(mx,my))game.NewMatch();return;}
    if(game.Busy())return;
    if(RestartButton().Contains(mx,my)){game.NewMatch();return;}
    if(EndButton().Contains(mx,my)){game.EndTurn();return;}
    if(UltimateButton().Contains(mx,my)){game.Ultimate();return;}
    const int count=int(game.state.hand.size());for(int i=0;i<count;++i)if(CardRect(count,i).Contains(mx,my)){game.Play(i);return;}
}
}
