#include "game.hpp"
#include <cmath>

namespace undergod {
void Game::NewRun(unsigned seed){seed_=seed;cinematic_.Cancel();state.Reset(seed);motion_=Motion::None;pending_card_=-1;motion_age_=cinematic_age_=0;motion_impact_=false;speed_=1;poses={};popups.clear();visual={};time=toast_age=0;toast.clear();next_visual_id_=1;cards.clear();DrawCards(.2f);}
void Game::DrawCards(float age){
    for(int i=0;i<int(state.hand.size());++i){
        auto target=HandBox(int(state.hand.size()),i);
        cards.push_back({next_visual_id_++,state.hand[i],i,{280,640,80,100},target,age,.10f,.01f*i,false,false});
    }
}
void Game::EnsureCards(){
    int count=0;bool match=true;
    for(const auto& c:cards)if(!c.leaving){++count;match&=c.slot>=0&&c.slot<int(state.hand.size())&&c.card==state.hand[c.slot];}
    if(!match||count!=int(state.hand.size())){cards.clear();DrawCards(.2f);}
}
void Game::ResolveCardVisual(){
    for(auto& c:cards){
        if(c.selected){c.from=c.Bounds();c.to={280,665,80,100};c.age=0;c.duration=motion_==Motion::PlayerOther?.57f:.25f;c.via_play=motion_==Motion::PlayerOther;c.delay=0;c.leaving=true;c.selected=false;c.slot=-1;}
        else if(!c.leaving){
            if(c.slot>pending_card_)--c.slot;
            c.from=c.Bounds();c.to=HandBox(int(state.hand.size()),c.slot);c.age=0;c.delay=0;c.duration=.25f;
        }
    }
}
void Game::ConsumeEvents(){
    for(const auto& event:state.events){
        if(event.type==EventType::Damage){const bool blocked=event.amount==0&&event.blocked>0;poses[event.enemy?1:0]={blocked?"defend":"hit",0};popups.push_back({event.enemy,event.amount?event.amount:event.blocked,EventType::Damage,0,blocked});}
        else if(event.type==EventType::Block){poses[event.enemy?1:0]={"defend",0};popups.push_back({event.enemy,event.amount,EventType::Block,0});}
        else if(event.type==EventType::Heal)popups.push_back({event.enemy,event.amount,EventType::Heal,0});
    }
    state.events.clear();
}
void Game::Play(int index){
    if(!CanAct()||index<0||index>=int(state.hand.size()))return;
    if(!state.CanPlay(index)){Toast("Không đủ năng lượng cho lá bài này.");return;}
    EnsureCards();
    for(auto& c:cards)if(c.slot==index&&!c.leaving){c.from=c.Bounds();c.to={632,485,176,195};c.age=0;c.delay=0;c.duration=.32f;c.selected=true;}
    pending_card_=index;const bool attack=state.Definition(state.hand[index]).damage>0;
    motion_=attack?Motion::PlayerAttack:Motion::PlayerOther;motion_age_=0;motion_impact_=false;
    if(attack)poses[0]={"attack",0};else ApplyMotion();
}
void Game::EndTurn(){if(!CanAct())return;EnsureCards();if(!state.EndPlayer())return;
    for(auto& c:cards){c.from=c.Bounds();c.to={280,665,80,100};c.age=0;c.delay=.015f*std::max(0,c.slot);c.duration=.25f;c.leaving=true;c.slot=-1;}resolving_intent_=state.intent;motion_=state.intent.attack?Motion::EnemyAttack:Motion::EnemyOther;motion_age_=0;motion_impact_=false;ConsumeEvents();}
void Game::ApplyMotion(){
    if(motion_impact_)return;
    motion_impact_=true;
    if(motion_==Motion::PlayerAttack||motion_==Motion::PlayerOther){state.Play(pending_card_);ResolveCardVisual();}
    else state.ActEnemy();
    ConsumeEvents();
}
void Game::CueStep(const Step& step){
    const float late=std::max(0.f,cinematic_age_-step.t/1000.f);
    switch(step.cue){
    case Cue::Dim:visual.dim=step.value;break;
    case Cue::Zoom:visual.zoom_target=step.value;visual.zoom_duration=step.duration/1000.f;visual.zoom_age=0;break;
    case Cue::Pose:poses[step.enemy?1:0]={step.pose,late};break;
    case Cue::Vfx:visual.vfx_age=late;visual.vfx_frames=int(step.value);visual.vfx_enemy=step.enemy;break;
    case Cue::Spear:visual.spear={true,step.falling,late,step.duration/1000.f,step.from_x,step.from_y,step.to_x,step.to_y};break;
    case Cue::Shake:visual.shake=step.value;visual.shake_left=std::max(0.f,step.duration/1000.f-late);break;
    case Cue::Flash:visual.flash_duration=step.duration/1000.f;visual.flash_left=std::max(0.f,visual.flash_duration-late);break;
    default:break;
    }
}
void Game::Ultimate(){
    if(!CanAct())return;
    if(!state.StartUltimate()){Toast("Sao Rơi cần đầy 100 Nộ.");return;}
    ConsumeEvents();visual={};cinematic_age_=0;
    cinematic_.Start(timeline_,[this](const Step& s){CueStep(s);},[this]{state.UltimateImpact();ConsumeEvents();},[this]{state.FinishUltimate();visual={};poses[0]={};});
}
void Game::SkipCinematic(){cinematic_.Skip();}
void Game::Update(float real_dt){
    real_dt=std::max(0.f,real_dt);toast_age=std::max(0.f,toast_age-real_dt);
    const float dt=real_dt*PlaybackRate();time+=dt;
    for(auto& c:cards)c.age+=dt;
    cards.erase(std::remove_if(cards.begin(),cards.end(),[](const CardVisual& c){return c.leaving&&c.age>=c.delay+c.duration;}),cards.end());
    for(auto& p:poses){p.age+=dt;const bool held=p.name=="ult_throw"&&Cinematic();if(!held&&p.name!="idle"&&p.age>(p.name=="ult_charge"?2.f:.65f))p={};}
    for(auto& p:popups)p.age+=dt;
    popups.erase(std::remove_if(popups.begin(),popups.end(),[](const auto& p){return p.age>.95f;}),popups.end());
    if(motion_==Motion::TurnTransition){
        motion_age_+=dt;if(motion_age_>=.20f){motion_=Motion::None;motion_age_=0;}
    }else if(motion_!=Motion::None){
        const bool enemy=motion_==Motion::EnemyAttack||motion_==Motion::EnemyOther;
        const float before=motion_age_;motion_age_+=dt;
        if(enemy&&before<.45f&&motion_age_>=.45f)poses[1]={resolving_intent_.attack?"attack":"defend",motion_age_-.45f};
        const float impact=enemy?(motion_==Motion::EnemyAttack?.77f:.45f):.32f;
        if(motion_age_>=impact&&!motion_impact_){
            ApplyMotion();
            for(auto& c:cards)if(!enemy)c.age=std::max(0.f,motion_age_-impact);
        }
        if(motion_age_>=(enemy?1.15f:.65f)){
            const float overshoot=motion_age_-(enemy?1.15f:.65f);
            motion_=enemy&&state.phase==Phase::Player&&overshoot<.20f?Motion::TurnTransition:Motion::None;
            motion_age_=overshoot;pending_card_=-1;
            if(enemy&&state.phase==Phase::Player)DrawCards(overshoot);
        }
    }
    if(Cinematic()){
        cinematic_age_+=dt;
        visual.zoom_age+=dt;
        const float z=std::min(1.f,visual.zoom_duration>0?visual.zoom_age/visual.zoom_duration:1.f);
        visual.zoom=1+(visual.zoom_target-1)*(1-(1-z)*(1-z));
        if(visual.vfx_age>=0)visual.vfx_age+=dt;
        if(visual.spear.visible)visual.spear.age+=dt;
        visual.shake_left=std::max(0.f,visual.shake_left-dt);visual.flash_left=std::max(0.f,visual.flash_left-dt);
        cinematic_.Advance(cinematic_age_*1000.f);
    }else cinematic_age_=0;
    cards.erase(std::remove_if(cards.begin(),cards.end(),[](const CardVisual& c){return c.leaving&&c.age>=c.delay+c.duration;}),cards.end());
}
std::pair<float,float> Game::SpearTip()const{
    const auto& s=visual.spear;const float t=std::clamp(s.age/std::max(.001f,s.duration),0.f,1.f);
    const float progress=s.falling?t*t:1-(1-t)*(1-t);
    return {s.from_x+(s.to_x-s.from_x)*progress,s.from_y+(s.to_y-s.from_y)*progress};
}
float Game::ActorOffset(bool enemy)const{
    const auto& p=poses[enemy?1:0];float recoil=p.name=="hit"?std::sin(p.age*60.f)*12.f*std::max(0.f,1-p.age/.3f)*(enemy?1.f:-1.f):0;
    if((enemy&&motion_!=Motion::EnemyAttack)||(!enemy&&motion_!=Motion::PlayerAttack))return recoil;
    const float t=motion_age_-(enemy?.45f:0.f);float f=0;
    if(t>0&&t<.22f)f=(t/.22f)*(t/.22f);else if(t>=.22f&&t<.32f)f=1;else if(t>=.32f&&t<.54f)f=1-(t-.32f)/.22f;
    const float contact_travel=EnemyHome-PlayerHome-(enemy?170.f:160.f);
    return (enemy?-1.f:1.f)*contact_travel*f+recoil;
}
}
