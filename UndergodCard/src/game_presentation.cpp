#include "game.hpp"
#include <cmath>
#include <set>

namespace undergod {
namespace {
CardBox DrawBox(){return {345,630,80,34};}
CardBox DiscardBox(){return {345,659,80,34};}
void Move(CardVisual& c,CardBox to,float duration=.2f,float delay=0){c.from=c.Bounds();c.to=to;c.age=0;c.delay=delay;c.duration=duration;}
}
void Game::ResetPresentation(){
    cards.clear();particles.clear();summary={};summary.seed=run.seed;terminal={};next_visual_id_=1;
    focused_card=-1;pile_view=pile_page=0;visual_rng_.Seed(run.seed^0xBADC0DEu^uint32_t(run.node+1));
}
void Game::SyncCards(bool deal){
    if(!turn_stars_)return;
    std::vector<int> visible;for(int i=0;i<int(state.hand.size());++i)if(!state.IsQueuedStar(i))visible.push_back(i);
    for(int i=0;i<int(state.hand.size());++i){
        auto found=std::find_if(cards.begin(),cards.end(),[&](const CardVisual& c){return !c.leaving&&!c.enemy&&c.hand_index==i;});
        int queue=-1;for(int q=0;q<int(state.stars.player.size());++q)if(state.stars.player[q].hand_index==i)queue=q;
        if(state.stars.resolving&&queue<0){
            // Drawn cards are held for the next hand; do not expose them mid-combo.
            if(found==cards.end())continue;
        }
        const auto it=std::find(visible.begin(),visible.end(),i);const int slot=int(it-visible.begin());
        const CardBox box=queue>=0?QueueBox(queue):HandBox(int(visible.size()),slot);
        if(found==cards.end()){
            CardVisual c;c.id=next_visual_id_++;c.card=state.hand[i];c.hand_index=i;c.queue_slot=queue;
            c.snapshot=state.StarCardView(state.Definition(c.card));c.from=DrawBox();c.to=box;c.zone=queue>=0?CardZone::Queue:CardZone::Hand;
            c.duration=deal?.24f:.22f;c.delay=deal?.08f+.12f*slot:0;cards.push_back(c);
        }else{
            auto& c=*found;c.card=state.hand[i];c.snapshot=state.StarCardView(state.Definition(c.card));
            if(c.zone==CardZone::Play)continue;
            const auto zone=queue>=0?CardZone::Queue:CardZone::Hand;
            if(deal||c.zone!=zone||c.to.x!=box.x||c.to.y!=box.y||c.to.w!=box.w){
                if(deal&&c.zone!=CardZone::Queue)c.from=DrawBox();else c.from=c.Bounds();
                c.to=box;c.age=0;c.duration=deal?.24f:.20f;c.delay=deal?.08f+.12f*slot:0;
            }
            c.zone=zone;c.queue_slot=queue;
        }
    }
    for(auto& c:cards)if(!c.leaving&&!c.enemy&&c.hand_index>=int(state.hand.size())){Move(c,DiscardBox());c.leaving=true;c.zone=CardZone::Discard;c.hand_index=-1;}
}
void Game::PrepareStarCards(){
    if(!turn_stars_)return;
    std::map<int,int> selected;int next=0;for(const auto& q:state.stars.player)if(q.hand_index>=0)selected[q.hand_index]=next++;
    for(auto& c:cards)if(!c.leaving&&!c.enemy){
        const auto at=selected.find(c.hand_index);
        if(at==selected.end()){Move(c,DiscardBox(),.24f);c.zone=CardZone::Discard;c.leaving=true;c.hand_index=-1;}
        else c.hand_index=at->second;
    }
    focused_card=-1;pile_view=0;
}
void Game::StartCardVisual(bool enemy,int slot){
    if(!turn_stars_||active_card_.id.empty()||skill_visual_.animation.id=="wait")return;
    if(enemy){
        CardVisual c;c.id=next_visual_id_++;c.card=active_card_.id;c.snapshot=active_card_;c.enemy=true;c.zone=CardZone::Play;
        c.from={1035.f+slot*139,235,130,90};c.to=PlayBox();c.duration=std::min(.22f,Timing().contact*.7f);cards.push_back(c);
    }else{
        const auto& q=state.stars.player.at(slot);
        for(auto& c:cards)if(!c.enemy&&!c.leaving&&c.hand_index==q.hand_index){c.snapshot=active_card_;Move(c,PlayBox(),std::min(.22f,Timing().contact*.7f));c.zone=CardZone::Play;break;}
    }
}
void Game::ResolveCardVisual(int hand_index,float late){
    if(!turn_stars_)return;
    for(auto& c:cards){
        if(c.zone==CardZone::Play&&!c.leaving){
            const auto& def=active_card_;
            const bool exhaust=!c.enemy&&(def.exhaust||def.power);
            Move(c,c.enemy?CardBox{1390,235,60,50}:(exhaust?CardBox{460,659,50,34}:DiscardBox()),.25f);
            c.age=late;c.leaving=true;c.zone=exhaust?CardZone::Exhaust:CardZone::Discard;c.hand_index=-1;
        }else if(!c.enemy&&!c.leaving&&hand_index>=0&&c.hand_index>hand_index)--c.hand_index;
    }
    SyncCards();
}
void Game::TickPresentation(float dt){
    for(auto& c:cards)c.age+=dt;
    cards.erase(std::remove_if(cards.begin(),cards.end(),[](const CardVisual& c){return c.leaving&&c.age>=c.duration+c.delay;}),cards.end());
    for(auto& p:particles)p.age+=dt;
    particles.erase(std::remove_if(particles.begin(),particles.end(),[](const ContactParticle& p){return p.age>=p.life;}),particles.end());
    visual.shake_left=std::max(0.f,visual.shake_left-dt);visual.flash_left=std::max(0.f,visual.flash_left-dt);
    if(terminal.active){terminal.age+=dt;summary.hp=state.player.hp;summary.turns=state.turn;}
    if(CanAct())SyncCards();
}
void Game::BeginTerminal(float late){
    if(!turn_stars_||terminal.active||(state.phase!=Phase::Won&&state.phase!=Phase::Lost))return;
    terminal.active=true;terminal.age=late;terminal.entries=1;summary.turns=state.turn;summary.hp=state.player.hp;summary.seed=run.seed;
    focused_card=-1;pile_view=0;
}
void Game::PresentEvent(const undergod::Event& event,float late){
    if(event.type==EventType::CardPlayed&&!event.enemy)++summary.cards;
    if(event.type==EventType::Ultimate)++summary.ultimates;
    if(event.type==EventType::Damage){if(event.enemy)summary.damage+=event.amount;else summary.blocked+=event.blocked;}
    if((event.type==EventType::Heal||event.type==EventType::Revive)&&!event.enemy)summary.healed+=event.amount;
    auto popup=[&](int amount,EventType type,bool blocked=false){
        if(amount<=0||late>=.95f)return;
        int lane=0;for(;lane<8;++lane){bool used=false;for(const auto& p:popups)used|=p.enemy==event.enemy&&p.target==event.target&&p.lane==lane;if(!used)break;}
        if(lane<8)popups.push_back({event.enemy,amount,type,late,blocked,event.status,event.target,lane});
    };
    if(event.type==EventType::Damage){popup(event.blocked,EventType::Damage,true);popup(event.amount,EventType::Damage);}
    else if(event.type==EventType::Block||event.type==EventType::Heal||event.type==EventType::Status)popup(event.amount,event.type);
    else if(event.type==EventType::Revive)popup(event.amount,EventType::Heal);
    else return;
    const int unit=event.enemy?event.target:-1;if(unit>int(state.adds.size()))return;
    const auto point=UnitAim(unit);
    if(!ReducedFeedback()&&event.type==EventType::Damage&&event.amount>0&&!Cinematic()){
        visual.shake=std::min(4.f,event.amount>=12?4.f:2.f);visual.shake_left=std::max(visual.shake_left,std::max(0.f,.18f-late));
    }
    const int count=ReducedFeedback()?2:(event.type==EventType::Damage?6:8);
    for(int i=0;i<count;++i){
        const float vx=float(visual_rng_.Next()*160-80),vy=float(-30-visual_rng_.Next()*90);
        if(particles.size()<64&&late<.4f)particles.push_back({unit,event.type,point.first,point.second,late,.4f,vx,vy});
    }
}
CardPreview Game::Preview(int index)const{
    CardPreview p;if(index<0||index>=int(state.hand.size())){p.unavailable="Chưa chọn lá";return p;}
    const auto& c=state.Definition(state.hand[index]);p.cost=turn_stars_?1:c.cost;
    if(c.unplayable){p.unavailable="Lời nguyền không chơi được";return p;}
    Combat copy=state;copy.events.clear();copy.phase=Phase::Player;copy.live.locked=-1;
    const int rage=copy.rage;
    if(turn_stars_){
        copy.stars={};copy.stars.enabled=true;
        copy.stars.enemy={{0,{},true},{0,{},true},{0,{},true}};
        copy.QueueStar(index);copy.QueueWait();copy.QueueWait();copy.BeginStarAttack();copy.ResolveStarAction();
    }else{copy.energy=std::max(copy.energy,c.cost);copy.Play(index);}
    p.rage=copy.rage-rage;
    for(const auto& e:copy.events){
        if(e.type==EventType::Damage){if(e.enemy){p.damage+=e.amount;p.absorbed+=e.blocked;}else p.self_damage+=e.amount;}
        if(e.type==EventType::Block&&!e.enemy)p.block+=e.amount;
        if((e.type==EventType::Heal||e.type==EventType::Revive)&&!e.enemy)p.heal+=e.amount;
    }
    if(!CanAct())p.unavailable="Chờ hành động kết thúc";
    else if(index==state.live.locked)p.unavailable="Lá đang bị khóa";
    else if(turn_stars_&&state.IsQueuedStar(index))p.unavailable="Đã xếp vào hàng sao";
    else if(turn_stars_&&state.stars.player.size()>=3)p.unavailable="Hàng sao đã đầy";
    else if(!turn_stars_&&!state.CanPlay(index))p.unavailable="Không đủ năng lượng";
    return p;
}
int Game::IncomingLoss()const{
    if(!CanAct())return 0;
    Combat copy=state;copy.events.clear();
    if(turn_stars_){
        while(copy.stars.player.size()<3)copy.QueueWait();if(!copy.BeginStarAttack())return 0;
        for(int n=0;n<6&&copy.phase==Phase::Player;++n)copy.ResolveStarAction();
    }else{copy.EndPlayer();copy.ActEnemy();}
    int damage=0;for(const auto& e:copy.events)if(e.type==EventType::Damage&&!e.enemy)damage+=e.amount;return damage;
}
std::string Game::TurnStatus()const{
    if(terminal.active&&!terminal.settled)return state.phase==Phase::Won?"Kết liễu · chờ kết thúc":"Đang kết thúc trận";
    if(ResolvingStars()){
        const auto actor=active_actor_<0?"Bạn":state.TargetName(active_actor_);
        return actor+(motion_impact_?" · Hồi động tác":" · Đang thi triển");
    }
    if(Busy())return "Chờ chia bài / chuyển cảnh";
    return "Lượt của bạn";
}
ActionTiming Game::Timing()const{
    if(skill_visual_.active){const auto& a=skill_visual_.animation;return {a.launch,a.contact,a.duration};}
    const bool enemy=motion_==Motion::EnemyAttack||motion_==Motion::EnemyOther;
    return enemy?ActionTiming{.45f,motion_==Motion::EnemyAttack?.77f:.45f,1.15f}:ActionTiming{0,.32f,.65f};
}
void Game::SampleZoom(float at){
    visual.zoom_age=std::max(0.f,at-visual.zoom_time);const float t=visual.zoom_duration>0?std::clamp(visual.zoom_age/visual.zoom_duration,0.f,1.f):1;
    visual.zoom=visual.zoom_start+(visual.zoom_target-visual.zoom_start)*(1-(1-t)*(1-t));
    if(ReducedFeedback())visual.zoom=1;
}
void Game::FocusCard(int delta){
    if(!CanAct())return;std::vector<int> slots;for(int i=0;i<int(state.hand.size());++i)if(!turn_stars_||!state.IsQueuedStar(i))slots.push_back(i);
    if(slots.empty()){focused_card=-1;return;}
    auto at=std::find(slots.begin(),slots.end(),focused_card);int n=at==slots.end()?(delta<0?int(slots.size())-1:0):(int(at-slots.begin())+delta+int(slots.size()))%int(slots.size());
    focused_card=slots[n];pile_view=0;
}
void Game::TogglePile(int which){if(!CanAct()||which<1||which>2)return;pile_view=pile_view==which?0:which;pile_page=0;focused_card=-1;}
void Game::NewMatch(){if(Busy())return;session_seed_+=0x9e3779b9u;if(practice_)StartBossPractice(state.EnemyId(),session_seed_,run.class_id);else NewRun(session_seed_,run.class_id,run.challenge);}
void Game::Retry(){if(Busy())return;if(practice_)StartBossPractice(state.EnemyId(),run.seed,run.class_id);else NewRun(run.seed,run.class_id,run.challenge);}
bool Game::SelectPracticeDifficulty(PracticeDifficulty level){
    if(!practice_||Busy()||(level!=PracticeDifficulty::Gentle&&level!=PracticeDifficulty::Standard&&level!=PracticeDifficulty::Challenge))return false;
    practice_difficulty_=level;Retry();return true;
}
void Game::CyclePracticeDifficulty(){SelectPracticeDifficulty(practice_difficulty_==PracticeDifficulty::Standard?PracticeDifficulty::Challenge:(practice_difficulty_==PracticeDifficulty::Challenge?PracticeDifficulty::Gentle:PracticeDifficulty::Standard));}
}
