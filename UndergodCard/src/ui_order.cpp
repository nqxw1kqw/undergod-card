#include "ui.hpp"
#include <DxLib.h>
#include <cmath>

namespace undergod {
namespace { unsigned Color(int r,int g,int b){return GetColor(r,g,b);} const unsigned Ink=0x343948,Muted=0x7c887c; }
std::string Ui::RelicDescription(const Game& game,const RelicDef& relic)const{
    if(!game.TurnStarRules())return relic.description;
    if(relic.energy!=0)return "Đầu mỗi vòng có thêm "+std::to_string(relic.energy*3+relic.draw)+" lá lựa chọn.";
    int drawn=0;for(const auto& e:relic.turn_start)if(e.op==EffectOp::Energy)drawn+=e.amount*std::max(1,e.times);
    return drawn>0?"Đầu mỗi vòng rút thêm "+std::to_string(drawn)+" lá.":relic.description;
}
std::vector<int> Ui::HandIndices(const Game& game)const{
    std::vector<int> result;
    for(int i=0;i<int(game.state.hand.size());++i)if(!game.TurnStarRules()||!game.state.IsQueuedStar(i))result.push_back(i);
    return result;
}
void Ui::ChooseHand(Game& game,int index){const auto hand=HandIndices(game);if(index>=0&&index<int(hand.size()))game.Play(hand[index]);}
void Ui::PlanCard(const CardDef& card,Rect r,int number,bool filled,bool spent,int mx,int my)const{
    const auto border=spent?Color(207,211,199):(filled?Color(129,154,119):Color(214,216,206));
    Box({r.x,r.y+3,r.w,r.h},Color(222,221,207),9);Box(r,border,9);Box({r.x+1,r.y+1,r.w-2,r.h-2},filled?Color(251,249,237):Color(239,239,229),8);
    if(!filled){Center(r.x+r.w/2,r.y+18,std::to_string(number),Muted,font_);Center(r.x+r.w/2,r.y+43,"Chọn một lá",Muted,small_);return;}
    const auto key="card_"+(card.upgraded?card.id.substr(0,card.id.size()-1):card.id);const int graph=Image(key);
    if(spent)SetDrawBlendMode(DX_BLENDMODE_ALPHA,100);
    if(graph>=0)DrawRotaGraph3F(r.x+29.f,r.y+r.h/2.f,128,128,.20,.20,0,graph,TRUE,FALSE);
    else Center(r.x+29,r.y+r.h/2-10,"✦",Muted,font_);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND,255);
    Wrapped({r.x+55,r.y+8,r.w-61,r.h-32},card.name,spent?Muted:Ink);
    Text(r.x+56,r.y+r.h-24,spent?"✓ Đã dùng":std::to_string(number)+" · 1 sao",Color(125,97,151),small_);
    if(r.Contains(mx,my)&&!spent)DrawRoundRect(r.x,r.y,r.x+r.w,r.y+r.h,9,9,Color(154,125,80),FALSE);
}
void Ui::DrawStarBattle(const Game& game,int mx,int my){
    const auto& state=game.state;
    DrawLine(20,610,Width-20,610,Color(219,219,202));
    Text(145,622,"TURN STAR",Muted,small_);
    for(int i=0;i<3;++i){const bool filled=i<int(state.stars.player.size());DrawCircle(ContentOffset+83+i*52,660,20,filled?Color(175,146,202):Color(229,221,238),TRUE);Center(ContentOffset+83+i*52,648,"★",filled?Color(255,253,245):Color(151,126,171),font_);}
    Button(WaitButton(),"Chờ 1 sao",game.CanAct()&&state.stars.player.size()<3,mx,my);
    Text(355,638,"Rút "+std::to_string(state.draw.size()),Muted,small_);Text(355,661,"Bỏ "+std::to_string(state.discard.size()),Muted,small_);Text(355,684,"Bộ "+std::to_string(game.run.deck.size()),Muted,small_);
    for(int slot=0;slot<3;++slot){
        const bool filled=slot<int(state.stars.player.size());CardDef card;
        if(filled){const auto& q=state.stars.player[slot];if(q.id.empty()){card.name="Chờ";card.description="Không thực hiện hành động.";}else card=state.StarCardView(state.Definition(q.id));}
        PlanCard(card,StarSlotRect(slot),slot+1,filled,state.stars.cursor>=slot*2+1,mx,my);
    }
    const std::string ultimate=state.Mods().ultimate=="storm"?"Bão Lông":(state.Mods().ultimate=="shadow"?"Ảnh Phân":"Sao Rơi");
    if(!game.ResolvingStars())Button(UltimateButton(),"✦ "+ultimate,game.CanAct()&&state.CanUltimate(),mx,my);
    if(!game.ResolvingStars())Button(EndButton(),"Attack →",game.CanAttack(),mx,my);
    if(!game.ResolvingStars())Text(1120,692,"Ta 1 → Địch 1 → Ta 2 → Địch 2…",Muted,small_);
    Text(550,612,game.CanAct()?"Chọn đủ ba lá · Bấm lá trong khung để gỡ · E: Attack":"Bài được thực hiện theo đúng thứ tự đã xếp.",Muted,small_);
    const auto visible=HandIndices(game);const float dealt=game.DealtCards();
    for(int display=0;display<int(visible.size());++display){
        if(game.ResolvingStars())break;
        const int index=visible[display];
        if(game.ArrivalStage()==Arrival::Fight||game.ArrivalStage()==Arrival::FadeIn)continue;
        if(game.ArrivalStage()==Arrival::Deal&&display>=int(std::ceil(dealt)))continue;
        auto r=StarHandRect(int(visible.size()),display);
        if(game.ArrivalStage()==Arrival::Deal){const float t=std::clamp(dealt-display,0.f,1.f);const float f=1-(1-t)*(1-t);r.x=int(SceneX(280)+(r.x-SceneX(280))*f);r.y=int(650+(r.y-650)*f);}
        Card(state.StarCardView(state.Definition(state.hand[index])),r,display,state.CanQueueStar(index),!game.CanAct(),mx,my);
        if(index==state.live.locked){Box({r.x+16,r.y+50,r.w-32,25},Color(154,91,112));Center(r.x+r.w/2,r.y+54,"BỊ KHÓA",Color(255,255,246),small_);}
    }
    if(state.stars.enabled&&!game.Cinematic()){
        const bool boss=!state.EnemyStarView(0).class_id.empty()&&state.EnemyStarView(0).rarity=="boss";
        Text(828,268,boss?"BỘ TRÙM · 3 SAO":"Ý ĐỊCH · 3 SAO",Muted,small_);
        for(int slot=0;slot<int(state.stars.enemy.size());++slot)PlanCard(state.EnemyStarView(slot),EnemyStarRect(slot),slot+1,true,state.stars.cursor>=slot*2+2,mx,my);
    }
    if(game.ResolvingStars()){
        Center(1310,622,(game.ActiveActor()<0?"BÀI CỦA BẠN · ":"BÀI CỦA ĐỊCH · ")+std::to_string(game.ActiveSlot()+1)+" / 3",game.ActiveActor()<0?Color(79,113,88):Color(165,100,84),font_);
        Card(game.ActiveCard(),{1150,650,320,218},-1,true,false,-1,-1);
    }
    for(int slot=0;slot<int(state.stars.enemy.size());++slot)if(EnemyStarRect(slot).Contains(mx,my)){
        const auto card=state.EnemyStarView(slot);const int actor=state.stars.enemy[slot].actor;
        Box({1035,337,425,185},Color(255,252,240),14);
        Wrapped({1055,353,385,42},state.TargetName(actor)+" · "+card.name,Ink,font_);
        Wrapped({1055,407,385,95},card.description,Ink,font_);return;
    }
    if(game.CanAct()){
        for(int display=0;display<int(visible.size());++display)if(StarHandRect(int(visible.size()),display).Contains(mx,my)){
            const auto card=state.StarCardView(state.Definition(state.hand[visible[display]]));const int x=std::clamp(StarHandRect(int(visible.size()),display).x,30,Width-405);
            Box({x,461,380,139},Color(255,252,246));Wrapped({x+15,475,350,115},card.name+": "+card.description,Ink);break;
        }
    }
}
std::pair<float,float> Ui::MapPlayerPosition(const Game& game)const{
    if(game.run.map.empty())return {Width/2.f,752.f};
    const int columns=game.run.act==0?game.state.config.map_columns:game.state.config.acts[game.run.act].columns;
    auto point=[&](int index){
        if(index<0)return std::pair<float,float>{Width/2.f,752.f};
        const auto& node=game.run.map.at(index);const auto r=MapNodeRect(node.floor-map_scroll_,node.column,node.kind==NodeKind::Boss,columns);
        return std::pair<float,float>{float(r.x+r.w/2),float(r.y+r.h/2)};
    };
    const auto from=point(game.MapAvatarFrom()),to=point(game.MapAvatarTo());
    const float progress=game.MapAvatarProgress(),f=progress*progress*(3-2*progress);
    return {from.first+(to.first-from.first)*f,from.second+(to.second-from.second)*f};
}
void Ui::DrawMapPlayer(const Game& game)const{
    if(!game.TurnStarRules()||game.run.map.empty())return;
    const auto position=MapPlayerPosition(game);const float x=position.first,y=position.second,progress=game.MapAvatarProgress();
    SetDrawBlendMode(DX_BLENDMODE_ALPHA,50);DrawOval(int(x),int(y+15),32,11,Color(56,86,65),TRUE);SetDrawBlendMode(DX_BLENDMODE_NOBLEND,255);
    DrawCircle(int(x),int(y+2),30,Color(252,247,224),TRUE);DrawCircle(int(x),int(y+2),31,Color(115,145,107),FALSE);
    const auto sprite=FindClass(game.state.config,game.run.class_id).sprite;
    int graph=Image(sprite+"_idle_"+std::to_string(int(game.time*6)%4));if(graph<0)graph=Image(sprite+"_idle_0");
    if(graph>=0)DrawRotaGraph3F(x,y+24-std::sin(progress*3.14159265f)*7,256,456,.14,.14,0,graph,TRUE,FALSE);
}
void Ui::DrawTransitions(const Game& game)const{
    if(!game.TurnStarRules())return;
    if(game.ArrivalStage()==Arrival::Fight){
        const int graph=Image("fight");
        if(graph>=0){int w=0,h=0;GetGraphSize(graph,&w,&h);const float scale=760.f/std::max(1,w);DrawRotaGraph3F(SceneX(game.FightBannerX()),420,w/2.f,h/2.f,scale,scale,0,graph,TRUE,FALSE);}
    }
    const int alpha=int(game.FadeOpacity()*255);
    if(alpha>0){SetDrawBlendMode(DX_BLENDMODE_ALPHA,alpha);DrawBox(0,0,Width,Height,Color(0,0,0),TRUE);SetDrawBlendMode(DX_BLENDMODE_NOBLEND,255);}
}
}
