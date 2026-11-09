#include "game.hpp"
#include <chrono>

namespace undergod {
ForecastStep DescribeForecastAction(const Combat& c,int at){
    ForecastStep step;step.actor=at%2?c.stars.enemy.at(at/2).actor:-1;
    if(at%2){const auto& q=c.stars.enemy.at(at/2);step.wait=q.wait;step.name=q.wait?"Chờ":c.EnemyStarView(at/2).name;}
    else{const auto& q=c.stars.player.at(at/2);step.wait=q.id.empty();step.name=step.wait?"Chờ":c.Definition(q.id).name;}
    return step;
}
void FinishForecastAction(ForecastStep& s,const Combat& c){
    s.hp=c.player.hp;s.block=c.player.block;s.resolved=true;
    std::map<StatusId,int> statuses;
    for(const auto& e:c.events){
        if(e.type==EventType::Damage){(e.enemy?s.damage_dealt:s.damage_taken)+=e.amount;s.absorbed+=e.blocked;}
        else if(e.type==EventType::Heal||e.type==EventType::Revive)s.healed+=e.amount;
        else if(e.type==EventType::Block)s.shield+=e.amount;
        else if(e.type==EventType::Status)statuses[e.status]+=e.amount;
    }
    auto add=[&](const std::string& text){if(!s.effects.empty())s.effects+=" · ";s.effects+=text;};
    if(s.damage_dealt)add("Địch −"+std::to_string(s.damage_dealt));
    if(s.damage_taken)add("Ta −"+std::to_string(s.damage_taken));
    if(s.absorbed)add("Chặn "+std::to_string(s.absorbed));
    if(s.shield)add("Giáp +"+std::to_string(s.shield));
    if(s.healed)add("Hồi +"+std::to_string(s.healed));
    for(const auto& v:statuses)if(v.second)add(std::string(StatusName(v.first))+" +"+std::to_string(v.second));
    if(s.effects.empty())s.effects=s.wait?"Chờ":(s.cancelled?"Bỏ qua":"Hỗ trợ / chuẩn bị");
}
void Game::EnsureForecastCache()const{
    auto& c=forecast_cache_;++c.metrics.probes;c.probe.clear();c.probe+=turn_stars_?'1':'0';state.AppendForecastKey(c.probe);
    if(c.probe==c.key)return;
    c.key=c.probe;c.round_valid=false;c.card_valid.fill(false);
}
RoundForecast Game::EvaluateRound()const{
    RoundForecast r;if(!turn_stars_||!state.stars.enabled||state.stars.resolving||state.phase!=Phase::Player)return r;
    Combat copy=state;copy.events.clear();const int selected=int(copy.stars.player.size());r.incomplete=selected<3;
    while(copy.stars.player.size()<3)if(!copy.QueueWait())return r;
    if(!copy.BeginStarAttack())return r;r.valid=true;
    for(int i=0;i<6;++i){
        auto& s=r.steps[i];s=DescribeForecastAction(copy,i);s.assumed_wait=i%2==0&&i/2>=selected;
        if(copy.phase!=Phase::Player){s.cancelled=true;s.hp=copy.player.hp;s.block=copy.player.block;s.effects="Hủy · trận đã kết thúc";continue;}
        s.cancelled=s.actor>=0&&!s.wait&&copy.Target(s.actor).hp<=0;
        copy.events.clear();copy.ResolveStarAction();FinishForecastAction(s,copy);s.resolved=false;
        r.damage_taken+=s.damage_taken;r.damage_dealt+=s.damage_dealt;
    }
    return r;
}
const RoundForecast& Game::Forecast()const{
    if(!CanAct())return active_round_;
    EnsureForecastCache();auto& c=forecast_cache_;
    if(!c.round_valid){const auto start=std::chrono::steady_clock::now();c.round=EvaluateRound();c.round_valid=true;++c.metrics.round_builds;c.metrics.build_us+=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();}
    return c.round;
}
CardPreview Game::Preview(int index)const{
    if(index<0||index>=int(state.hand.size())||index>=10)return EvaluateCard(index);
    EnsureForecastCache();auto& c=forecast_cache_;
    if(!c.card_valid[index]){c.cards[index]=EvaluateCard(index);c.card_valid[index]=true;++c.metrics.card_builds;}
    auto p=c.cards[index];p.unavailable.clear();const auto& card=state.Definition(state.hand[index]);
    if(card.unplayable)p.unavailable="Lời nguyền không chơi được";
    else if(!CanAct())p.unavailable="Chờ hành động kết thúc";
    else if(index==state.live.locked)p.unavailable="Lá đang bị khóa";
    else if(turn_stars_&&state.IsQueuedStar(index))p.unavailable="Đã xếp vào hàng sao";
    else if(turn_stars_&&state.stars.player.size()>=3)p.unavailable="Hàng sao đã đầy";
    else if(!turn_stars_&&!state.CanPlay(index))p.unavailable="Không đủ năng lượng";
    return p;
}
void Game::ToggleForecast(){if(turn_stars_&&(CanAct()||ResolvingStars())){forecast_view=!forecast_view;pile_view=0;focused_card=-1;}}
}
