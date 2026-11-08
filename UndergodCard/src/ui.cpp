#include "ui.hpp"
#include <DxLib.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <set>
#include "../third_party/nlohmann/json.hpp"

namespace undergod {
namespace {
unsigned C(int r,int g,int b){return GetColor(r,g,b);}
const unsigned Ink=0x343948,Muted=0x7c887c; // RGB packed values match GetColor on 32-bit mode.
void Blend(int alpha){SetDrawBlendMode(alpha==255?DX_BLENDMODE_NOBLEND:DX_BLENDMODE_ALPHA,alpha);}
}
int Ui::Image(const std::string& key)const{const auto it=images_.find(key);return it==images_.end()?-1:it->second;}
bool Ui::Init(const Config& config){
    {std::ifstream file("assets/manifest.json");if(!file)return false;nlohmann::json manifest;file>>manifest;expected=manifest.at("runtime_pngs");
        for(const auto& item:manifest.at("files")){const std::string path=item.at("path");if(item.contains("bounds")&&path.find("/skill_fx/")!=std::string::npos){const auto& b=item.at("bounds");image_bounds_["skill_"+std::filesystem::path(path).stem().string()]={b[0],b[1],int(b[2])-int(b[0]),int(b[3])-int(b[1])};}}
    }
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
    std::set<std::string> actors;for(const auto& c:config.classes)actors.insert(c.sprite);for(const auto& e:config.bestiary)actors.insert(e.sprite);
    for(const auto& actor:actors)for(const auto* state:{"idle","attack","hit","ult_charge","defend","ult_throw"})for(int f=0;f<4;++f){
        const std::string key=std::string(actor)+"_"+state+"_"+std::to_string(f);
        const auto file="characters/char_"+key+".png";if(std::filesystem::exists("assets/pastel/"+file))load(key,file,512);
    }
    for(const auto& b:config.animations.bodies)for(int f=0;f<4;++f){const auto key=b.actor+"_"+b.pose+"_"+std::to_string(f);load(key,"characters/char_"+key+".png",512);}
    for(const auto& actor:actors){
        auto file="assets/pastel/characters/char_"+actor+"_hit_3.png";
        if(!std::filesystem::exists(file))file="assets/pastel/characters/char_"+actor+"_idle_0.png";
        const int gray=LoadGraph(file.c_str());if(gray>=0&&GraphFilter(gray,DX_GRAPH_FILTER_MONO,0,0)==0)++grayscale;
        images_[actor+"_defeated"]=gray;
    }
    for(const auto& effect:config.animations.effects)for(int f=0;f<4;++f){const auto key=effect+"_"+std::to_string(f);load("skill_"+key,"skill_fx/"+key+".png",512);}
    for(const auto& card:config.cards)if(!card.upgraded){const auto file="cards/"+card.id+".png";load("card_"+card.id,file,256);}
    for(const auto& card:config.boss_cards)load("card_"+card.id,"boss_cards/"+card.id+".png",256);
    for(const auto& relic:config.relics)load("relic_"+relic.id,"relics/"+relic.id+".png",256);
    for(const auto& potion:config.potions)load("potion_"+potion.id,"potions/"+potion.id+".png",256);
    load("spear","vfx/spear.png",512);load("spear_impact","vfx/spear_impact.png",512);
    load("feather","vfx/feather.png",256);load("kunai","vfx/kunai.png",256);
    const int fight=LoadGraph("assets/pastel/ui/fight.png");images_["fight"]=fight;
    if(fight>=0){int w=0,h=0;GetGraphSize(fight,&w,&h);if(w>0&&h>0)++loaded;}
    return stage_>=0&&font_>=0&&small_>=0&&heading_>=0&&brand_>=0&&loaded==expected;
}
void Ui::Shutdown(){for(const auto& pair:images_)if(pair.second>=0)DeleteGraph(pair.second);images_.clear();if(stage_>=0)DeleteGraph(stage_);stage_=-1;for(int f:{font_,small_,heading_,brand_})if(f>=0)DeleteFontToHandle(f);}
void Ui::Text(int x,int y,const std::string& text,unsigned color,int font)const{DrawStringToHandle(x,y,text.c_str(),color,font<0?font_:font);}
void Ui::Center(int x,int y,const std::string& text,unsigned color,int font)const{if(font<0)font=font_;const int w=GetDrawStringWidthToHandle(text.c_str(),int(text.size()),font);Text(x-w/2,y,text,color,font);}
void Ui::Box(Rect r,unsigned fill,int radius)const{DrawRoundRect(r.x,r.y,r.x+r.w,r.y+r.h,radius,radius,fill,TRUE);}
void Ui::Button(Rect r,const std::string& text,bool active,int mx,int my)const{
    const bool hover=active&&r.Contains(mx,my);Box(r,active?(hover?C(93,125,105):C(79,106,91)):C(223,225,216));
    Center(r.x+r.w/2,r.y+(r.h-18)/2,text,active?C(255,255,246):C(151,159,147),small_);
}
void Ui::Wrapped(Rect r,const std::string& text,unsigned color,int font)const{
    if(font<0)font=small_;const int step=font==font_?24:19;
    std::string line,word;std::vector<std::string> lines;
    auto width=[&](const std::string& s){return GetDrawStringWidthToHandle(s.c_str(),int(s.size()),font);};
    auto push=[&](){if(word.empty())return;const auto next=line.empty()?word:line+" "+word;if(!line.empty()&&width(next)>r.w){lines.push_back(line);line=word;}else line=next;word.clear();};
    for(char ch:text){if(ch==' '||ch=='\n'){push();if(ch=='\n'){lines.push_back(line);line.clear();}}else word+=ch;}
    push();if(!line.empty())lines.push_back(line);
    const int limit=std::max(1,r.h/step);
    for(int i=0;i<std::min(limit,int(lines.size()));++i)Text(r.x,r.y+i*step,lines[i],color,font);
}
void Ui::Page(Game& game,int direction){
    if(game.pile_view){const int pages=std::max(1,(int(Game::PileCounts(game.pile_view==1?game.state.draw:game.state.discard).size())+13)/14);game.pile_page=std::clamp(game.pile_page+direction,0,pages-1);return;}
    if(game.Map())map_scroll_=std::clamp(map_scroll_+direction*3,0,std::max(0,game.run.map.back().floor-5));
    else if(game.Shop()||game.Rest()||game.CardSelection()||game.RelicSelection()){const int count=game.RelicSelection()?int(game.run.relics.size()):int(game.run.deck.size());deck_page_=std::clamp(deck_page_+direction,0,std::max(0,(count-1)/8));}
}
void Ui::Actor(const Game& game,bool enemy,int target)const{
    if(game.TurnStarRules()){SkillActor(game,enemy,target);return;}
    if(enemy&&game.state.Target(target).hp<=0)return;
    const Pose idle;const auto& pose=enemy&&game.ResolvingStars()&&game.ActiveActor()!=target?idle:game.poses[enemy?1:0];const std::string id=enemy?game.state.Sprite(target):FindClass(game.state.config,game.run.class_id).sprite;
    const float fps=pose.name=="ult_throw"?4.f:8.f;
    const int frame=pose.name=="idle"?int(pose.age*4)%4:std::min(3,int(pose.age*fps));
    int graph=Image(id+"_"+pose.name+"_"+std::to_string(frame));if(graph<0)graph=Image(id+"_idle_0");
    const float x=SceneX(enemy?game.TargetX(target):Game::PlayerHome)+game.ActorOffset(enemy,target),y=320-std::sin(game.time*3.14f)*2;
    const float base_scale=enemy&&id=="tiger"?.65f:.5f;
    const float scale=enemy&&!game.state.adds.empty()?(target==0||game.state.adds[target-1].illusion?.4f:.27f):base_scale;
    Blend(38);DrawOval(int(x),324,enemy?70:46,9,C(80,107,76),TRUE);Blend(255);
    if(graph>=0){
        DrawRotaGraph3F(x,y,256,456,scale,scale,0,graph,TRUE,FALSE);
        if(pose.name=="hit"&&pose.age<.1f){SetDrawBlendMode(DX_BLENDMODE_ADD,150);DrawRotaGraph3F(x,y,256,456,scale,scale,0,graph,TRUE,FALSE);Blend(255);}
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
    if(game.TurnStarRules()){for(int target=0;target<=int(game.state.adds.size());++target)Actor(game,true,target);}
    else for(int target:game.state.Targets())Actor(game,true,target);
    if(game.Cinematic()){Blend(int(game.visual.dim*(game.ReducedFeedback()?70:255)));DrawBox(0,0,Width,360,C(33,28,60),TRUE);Blend(255);}
    Actor(game,false);
    if(game.TurnStarRules()){
        DrawWard(game,-1);for(int target=0;target<=int(game.state.adds.size());++target)DrawWard(game,target);
        DrawSkillEffects(game);
    }
    if(game.Cinematic()&&game.state.Mods().ultimate!="spear"){
        const float age=game.CinematicTime();const auto mode=game.state.Mods().ultimate;
        if(mode=="storm"){
            const int feather=Image("feather");const auto targets=game.state.Targets();
            if(feather>=0)for(int i=0;i<4;++i){const float launch=std::clamp((age-1.3f)/1.3f,0.f,1.f);const float end=targets.empty()?1120.f:game.TargetX(targets[i%targets.size()]);const float x=320+(end-320)*launch+std::sin(age*6+i*1.57f)*30*(1-launch),y=175-std::sin(launch*3.14159f)*110;DrawRotaGraphF(SceneX(x),y,.42,age*3+i,feather,TRUE);}
        }else{
            const int clone=Image("ninja_ult_throw_"+std::to_string(std::min(3,int(game.poses[0].age*4))));
            if(clone>=0&&age>.7f){Blend(110);for(int i=0;i<3;++i)DrawRotaGraph3F(SceneX(440.f+i*180+std::min(1.f,(age-.7f)/1.6f)*120),320,256,456,.5,.5,0,clone,TRUE);Blend(255);}
        }
    }
    if(game.visual.spear.visible){const auto tip=game.SpearTip();const int spear=Image("spear");if(spear>=0)DrawRotaGraph3F(SceneX(tip.first),tip.second,256,56,.45,.45,game.visual.spear.falling?3.141592653589793:0,spear,TRUE);}
    if(game.visual.vfx_age>=0&&game.visual.vfx_age<game.visual.vfx_frames/8.f){const int frame=std::min(game.visual.vfx_frames-1,int(game.visual.vfx_age*8)),vfx=Image("spear_impact");if(vfx>=0)DrawRectRotaGraph3F(SceneX(game.visual.vfx_enemy?1120.f:320.f),195,(frame%2)*256,(frame/2)*256,256,256,128,128,1,1,0,vfx,TRUE);}
    for(const auto& p:game.particles){const float t=p.age/p.life;Blend(int(170*(1-t)));const auto color=p.type==EventType::Heal?C(90,170,100):(p.type==EventType::Block?C(100,170,210):C(235,185,100));DrawCircle(int(SceneX(p.x+p.vx*p.age)),int(p.y+p.vy*p.age+160*p.age*p.age),p.type==EventType::Damage?3:4,color,TRUE);Blend(255);}
    for(const auto& popup:game.popups){
        const bool status=popup.type==EventType::Status;
        const auto color=popup.type==EventType::Heal||status?C(90,142,98):((popup.type==EventType::Block||popup.blocked)?C(84,130,150):C(172,85,106));
        std::string value;
        if(status)value="+"+std::to_string(popup.amount)+" "+StatusName(popup.status);
        else{const auto suffix=popup.type==EventType::Heal?" HP":(popup.type==EventType::Block?" giáp":"");value=(popup.blocked?"Chặn ":(popup.type==EventType::Damage?"−":"+"))+std::to_string(popup.amount)+suffix;}
        Blend(int(255*(1-popup.age/.95f)));Center(int(SceneX(popup.enemy?game.TargetX(popup.target):320))+(popup.lane/4)*(popup.enemy?-105:105),int(165-popup.age*55)-(popup.lane%4)*24,value,color,heading_);Blend(255);
    }
    if(!game.ReducedFeedback()&&game.visual.flash_left>0){Blend(int(255*game.visual.flash_left/std::max(.01f,game.visual.flash_duration)));DrawBox(0,0,Width,360,C(255,255,255),TRUE);Blend(255);}
    SetDrawScreen(DX_SCREEN_BACK);SetDrawArea(20,250,Width-20,610);
    const float shake=!game.ReducedFeedback()&&game.visual.shake_left>0?std::sin(game.time*95)*game.visual.shake*std::min(1.f,game.visual.shake_left/.18f):0;
    DrawRotaGraph3F(Width/2.f+shake,430,Width/2.f,180,(game.ReducedFeedback()?1:game.visual.zoom),(game.ReducedFeedback()?1:game.visual.zoom),0,stage_,TRUE);
    SetDrawArea(0,0,Width,Height);
}
void Ui::Card(const CardDef& original,Rect r,int index,bool affordable,bool locked,int mx,int my)const{
    auto card=original;if(star_cards_)card.cost=1;
    if(!locked&&r.Contains(mx,my))r.y-=8;
    const unsigned stripe=card.rarity=="boss"?C(157,101,117):(card.rarity=="legendary"?C(188,143,58):(card.rarity=="curse"?C(128,99,147):(card.rarity=="rare"?C(125,141,178):(card.rarity=="uncommon"?C(90,142,120):C(196,154,132)))));
    Box({r.x,r.y+4,r.w,r.h},C(225,218,202));Box(r,C(221,214,201));Box({r.x+1,r.y+1,r.w-2,r.h-2},C(255,252,246));
    const auto bg=card.poison>0?C(214,230,214):(card.heal>0?C(244,236,204):(card.block>0&&card.damage==0?C(227,236,235):(card.rarity=="rare"?C(236,228,206):C(242,230,223))));
    Box({r.x+12,r.y+20,r.w-24,std::min(90,r.h-104)},bg,8);
    const int graph=Image("card_"+(card.upgraded?card.id.substr(0,card.id.size()-1):card.id));if(!affordable||locked)Blend(95);
    const bool compact=r.h<190;
    if(graph>=0)DrawRotaGraph3F(r.x+r.w/2.f,float(r.y+(compact?43:65)),128,128,compact?.24:.42,compact?.24:.42,0,graph,TRUE,FALSE);else Center(r.x+r.w/2,r.y+40,"✦",Muted,heading_);Blend(255);
    DrawBox(r.x+18,r.y+12,r.x+r.w-18,r.y+16,stripe,TRUE);
    DrawCircle(r.x+21,r.y+28,15,C(255,255,255),TRUE);Center(r.x+21,r.y+18,std::to_string(card.cost),C(123,91,153),small_);
    if(index>=0)Text(r.x+r.w-20,r.y+16,std::to_string(index+1),Muted,small_);
    const int title_y=r.y+std::min(126,r.h-75);
    Wrapped({r.x+12,title_y,r.w-24,20},card.name,affordable&&!locked?Ink:Muted);
    if(!card.keywords.empty())Text(r.x+12,title_y+18,card.keywords.front(),C(123,91,153),small_);
    const int max_w=r.w-24;
    std::string line,word;
    std::vector<std::string> lines;
    auto measure=[&](const std::string& text){return GetDrawStringWidthToHandle(text.c_str(),int(text.size()),small_);};
    auto push=[&](){
        if(word.empty())return;
        const std::string trial=line.empty()?word:line+" "+word;
        if(!line.empty()&&measure(trial)>max_w){lines.push_back(line);line=word;}
        else line=trial;
        word.clear();
    };
    for(unsigned char ch:card.description){if(ch==' ')push();else word.push_back(char(ch));}
    push();
    if(!line.empty())lines.push_back(line);
    const int text_y=title_y+(card.keywords.empty()?23:38),limit=std::max(1,(r.y+r.h-14-text_y)/18+1);
    for(int i=0;i<std::min(limit,int(lines.size()));++i)Text(r.x+12,text_y+i*18,lines[i],Muted,small_);
}
void Ui::Draw(const Game& game,int mx,int my){
    struct FinishFrame { const Ui& ui;const Game& game;int mx,my;~FinishFrame(){ui.DrawPresentation(game,mx,my);ui.DrawTransitions(game);} } finish{*this,game,mx,my};
    star_cards_=game.TurnStarRules();
    const auto& s=game.state;
    if(screen_place_!=game.run.place||screen_node_!=game.run.node){deck_page_=0;screen_place_=game.run.place;screen_node_=game.run.node;}
    deck_page_=std::clamp(deck_page_,0,std::max(0,(int(game.RelicSelection()?game.run.relics.size():game.run.deck.size())-1)/8));
    if(map_node_!=game.run.node){map_node_=game.run.node;const int floor=game.run.node>=0?game.run.map[game.run.node].floor:0;map_scroll_=std::clamp(floor-2,0,std::max(0,game.run.map.empty()?0:game.run.map.back().floor-5));}
    if(game.MapTravelPending()&&game.TravelTo()>=0){const int floor=game.run.map.at(game.TravelTo()).floor;if(floor-map_scroll_<0||floor-map_scroll_>5)map_scroll_=std::clamp(floor-2,0,std::max(0,game.run.map.back().floor-5));}
    if(DrawExtensions(game,mx,my))return;
    auto backdrop=[&](){
        DrawBox(0,0,Width,Height,C(232,233,227),TRUE);Box({20,20,Width-40,Height-40},C(251,249,243),22);
        Text(135,42,"✦ CARD QUEST",C(75,93,82),brand_);Text(162,78,"UNDERGOD  /  C++ · DXLIB",Muted,small_);
        Button(RestartButton(),"Chơi lại ↻",!game.Busy(),mx,my);Button(SaveButton(),"F6 · Lưu",!game.Busy()&&!game.Practice(),mx,my);Button(LoadButton(),"F9 · Tiếp tục",!game.Busy(),mx,my);
        Button(FeedbackButton(),game.ReducedFeedback()?"FX: nhẹ":"FX: đầy",true,mx,my);
        Button(AudioButton(),game.profile.muted?"Tắt ♪":"Bật ♪",true,mx,my);
        Text(1240,87,"Vàng: "+std::to_string(game.run.gold),C(146,112,48),small_);Text(510,53,"Tốc độ",Muted,small_);
        for(int speed=1;speed<=3;++speed){const Rect r=SpeedButton(speed);const bool selected=game.Speed()==speed;Box(r,selected?C(79,106,91):(r.Contains(mx,my)?C(226,229,218):C(239,237,229)));Center(r.x+r.w/2,r.y+11,"x"+std::to_string(speed),selected?C(255,255,246):Ink,small_);}
        if(game.toast_age>0){Box({500,842,680,32},C(55,72,62));Wrapped({515,848,650,22},game.toast,C(255,255,246));}
    };
    if(game.Result()){
        backdrop();Box({520,305,560,330},C(252,249,239),24);Center(800,338,"✦",C(174,144,199),brand_);
        Center(800,395,game.Practice()?(game.run.victory?"Đã hạ trùm!":"Thử lại một lần nhé!"):(game.run.victory?"Khu vườn nở hoa!":"Nghỉ một chút nhé!"),Ink,brand_);
        Center(800,454,game.Practice()?FindEnemy(s.config,s.EnemyId()).name:(game.run.victory?"Bạn đã vượt qua ba hồi và Undergod.":"Hành trình dừng trước khi hạ được trùm."),Muted,font_);
        Center(800,491,game.Practice()?"Đấu thử · Không nhận thưởng hoặc thay đổi bản lưu":"Lông Vũ: "+std::to_string(game.profile.feathers)+" · Hồi "+std::to_string(game.run.act+1)+" / 3",Muted,small_);
        if(game.TurnStarRules())Center(800,516,std::to_string(game.summary.turns)+" lượt · "+std::to_string(game.summary.cards)+" lá · Gây "+std::to_string(game.summary.damage)+" HP · Chặn "+std::to_string(game.summary.blocked),Muted,small_);
        Button(ResultButton(),game.Practice()?"Chọn lớp / Đấu lại ↻":"Chọn lớp / Chơi lại ↻",true,mx,my);if(game.TurnStarRules())Center(800,605,"Seed "+std::to_string(game.summary.seed)+" · Còn "+std::to_string(game.summary.hp)+" HP · Hồi "+std::to_string(game.summary.healed)+" · Tuyệt kỹ "+std::to_string(game.summary.ultimates),Muted,small_);
        Button(RetrySeedButton(),"Cùng seed · Shift+R",true,mx,my);Button(NewSeedButton(),"Seed mới · N",true,mx,my);return;
    }
    if(game.ShowingMap()){
        backdrop();Center(800,120,s.config.acts[game.run.act].name,Ink,brand_);Center(800,158,"Hồi "+std::to_string(game.run.act+1)+" / 3 · HP "+std::to_string(game.run.hp)+"/"+std::to_string(game.run.max_hp),Muted,font_);
        Text(145,210,"HÀNH TRÌNH",Ink,heading_);Text(145,250,"Tầng "+std::to_string(game.run.node<0?1:game.run.map[game.run.node].floor+1)+" / "+std::to_string(game.run.map.back().floor+1),Muted,font_);
        Text(145,282,"Bộ bài: "+std::to_string(game.run.deck.size())+" lá",Muted,font_);Text(145,326,"DI VẬT · "+std::to_string(game.run.relics.size()),Ink,font_);
        for(int i=0;i<int(game.run.relics.size());++i){const auto& relic=FindRelic(s.config,game.run.relics[i]);const Rect r={ContentOffset+65+i%3*78,352+i/3*54,72,50};Box(r,C(239,235,221),8);const int graph=Image("relic_"+relic.id);if(graph>=0)DrawRotaGraph3F(r.x+r.w/2.f,r.y+r.h/2.f,128,128,.20,.20,0,graph,TRUE);
            if(r.Contains(mx,my)){Box({120,750,275,91},C(244,239,223));Wrapped({132,760,252,74},relic.name+": "+RelicDescription(game,relic),Ink);}
        }
        Text(145,640,"THUỐC "+std::to_string(game.run.potions.size())+" / "+std::to_string(game.run.potion_slots),Ink,font_);
        for(int i=0;i<int(game.run.potions.size());++i)Text(145,672+i*22,FindPotion(s.config,game.run.potions[i]).name,Muted,small_);
        SetDrawArea(ContentOffset+330,195,ContentOffset+1110,746);
        for(const auto& node:game.run.map)for(int next:node.next){
            const bool boss=game.run.map[next].kind==NodeKind::Boss;const Rect a=MapNodeRect(node.floor-map_scroll_,node.column,node.kind==NodeKind::Boss,(game.run.act==0?s.config.map_columns:s.config.acts[game.run.act].columns)),b=MapNodeRect(game.run.map[next].floor-map_scroll_,game.run.map[next].column,boss,(game.run.act==0?s.config.map_columns:s.config.acts[game.run.act].columns));
            DrawLine(a.x+a.w/2,a.y+a.h/2,b.x+b.w/2,b.y+b.h/2,C(186,196,176));
        }
        for(const auto& node:game.run.map){
            const bool boss=node.kind==NodeKind::Boss;const Rect r=MapNodeRect(node.floor-map_scroll_,node.column,boss,(game.run.act==0?s.config.map_columns:s.config.acts[game.run.act].columns));
            const bool open=std::find(game.run.options.begin(),game.run.options.end(),int(&node-&game.run.map[0]))!=game.run.options.end();
            const unsigned fill=node.kind==NodeKind::Elite?C(184,112,112):(node.kind==NodeKind::Boss?C(126,96,150):(node.kind==NodeKind::Shop?C(196,164,92):(node.kind==NodeKind::Rest?C(116,158,124):(node.kind==NodeKind::Event?C(120,146,176):C(206,164,132)))));
            const int current_floor=game.run.node<0?-1:game.run.map[game.run.node].floor;const bool fog=node.floor>current_floor&&node.floor<=game.run.fog_until;
            Box(r,open?(fog?C(118,135,131):fill):C(214,216,206),18);Center(r.x+r.w/2,r.y+14,fog?"?":NodeKindName(node.kind),open?C(255,252,246):Muted,small_);
            if(node.visited)Center(r.x+r.w/2,r.y+36,"xong",open?C(255,252,246):Muted,small_);
        }
        SetDrawArea(ContentOffset+330,195,ContentOffset+1110,786);DrawMapPlayer(game);
        SetDrawArea(0,0,Width,Height);
        Button(MapUpButton(),"Tầng cao hơn ↑",map_scroll_<game.run.map.back().floor-5,mx,my);Button(MapDownButton(),"Tầng thấp hơn ↓",map_scroll_>0,mx,my);
        Wrapped({1230,370,215,130},"Lăn chuột hoặc Page Up / Page Down để xem bản đồ. Chỉ nút có màu là đi được.",Muted);
        for(int i=0;i<int(game.run.options.size());++i){const auto& node=game.run.map[game.run.options[i]];Button(MapChoiceRect(i,int(game.run.options.size())),std::to_string(i+1)+" · "+(node.floor<=game.run.fog_until?"?":NodeKindName(node.kind)),true,mx,my);}
        Text(135,866,"1–9 chọn lối. F6 lưu / F9 tiếp tục.",Muted,small_);return;
    }
    if(!game.Menu()&&game.run.place==Place::Event){
        backdrop();Box({360,180,880,620},C(252,249,239),24);Center(800,220,game.run.event_title,Ink,brand_);
        Wrapped({425,285,750,150},game.run.event_text,Muted,font_);
        for(int i=0;i<int(game.run.event_choices.size());++i){
            const Rect r=EventChoiceRect(i);const bool hover=r.Contains(mx,my);
            const bool active=CanChooseEvent(game.run,i);Box(r,active?(hover?C(93,125,105):C(79,106,91)):C(215,216,205),14);
            Wrapped({r.x+20,r.y+10,r.w-40,25},std::to_string(i+1)+" · "+game.run.event_choices[i].label,active?C(255,255,246):Muted,font_);
            Wrapped({r.x+20,r.y+43,r.w-40,40},active?game.run.event_choices[i].outcome:"Không đủ vàng.",active?C(226,232,220):Muted);
        }
        return;
    }
    if(!game.Menu()&&game.run.place==Place::Shop){
        backdrop();Text(160,118,"Cửa hàng",Ink,heading_);
        for(int i=0;i<int(game.run.shop.size());++i){
            const auto& item=game.run.shop[i];
            std::string name=item.id;
            if(item.ware==Ware::Card)name=s.Definition(item.id).name;else if(item.ware==Ware::Relic)name=FindRelic(s.config,item.id).name;else name=FindPotion(s.config,item.id).name;
            const bool fits=item.ware!=Ware::Potion||int(game.run.potions.size())<game.run.potion_slots;
            Button(ShopRect(i),std::to_string(i+1)+" · "+name+"  ·  "+std::to_string(item.price)+" vàng"+(item.sold?"  ·  đã bán":(!fits?"  ·  đầy thuốc":"")),!item.sold&&game.run.gold>=item.price&&fits,mx,my);
            if(ShopRect(i).Contains(mx,my)){
                std::string description;
                if(item.ware==Ware::Card)description=s.StarCardView(s.Definition(item.id),game.TurnStarRules()).description;
                else if(item.ware==Ware::Relic)description=RelicDescription(game,FindRelic(s.config,item.id));
                else{const auto& potion=FindPotion(s.config,item.id);CardDef view;view.description=potion.description;view.effects=potion.effects;description=s.StarCardView(view,game.TurnStarRules()).description;}
                Wrapped({1080,170,330,65},name,Ink,font_);Wrapped({1080,250,330,230},description,Muted,font_);
                const int graph=Image((item.ware==Ware::Card?"card_":(item.ware==Ware::Relic?"relic_":"potion_"))+item.id);if(graph>=0)DrawRotaGraph3F(SceneX(1160),550,128,128,.55,.55,0,graph,TRUE);
            }
        }
        Text(160,628,"Gỡ một lá · "+std::to_string(s.config.remove_price)+" vàng. Bộ còn "+std::to_string(game.run.deck.size())+" lá.",Muted,small_);
        for(int i=0;i<8&&deck_page_*8+i<int(game.run.deck.size());++i)Button(DeckRect(i),s.Definition(game.run.deck[deck_page_*8+i]).name,game.run.gold>=s.config.remove_price&&int(game.run.deck.size())>5,mx,my);
        Button(DeckPrevButton(),"← Trang trước",deck_page_>0,mx,my);Button(DeckNextButton(),"Trang sau →",(deck_page_+1)*8<int(game.run.deck.size()),mx,my);
        Text(530,802,"Trang "+std::to_string(deck_page_+1)+" / "+std::to_string((game.run.deck.size()+7)/8),Muted,small_);
        Button(LeaveButton(),"Rời cửa hàng",true,mx,my);return;
    }
    if(!game.Menu()&&game.run.place==Place::Rest){
        const int heal=RestHealAmount(game.run,s.config);
        backdrop();Text(200,120,"Chỗ nghỉ",Ink,heading_);
        Button(HealButton(),"H · Nghỉ: hồi "+std::to_string(heal)+" HP",!game.run.rest_used,mx,my);
        Text(200,270,"Hoặc bấm 1–8 để cường hóa một lá trên trang. Chỉ chọn một việc.",Muted,font_);
        for(int i=0;i<8&&deck_page_*8+i<int(game.run.deck.size());++i){
            const auto& card=s.Definition(game.run.deck[deck_page_*8+i]);
            Button(UpgradeRect(i),std::to_string(i+1)+" · "+card.name,!card.upgraded&&!card.upgrade_id.empty(),mx,my);
            if(UpgradeRect(i).Contains(mx,my)&&!card.upgrade_id.empty())Wrapped({200,570,1100,120},s.Definition(card.upgrade_id).name+": "+s.StarCardView(s.Definition(card.upgrade_id),game.TurnStarRules()).description,Ink,font_);
        }
        Button(DeckPrevButton(),"← Trang trước",deck_page_>0,mx,my);Button(DeckNextButton(),"Trang sau →",(deck_page_+1)*8<int(game.run.deck.size()),mx,my);
        Text(530,802,"Trang "+std::to_string(deck_page_+1)+" / "+std::to_string((game.run.deck.size()+7)/8),Muted,small_);
        return;
    }
    backdrop();
    std::string progress="Hành trình";
    if(!game.run.map.empty()){
        const int node=std::clamp(game.run.index,0,int(game.run.map.size())-1);
        progress=game.Practice()?"ĐẤU THỬ TRÙM":"Hồi "+std::to_string(game.run.act+1)+" · Tầng "+std::to_string(game.run.map[node].floor+1)+"/"+std::to_string(game.run.map.back().floor+1);
        if(s.PhaseCount()>1)progress+=" · Giai đoạn "+std::to_string(s.PhaseIndex()+1)+"/"+std::to_string(s.PhaseCount());
    }
    if(!game.Practice())Text(840,87,progress,Muted,small_);
    if(game.Practice())Button(DifficultyButton(),std::string("D · Đấu thử: ")+PracticeDifficultyName(game.PracticeLevel())+" · đổi và thử lại",!game.Busy(),mx,my);
    DrawBox(20,110,Width-20,250,C(227,240,237),TRUE);
    auto health=[&](bool enemy){const auto& actor=enemy?s.enemy:s.player;int x=ContentOffset+(enemy?1100:80);
        Text(x,123,enemy?"NGƯỜI GIỮ KHU VƯỜN":"NGƯỜI LỮ HÀNH",Muted,small_);
        Text(x,148,enemy?s.PhaseName():FindClass(s.config,game.run.class_id).name,Ink,heading_);
        const int ratio=actor.max_hp>0?std::max(0,260*actor.hp/actor.max_hp):0;
        Box({x,185,260,22},C(203,214,200),8);if(ratio>0)Box({x,185,std::max(1,ratio),22},enemy?C(157,187,131):C(237,159,166),8);
        Center(x+130,187,std::to_string(actor.hp)+" / "+std::to_string(actor.max_hp)+" HP",Ink,small_);
        Wrapped({x,212,280,40},s.StatusText(actor),Muted,small_);
    };health(false);health(true);
    Box({160,245,260,5},C(209,215,210),2);if(s.rage>0)Box({160,245,260*s.rage/s.config.balance.rage_max,5},C(173,146,212),2);
    Text(440,232,"Nộ "+std::to_string(s.rage)+" / "+std::to_string(s.config.balance.rage_max),C(123,91,153),small_);
    const std::string ultimate=s.Mods().ultimate=="storm"?"Bão Lông":(s.Mods().ultimate=="shadow"?"Ảnh Phân":"Sao Rơi");
    Center(800,131,"LƯỢT "+std::to_string(s.turn),Muted,small_);Center(800,162,game.Cinematic()?ultimate:game.TurnStatus(),Ink,font_);
    if(game.CanAct())Center(800,198,"Dự kiến mất "+std::to_string(game.IncomingLoss())+" HP theo hàng bài hiện tại",Muted,small_);
    World(game);
    DrawTargets(game,mx,my);
    // Intent remains visible above the opponent, outside the zoomed arena.
    if(!game.Cinematic()&&!game.TurnStarRules()){Box({1040,252,420,36},C(255,249,237),9);Center(1250,261,s.IntentText(),Ink,small_);}
    if(game.TurnStarRules())DrawStarBattle(game,mx,my);
    else{
    DrawLine(20,610,Width-20,610,C(219,219,202));
    DrawCircle(ContentOffset+90,659,25,C(233,221,244),TRUE);Center(170,647,std::to_string(s.energy)+"/"+std::to_string(s.config.balance.energy+s.Mods().energy),C(123,91,153),font_);
    Text(208,643,"NĂNG LƯỢNG",Muted,small_);    Text(360,639,"Rút: "+std::to_string(s.draw.size()),Muted,small_);Text(360,663,"Bỏ: "+std::to_string(s.discard.size())+"   Bộ: "+std::to_string(game.run.deck.size()),Muted,small_);
    Text(550,652,"1–9: bài · E: hết lượt · U: "+ultimate,Muted,small_);
    Button(UltimateButton(),"✦ "+ultimate+" · "+std::to_string(s.UltimateCost()),game.CanAct()&&s.CanUltimate(),mx,my);Button(EndButton(),"Kết thúc lượt →",game.CanAct(),mx,my);
    for(size_t i=0;i<s.hand.size();++i)Card(s.Definition(s.hand[i]),CardRect(int(s.hand.size()),int(i)),int(i),s.CanPlay(int(i)),!game.CanAct(),mx,my);
    if(s.live.locked>=0&&s.live.locked<int(s.hand.size())){const auto r=CardRect(int(s.hand.size()),s.live.locked);Box({r.x+20,r.y+90,r.w-40,26},C(154,91,112));Center(r.x+r.w/2,r.y+96,"BỊ KHÓA",C(255,255,246),small_);}
    if(s.Mods().mana)Text(208,670,"Mana: "+std::to_string(s.live.mana)+" / "+std::to_string(s.Mods().max_mana),C(123,91,153),small_);
    }
    Text(135,866,"Tab: đổi mục tiêu · Z/X/C/V: thuốc · F10: âm thanh",Muted,small_);if(!game.ResolvingStars())Text(1170,866,"BA HỒI · 3 LỚP · 50 LÁ",Muted,small_);
    if(game.toast_age>0){Box({500,570,600,43},C(55,72,62));Center(800,581,game.toast,C(255,255,246),small_);}
    if(game.Cinematic())Button(SkipButton(),"Bỏ qua · Space / Esc",true,mx,my);
    const char* potion_keys[]={"Z","X","C","V"};
    for(int i=0;i<int(game.run.potions.size())&&i<4;++i)Button(PotionRect(i),std::string(potion_keys[i])+" · "+FindPotion(s.config,game.run.potions[i]).name,game.CanAct(),mx,my);
    if(game.Reward()){
        Blend(210);DrawBox(0,0,Width,Height,C(38,57,44),TRUE);Blend(255);
        Box({340,205,920,470},C(251,249,243),24);
        Center(800,250,"Chiến lợi phẩm",Ink,brand_);
        Center(800,300,"Chọn 1 trong 3 lá. HP "+std::to_string(game.run.hp)+"/"+std::to_string(game.run.max_hp)+" được giữ sang bản đồ.",Muted,font_);
        if(game.TurnStarRules())Center(800,640,std::to_string(game.summary.turns)+" lượt / "+std::to_string(game.summary.cards)+" lá / HP -"+std::to_string(game.summary.damage)+" / Chặn "+std::to_string(game.summary.blocked)+" / Hồi +"+std::to_string(game.summary.healed)+" / Seed "+std::to_string(game.summary.seed),Muted,small_);
        for(int i=0;i<int(game.run.rewards.size());++i){
            const auto card=s.StarCardView(s.Definition(game.run.rewards[i]),game.TurnStarRules());
            Card(card,RewardRect(i),i,true,false,mx,my);
            Center(RewardRect(i).x+RewardRect(i).w/2,RewardRect(i).y-22,RarityLabel(card.rarity),C(123,91,153),small_);
        }
    }
}
void Ui::Click(Game& game,int mx,int my){
    if(FeedbackButton().Contains(mx,my)){game.ToggleFeedback();return;}
    if(game.pile_view){if(PileCloseButton().Contains(mx,my))game.pile_view=0;else if(PilePrevButton().Contains(mx,my))game.pile_page=std::max(0,game.pile_page-1);else if(PileNextButton().Contains(mx,my)){const int pages=std::max(1,(int(Game::PileCounts(game.pile_view==1?game.state.draw:game.state.discard).size())+13)/14);game.pile_page=std::min(pages-1,game.pile_page+1);}return;}
    if(game.Practice()&&DifficultyButton().Contains(mx,my)){game.CyclePracticeDifficulty();return;}
    if(game.CanAct()&&DrawPileButton().Contains(mx,my)){game.TogglePile(1);return;}
    if(game.CanAct()&&DiscardPileButton().Contains(mx,my)){game.TogglePile(2);return;}
    if(AudioButton().Contains(mx,my)){game.ToggleAudio();return;}
    if(game.Menu()){
        if(BackButton().Contains(mx,my)){game.BackMenu();return;}
        if(game.MenuPage()==1){
            for(int i=0;i<int(game.state.config.classes.size());++i)if(ClassRect(i).Contains(mx,my)){game.SelectClass(i);return;}
            if(StartButton().Contains(mx,my)){game.BeginSelected(unsigned(GetNowHiPerformanceCount()));ResetView();return;}
            if(SkillsButton().Contains(mx,my)){game.OpenSkills();return;}
            if(AchievementsButton().Contains(mx,my)){game.OpenAchievements();return;}
            if(ChallengeMinus().Contains(mx,my)){game.SetChallenge(game.SelectedChallenge()-1);return;}
            if(ChallengePlus().Contains(mx,my)){game.SetChallenge(game.SelectedChallenge()+1);return;}
            if(PracticeButton().Contains(mx,my)){game.PracticeSelected(unsigned(GetNowHiPerformanceCount()));ResetView();return;}
        }else if(game.MenuPage()==2){
            std::vector<std::string> branches;for(const auto& s:game.state.config.skills)if(s.class_id==game.profile.selected_class&&std::find(branches.begin(),branches.end(),s.branch)==branches.end())branches.push_back(s.branch);
            for(int b=0;b<int(branches.size());++b)for(const auto& s:game.state.config.skills)if(s.class_id==game.profile.selected_class&&s.branch==branches[b]&&SkillRect(b,s.tier-1).Contains(mx,my)){game.BuySkill(s.id);return;}
        }
        return;
    }
    if(game.RelicReward()){
        for(int i=0;i<int(game.run.relic_rewards.size());++i){auto r=RewardRect(i);r.h=290;if(r.Contains(mx,my)){game.ChooseRelic(i);return;}}
        if(BackButton().Contains(mx,my))game.OpenClasses();return;
    }
    if(game.CardSelection()||game.RelicSelection()){
        if(BackButton().Contains(mx,my)){game.CancelSelection();return;}
        if(DeckPrevButton().Contains(mx,my)){Page(game,-1);return;}if(DeckNextButton().Contains(mx,my)){Page(game,1);return;}
        for(int i=0;i<8;++i)if(UpgradeRect(i).Contains(mx,my)){if(game.CardSelection())ChooseSelection(game,i);else ChooseTrade(game,i);return;}
        return;
    }
    for(int speed=1;speed<=3;++speed)if(SpeedButton(speed).Contains(mx,my)){game.SetSpeed(speed);return;}
    if(game.Cinematic()){if(SkipButton().Contains(mx,my))game.SkipCinematic();return;}
    if(SaveButton().Contains(mx,my)){game.Save();return;}if(LoadButton().Contains(mx,my)){game.Load();map_node_=-999;screen_node_=-999;return;}
    if(game.Reward()){
        if(RestartButton().Contains(mx,my)){game.OpenClasses();ResetView();return;}
        for(int i=0;i<int(game.run.rewards.size());++i)if(RewardRect(i).Contains(mx,my)){game.ChooseReward(i);return;}
        return;
    }
    if(game.Result()){
        if(RetrySeedButton().Contains(mx,my)){game.Retry();ResetView();return;}
        if(NewSeedButton().Contains(mx,my)){game.NewMatch();ResetView();return;}
        if(ResultButton().Contains(mx,my)||RestartButton().Contains(mx,my)){game.OpenClasses();ResetView();}return;
    }
    if(RestartButton().Contains(mx,my)&&!game.Busy()){game.OpenClasses();ResetView();return;}
    if(game.Map()){
        if(MapUpButton().Contains(mx,my)){Page(game,1);return;}if(MapDownButton().Contains(mx,my)){Page(game,-1);return;}
        for(int i=0;i<int(game.run.options.size());++i)if(MapChoiceRect(i,int(game.run.options.size())).Contains(mx,my)){game.ChooseNode(i);return;}
        if(my<195||my>=746||mx<ContentOffset+330||mx>=ContentOffset+1110)return;
        for(int i=0;i<int(game.run.map.size());++i){const auto& node=game.run.map[i];if(node.floor-map_scroll_<0||node.floor-map_scroll_>5||!MapNodeRect(node.floor-map_scroll_,node.column,node.kind==NodeKind::Boss,(game.run.act==0?game.state.config.map_columns:game.state.config.acts[game.run.act].columns)).Contains(mx,my))continue;for(int option=0;option<int(game.run.options.size());++option)if(game.run.options[option]==i){game.ChooseNode(option);return;}}
        return;
    }
    if(game.Event()){for(int i=0;i<int(game.run.event_choices.size());++i)if(EventChoiceRect(i).Contains(mx,my)){game.ChooseEvent(i);return;}return;}
    if(game.Shop()){
        if(DeckPrevButton().Contains(mx,my)){Page(game,-1);return;}if(DeckNextButton().Contains(mx,my)){Page(game,1);return;}
        if(LeaveButton().Contains(mx,my)){game.LeaveShop();return;}
        for(int i=0;i<int(game.run.shop.size());++i)if(ShopRect(i).Contains(mx,my)){game.Buy(i);return;}
        for(int i=0;i<8&&deck_page_*8+i<int(game.run.deck.size());++i)if(DeckRect(i).Contains(mx,my)){game.RemoveCard(deck_page_*8+i);return;}
        return;
    }
    if(game.Rest()){
        if(DeckPrevButton().Contains(mx,my)){Page(game,-1);return;}if(DeckNextButton().Contains(mx,my)){Page(game,1);return;}
        if(HealButton().Contains(mx,my)){game.RestHeal();return;}
        for(int i=0;i<8&&deck_page_*8+i<int(game.run.deck.size());++i)if(UpgradeRect(i).Contains(mx,my)){game.UpgradeCard(deck_page_*8+i);return;}
        return;
    }
    if(game.Busy())return;
    if(game.TurnStarRules()){
        for(int slot=0;slot<3;++slot)if(StarSlotRect(slot).Contains(mx,my)){game.UnqueueCard(slot);return;}
        if(WaitButton().Contains(mx,my)){game.QueueWait();return;}
    }
    if(game.CanAct()&&my>=300&&my<=610){int target=-1;float distance=100.f;for(int index:game.state.Targets()){const float delta=std::abs(mx-SceneX(game.TargetX(index)));if(delta<distance){distance=delta;target=index;}}if(target>=0){game.SelectTarget(target);return;}}
    if(EndButton().Contains(mx,my)){game.EndTurn();return;}
    if(UltimateButton().Contains(mx,my)){game.Ultimate();return;}
    for(int i=0;i<int(game.run.potions.size())&&i<4;++i)if(PotionRect(i).Contains(mx,my)){game.DrinkPotion(i);return;}
    if(game.TurnStarRules()){for(auto it=game.cards.rbegin();it!=game.cards.rend();++it)if(it->zone==CardZone::Hand&&!it->leaving){const auto b=it->Bounds();if(Rect{int(b.x),int(b.y),int(b.w),int(b.h)}.Contains(mx,my)){game.Play(it->hand_index);return;}}return;}
    const auto hand=HandIndices(game);const int count=int(hand.size());for(int i=0;i<count;++i)if(CardRect(count,i).Contains(mx,my)){game.Play(hand[i]);return;}
}
}
