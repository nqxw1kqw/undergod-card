#include "ui.hpp"
#include <DxLib.h>
#include <cmath>

namespace undergod {
namespace { const unsigned Ink=0x343948,Muted=0x7c887c; }
void Ui::DrawCardVisuals(const Game& game,int mx,int my)const{
    for(int layer=0;layer<2;++layer)for(const auto& v:game.cards){
        if((v.zone==CardZone::Hand)!=(layer==0))continue;
        if(v.zone==CardZone::Hand&&(game.ResolvingStars()||game.ArrivalStage()==Arrival::Fight||game.ArrivalStage()==Arrival::FadeIn||game.ArrivalStage()==Arrival::BlackHold))continue;
        const auto box=v.Bounds();const Rect r={int(box.x),int(box.y),std::max(35,int(box.w)),std::max(28,int(box.h))};
        if(v.leaving)SetDrawBlendMode(DX_BLENDMODE_ALPHA,int(255*std::clamp(1-v.age/std::max(.01f,v.duration),0.f,1.f)));
        if(r.h<125){
            if(r.w>=120&&r.h>=65)PlanCard(v.snapshot,r,std::max(0,v.queue_slot)+1,true,false,-1,-1);
            else{Box(r,GetColor(237,230,210),7);const auto id=v.snapshot.upgraded?v.card.substr(0,v.card.size()-1):v.card;const int graph=Image("card_"+id);if(graph>=0){const float scale=std::min(r.w,r.h)*.8f/256;DrawRotaGraph3F(r.x+r.w/2.f,r.y+r.h/2.f,128,128,scale,scale,0,graph,TRUE);}}
        }else{
            int index=-1;if(v.zone==CardZone::Hand){const auto hand=HandIndices(game);const auto at=std::find(hand.begin(),hand.end(),v.hand_index);if(at!=hand.end())index=int(at-hand.begin());}
            Card(v.snapshot,r,index,v.hand_index<0||game.state.CanQueueStar(v.hand_index),!game.CanAct()||v.zone!=CardZone::Hand,-1,-1);
            if(v.zone==CardZone::Hand&&v.hand_index==game.focused_card)DrawRoundRect(r.x-2,r.y-2,r.x+r.w+2,r.y+r.h+2,10,10,GetColor(154,125,80),FALSE);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND,255);
    }
}
void Ui::DrawPresentation(const Game& game,int mx,int my)const{
    if(game.Menu()||game.run.place!=Place::Fight)return;
    if(game.terminal.active&&!game.terminal.settled&&!game.Cinematic()){
        const float alpha=game.ResultAlpha();if(alpha<=0)return;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA,int(alpha*205));DrawBox(20,250,Width-20,610,GetColor(39,47,49),TRUE);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA,int(alpha*255));
        Center(Width/2,388,game.state.phase==Phase::Won?"CHIẾN THẮNG":"TRẬN ĐẤU KẾT THÚC",GetColor(255,246,218),brand_);
        const auto& s=game.summary;
        Center(Width/2,438,std::to_string(s.turns)+" lượt · "+std::to_string(s.cards)+" lá · Gây "+std::to_string(s.damage)+" HP · Chặn "+std::to_string(s.blocked),GetColor(255,250,234),font_);
        Center(Width/2,474,"Hồi "+std::to_string(s.healed)+" HP · Seed "+std::to_string(s.seed),GetColor(228,230,218),small_);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND,255);return;
    }
    if(!game.CanAct())return;
    if(game.pile_view){
        const auto counts=Game::PileCounts(game.pile_view==1?game.state.draw:game.state.discard);
        Box({450,290,700,326},GetColor(255,252,240),16);
        Text(475,307,game.pile_view==1?"CHỒNG RÚT · Không theo thứ tự":"CHỒNG BỎ · Không theo thứ tự",Ink,heading_);
        if(counts.empty())Text(475,355,"Chồng bài đang trống.",Muted,font_);
        const int pages=std::max(1,(int(counts.size())+13)/14),page=std::clamp(game.pile_page,0,pages-1);int i=0;
        for(const auto& entry:counts){const int item=i++;if(item/14!=page)continue;const int at=item%14;const auto& card=game.state.Definition(entry.first);
            Wrapped({475+(at/7)*330,350+(at%7)*30,310,26},card.name+" ×"+std::to_string(entry.second),Ink,font_);
        }
        Button(PilePrevButton(),"← Trang trước",page>0,mx,my);Button(PileNextButton(),"Trang sau →",page+1<pages,mx,my);Button(PileCloseButton(),"Đóng · Esc",true,mx,my);return;
    }
    int detail=game.focused_card;
    for(const auto& v:game.cards)if(v.zone==CardZone::Hand&&!v.leaving){const auto b=v.Bounds();if(Rect{int(b.x),int(b.y),int(b.w),int(b.h)}.Contains(mx,my))detail=v.hand_index;}
    if(detail<0||detail>=int(game.state.hand.size()))return;
    const auto& card=game.state.Definition(game.state.hand[detail]);const auto p=game.Preview(detail);
    Box({515,330,500,263},GetColor(255,252,240),16);
    Wrapped({535,345,460,49},card.name,Ink,heading_);
    Wrapped({535,398,460,76},game.state.StarCardView(card,game.TurnStarRules()).description,Ink,font_);
    Text(535,477,"Nếu dùng ngay · "+std::to_string(p.cost)+(game.TurnStarRules()?" sao":" năng lượng"),Muted,small_);
    Text(535,499,"Gây "+std::to_string(p.damage)+" HP · Bị giáp chặn "+std::to_string(p.absorbed)+" · Tự mất "+std::to_string(p.self_damage),Ink,small_);
    Text(535,521,"Giáp +"+std::to_string(p.block)+" · Hồi thực +"+std::to_string(p.heal)+" · Nộ "+(p.rage>=0?"+":"")+std::to_string(p.rage),Ink,small_);
    Wrapped({535,550,460,28},p.unavailable.empty()?"←/→ xem lá · Space xếp vào hàng sao":p.unavailable,GetColor(142,104,99),small_);
}
}
