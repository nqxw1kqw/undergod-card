#include "game.hpp"
#include <cmath>

namespace undergod {
void Game::BeginArrival(bool banner){
    arrival_banner_=banner;arrival_=banner?Arrival::FadeIn:Arrival::Deal;arrival_age_=0;
}
void Game::UpdateArrival(float dt){
    if(arrival_==Arrival::None)return;
    arrival_age_+=dt;
    for(int guard=0;guard<12&&arrival_!=Arrival::None;++guard){
        const float duration=arrival_==Arrival::Travel?.85f:(arrival_==Arrival::FadeOut?.48f:(arrival_==Arrival::BlackHold?.12f:(arrival_==Arrival::FadeIn?.45f:(arrival_==Arrival::Fight?1.35f:.20f+.12f*state.hand.size()))));
        if(arrival_age_<duration)break;
        arrival_age_-=duration;
        if(arrival_==Arrival::Travel)arrival_=Arrival::FadeOut;
        else if(arrival_==Arrival::FadeOut){
            if(!undergod::ChooseNode(run,state,pending_option_)){arrival_=Arrival::None;Toast("Không thể tới khu vực này.");break;}
            pending_option_=-1;poses={};popups.clear();visual={};motion_=Motion::None;
            arrival_banner_=run.place==Place::Fight;
            if(arrival_banner_)state.EnableTurnStars();
            arrival_=Arrival::BlackHold;save_dirty_=true;
        }else if(arrival_==Arrival::BlackHold)arrival_=Arrival::FadeIn;
        else if(arrival_==Arrival::FadeIn)arrival_=arrival_banner_?Arrival::Fight:Arrival::None;
        else if(arrival_==Arrival::Fight){arrival_=Arrival::Deal;sound_events.push_back("select");}
        else{arrival_=Arrival::None;arrival_age_=0;save_dirty_=true;}
    }
}
float Game::FadeOpacity()const{
    if(arrival_==Arrival::BlackHold)return 1;
    if(arrival_==Arrival::FadeOut)return std::clamp(arrival_age_/.48f,0.f,1.f);
    if(arrival_==Arrival::FadeIn)return 1-std::clamp(arrival_age_/.45f,0.f,1.f);
    return 0;
}
float Game::FightBannerX()const{
    const auto ease=[](float x){x=std::clamp(x,0.f,1.f);return x*x*(3-2*x);};
    if(arrival_age_<.35f)return 1840+(720-1840)*ease(arrival_age_/.35f);
    if(arrival_age_<.90f)return 720;
    return 720+(-420-720)*ease((arrival_age_-.90f)/.45f);
}
float Game::DealtCards()const{
    if(arrival_==Arrival::Fight||arrival_==Arrival::FadeIn||arrival_==Arrival::BlackHold)return 0;
    if(arrival_==Arrival::Deal)return std::clamp((arrival_age_-.08f)/.12f,0.f,float(state.hand.size()));
    return float(state.hand.size());
}
bool Game::QueueWait(){const bool result=CanAct()&&turn_stars_&&state.QueueWait();save_dirty_|=result;return result;}
bool Game::UnqueueCard(int slot){const bool result=CanAct()&&state.UnqueueStar(slot);save_dirty_|=result;if(result)sound_events.push_back("select");return result;}
void Game::BeginStarMotion(){
    const int cursor=state.stars.cursor,slot=cursor/2;
    const bool enemy=cursor%2!=0;
    active_actor_=-1;
    if(enemy){active_card_=state.EnemyStarView(slot);active_actor_=state.stars.enemy.at(slot).actor;}
    else{
        const auto& q=state.stars.player.at(slot);
        if(q.id.empty()){active_card_={};active_card_.name="Chờ";active_card_.description="Giữ nhịp, không thực hiện hành động.";active_card_.cost=1;}
        else active_card_=state.StarCardView(state.Definition(q.id));
        if(q.target<=int(state.adds.size())&&state.Target(q.target).hp>0)state.SelectTarget(q.target);
        else{const auto targets=state.Targets();if(!targets.empty())state.SelectTarget(targets.front());}
    }
    const bool attack=active_card_.damage>0;
    motion_=enemy?(attack?Motion::EnemyAttack:Motion::EnemyOther):(attack?Motion::PlayerAttack:Motion::PlayerOther);
    motion_age_=0;motion_impact_=false;
    if(!enemy)poses[0]={attack?"attack":"defend",0};
}
void Game::FinishStarMotion(){
    if(state.phase==Phase::Won||state.phase==Phase::Lost){star_sequence_=false;state.stars.resolving=false;active_actor_=-1;return;}
    if(state.stars.cursor<Combat::TurnStars*2){BeginStarMotion();return;}
    state.FinishStarRound();star_sequence_=false;active_actor_=-1;AfterResolve();
    if(state.phase==Phase::Player&&run.place==Place::Fight)BeginArrival(false);
}
AutoStep Game::Suggest()const{
    if(!turn_stars_||run.place!=Place::Fight||!state.stars.enabled)return SuggestAuto(run,state);
    if(!CanAct())return {};
    if(state.CanUltimate())return {AutoKind::Ultimate};
    if(state.player.hp<state.player.max_hp/2)for(int i=0;i<int(run.potions.size());++i)if(run.potions[i]=="serum")return {AutoKind::Potion,i};
    if(state.stars.player.size()==Combat::TurnStars)return {AutoKind::EndTurn};
    int best=-1;float score=-10000;
    int incoming=0;for(int i=0;i<int(state.stars.enemy.size());++i)incoming+=state.EnemyStarView(i).damage;
    for(int i=0;i<int(state.hand.size());++i)if(state.CanQueueStar(i)){
        const auto& c=state.Definition(state.hand[i]);float value=float(c.damage)+c.poison*2.4f+c.strength*3+c.cards_drawn*2;
        value+=std::min(c.heal,state.player.max_hp-state.player.hp)*1.5f;
        value+=std::min(c.block,std::max(0,incoming-state.player.block))*.95f;
        value+=c.weak*2+c.vulnerable*2;if(c.power)value+=4;
        if(value>score){score=value;best=i;}
    }
    return best>=0?AutoStep{AutoKind::Play,best}:AutoStep{AutoKind::Wait};
}
int Game::SuggestCard()const{return turn_stars_?(Suggest().kind==AutoKind::Play?Suggest().index:-1):undergod::SuggestCard(state);}
}
