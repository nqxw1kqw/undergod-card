#include "game.hpp"
#include <cmath>

namespace undergod {
namespace {
constexpr float Pi=3.14159265358979323846f;
const char* MobProfile(const Intent& action){
    switch(action.kind){
    case IntentKind::Defend:return "mob_defend";case IntentKind::Buff:return "mob_buff";case IntentKind::Debuff:return "mob_debuff";
    case IntentKind::MultiAttack:return "mob_multi";case IntentKind::Charge:return "mob_charge";case IntentKind::Curse:return "mob_curse";
    case IntentKind::Summon:return "mob_summon";case IntentKind::Mimic:return "mob_mimic";case IntentKind::Drain:return "mob_drain";
    default:return action.status==StatusId::Poison&&action.status_amount>0?"mob_spit":"mob_attack";
    }
}
float EnemyX(const Combat& state,int target){
    const int count=int(state.adds.size())+1;if(count==1)return Game::EnemyHome;
    int slot=target;for(int i=0;i<int(state.live.slots.size());++i)if(state.live.slots[i]==target)slot=i;
    return 840.f+500.f*slot/std::max(1,count-1);
}
}
void Game::ResetSkillVisuals(){skill_visual_={};reactions_={};floating_fx_.clear();known_adds_=int(state.adds.size());ResetPresentation();}
float Game::ActorScale(bool enemy,int target)const{
    if(!enemy)return .5f;
    if(!state.adds.empty())return target==0||state.adds.at(target-1).illusion?.4f:.27f;
    return state.Sprite(target)=="tiger"?.65f:.5f;
}
std::pair<float,float> Game::UnitAim(int unit)const{
    const bool enemy=unit>=0;const int target=std::max(0,unit);
    const float chest=enemy&&state.Sprite(target)=="tiger"?145.f:180.f;
    return {(enemy?TargetX(target):PlayerHome)+ActorOffset(enemy,target),320.f+ActorLift(enemy,target)-chest*ActorScale(enemy,target)};
}
std::string Game::WardFor(int unit)const{
    const auto& stored=Reaction(unit).ward;if(!stored.empty())return stored;
    const auto id=unit<0?run.class_id:state.Sprite(unit);
    if(id=="tiger")return "ward_fire";
    if(id=="fox"||id=="undergod"||id=="ninja")return "ward_shadow";
    if(id=="frog_king"||id=="snake"||id=="tadpole")return "ward_leaf";
    return "ward_blue";
}
void Game::SyncSkillShields(){
    if(!turn_stars_)return;
    for(int unit=-1;unit<=int(state.adds.size());++unit){auto& r=reactions_[unit+1];const int block=UnitBlock(unit);
        if(block>r.last_block)r.guard_age=0;
        if(block==0&&r.last_block>0)r.break_age=0;
        r.last_block=block;
    }
    for(int index=known_adds_;index<int(state.adds.size());++index){
        const int unit=index+1;reactions_[unit+1].spawn_age=0;const auto at=UnitAim(unit);
        if(!skill_visual_.active||skill_visual_.animation.impact_fx!="summon")floating_fx_.push_back({"summon",at.first,310,0,145});
    }
    known_adds_=int(state.adds.size());
}
void Game::TickSkillVisuals(float dt){
    if(!turn_stars_)return;
    for(auto& r:reactions_){r.hit_age+=dt;r.guard_age+=dt;r.break_age+=dt;r.spawn_age+=dt;r.defeat_age+=dt;}
    for(auto& fx:floating_fx_)fx.age+=dt;
    floating_fx_.erase(std::remove_if(floating_fx_.begin(),floating_fx_.end(),[](const FloatingFx& f){return f.age>.65f;}),floating_fx_.end());
    SyncSkillShields();
}
void Game::ReactSkillEvent(const undergod::Event& event,float late){
    if(!turn_stars_)return;
    const int unit=event.enemy?event.target:-1;if(unit< -1||unit>int(state.adds.size()))return;
    auto& r=reactions_[unit+1];const auto point=UnitAim(unit);
    if(!r.defeated&&(unit<0?state.player.hp:state.Target(unit).hp)<=0){r.defeated=true;r.defeat_age=late;r.defeat_x=(unit<0?PlayerHome:TargetX(unit))+ActorOffset(unit>=0,std::max(0,unit));r.defeat_y=320+ActorLift(unit>=0,std::max(0,unit));}
    if(event.type==EventType::Damage){if(event.amount>0)r.hit_age=late;if(event.blocked>0)r.guard_age=late;}
    else if(event.type==EventType::Block)r.guard_age=late;
    else if(event.type==EventType::Heal||event.type==EventType::Revive)floating_fx_.push_back({"heal",point.first,point.second,late,145});
    else if(event.type==EventType::Status)floating_fx_.push_back({event.status==StatusId::Poison?"toxin_orb":"sigil",point.first,point.second,late,80});
}
void Game::BeginSkillVisual(bool enemy,int slot){
    skill_visual_={};auto& v=skill_visual_;v.enemy=enemy;v.actor=enemy?state.stars.enemy.at(slot).actor:-1;v.card_id=active_card_.id;
    const Intent* action=enemy?&state.stars.enemy.at(slot).action:nullptr;
    const bool wait=enemy?(state.stars.enemy.at(slot).wait||state.Target(v.actor).hp<=0):state.stars.player.at(slot).id.empty();
    std::string key=wait?"wait":(enemy?(action->deck_card_id.empty()?MobProfile(*action):action->deck_card_id):active_card_.id);
    v.animation=FindSkillAnimation(state.config,key);
    if(enemy&&action->kind==IntentKind::Mimic&&!action->card_id.empty()&&!wait){
        v.animation=FindSkillAnimation(state.config,action->card_id);v.animation.id=key;
        v.animation.launch+=.45f;v.animation.contact+=.45f;v.animation.duration+=.45f;v.animation.cast_fx="ward_shadow";v.animation.ward="ward_shadow";
    }
    if(enemy&&action->kind==IntentKind::Charge&&!wait&&active_card_.damage>0){
        v.animation.motion=SkillMotion::Pounce;v.animation.pose="pounce";v.animation.self=false;v.animation.cast_fx=state.Sprite(v.actor)=="tiger"?"fire_orb":"sigil";v.animation.impact_fx=state.Sprite(v.actor)=="tiger"?"claw":"shock";
        v.animation.launch=.45f;v.animation.contact=.77f;v.animation.duration=1.18f;
    }
    v.amplified=active_card_.upgraded;v.active=true;
    const std::string actor=enemy?state.Sprite(v.actor):FindClass(state.config,run.class_id).sprite;
    auto exists=[&](const std::string& pose){for(const auto& b:state.config.animations.bodies)if(b.actor==actor&&b.pose==pose)return true;return false;};
    const auto& wanted=v.animation.pose;
    if(wanted=="idle")v.pose="idle";
    else if(wanted=="melee")v.pose=enemy?"attack":(actor=="ninja"?"slash":"thrust");
    else if(wanted=="charge")v.pose=actor=="tiger"?"ult_charge":(exists("cast")?"cast":"attack");
    else if(wanted=="pounce")v.pose=exists("pounce")?"pounce":"attack";
    else if(wanted=="guard")v.pose=exists("guard")?"guard":(exists("cast")?"cast":"defend");
    else if(wanted=="peck"||wanted=="stomp")v.pose=exists(wanted)?wanted:(enemy?"attack":(actor=="ninja"?"slash":"thrust"));
    else v.pose=actor=="ninja"?"throw":(exists("cast")?"cast":(actor=="tiger"?"ult_charge":"attack"));
    if(!v.animation.ward.empty())reactions_[v.actor+1].ward=v.animation.ward;
    const float scale=ActorScale(enemy,std::max(0,v.actor));
    float anchor_x=enemy?105.f:400.f,anchor_y=240;
    for(const auto& b:state.config.animations.bodies)if(b.actor==actor&&b.pose==v.pose){anchor_x=b.emitter_x;anchor_y=b.emitter_y;break;}
    v.source_x=(enemy?TargetX(v.actor):PlayerHome)+(anchor_x-256)*scale;v.source_y=320+(anchor_y-456)*scale;
    if(key=="boss_frog_tongue"){v.source_x=TargetX(v.actor)-35*scale;v.source_y=320-225*scale;}
    if(actor=="tiger"&&v.animation.path!=FlightPath::None){v.source_x=TargetX(v.actor)-70*scale;v.source_y=320-230*scale;}
    auto target=[&](int unit){const auto point=UnitAim(unit);v.targets.push_back({unit,point.first,point.second});};
    if(v.animation.self)target(v.actor);
    else if(enemy)target(-1);
    else{
        bool random=false,all=false;for(const auto& effect:active_card_.effects){random|=effect.target==EffectTarget::RandomEnemy;all|=effect.target==EffectTarget::AllEnemies;}
        if(random){
            Combat preview=state;preview.events.clear();preview.ResolveStarAction();
            for(const auto& e:preview.events)if(e.type==EventType::Damage&&e.enemy){
                const int unit=e.target;if(unit<=int(state.adds.size()))target(unit);else v.targets.push_back({unit,EnemyX(preview,unit),270});
            }
        }else if(all)for(int unit:state.Targets())target(unit);
        if(v.targets.empty())target(state.live.target);
    }
    if(enemy&&action->kind==IntentKind::Summon&&!wait){
        Combat preview=state;preview.events.clear();preview.ResolveStarAction();v.targets.clear();
        for(int index=int(state.adds.size());index<int(preview.adds.size());++index)v.targets.push_back({index+1,EnemyX(preview,index+1),310});
        if(v.targets.empty())target(v.actor);
    }
}
int Game::SkillFrame()const{
    if(!skill_visual_.active)return 0;const auto& a=skill_visual_.animation;const float age=skill_visual_.age;
    if(skill_visual_.pose=="cast"||skill_visual_.pose=="throw"){
        if(age<a.launch*.5f)return 0;if(age<a.launch)return 1;if(age<a.contact+(a.duration-a.contact)*.45f)return 2;return 3;
    }
    const float strike=a.motion==SkillMotion::Stomp?a.launch:a.contact;
    if(age<std::min(.10f,strike*.3f))return 0;if(age<strike)return 1;if(age<strike+(a.duration-strike)*.45f)return 2;return 3;
}
float Game::ActorLift(bool enemy,int target)const{
    if(!SkillActor(enemy,target))return 0;const auto& v=skill_visual_;const auto& a=v.animation;
    const float move=std::clamp((v.age-a.launch)/std::max(.01f,a.contact-a.launch),0.f,1.f);
    if(a.motion==SkillMotion::Stomp)return v.age<a.launch?-28*std::sin(Pi*v.age/std::max(.01f,a.launch)):0;
    if(a.motion==SkillMotion::Pounce)return -std::sin(Pi*move)*(enemy&&state.Sprite(target)=="tiger"?45.f:30.f);
    if(a.motion==SkillMotion::Swoop)return -std::sin(Pi*move)*std::max(30.f,a.arc);
    if(a.motion==SkillMotion::Cast||a.motion==SkillMotion::Roar)return -std::sin(std::min(1.f,v.age/a.duration)*Pi)*5;
    return 0;
}
float Game::ActorAngle(bool enemy,int target)const{
    if(!SkillActor(enemy,target))return 0;const auto& v=skill_visual_;const auto m=v.animation.motion;
    if(m==SkillMotion::Roar)return std::sin(v.age*Pi/v.animation.duration)*(enemy?.05f:-.05f);
    if(m==SkillMotion::Dash||m==SkillMotion::Swoop)return std::sin(v.age*Pi/v.animation.duration)*(enemy?-.12f:.12f);
    return 0;
}
std::vector<ProjectileSample> Game::Projectiles()const{
    std::vector<ProjectileSample> result;const auto& v=skill_visual_;const auto& a=v.animation;
    if(!v.active||a.path==FlightPath::None||a.projectile.empty()||v.targets.empty()||v.age<a.launch)return result;
    const bool returning=a.path==FlightPath::Return&&v.age>=a.contact;
    if(v.age>=a.contact&&a.path!=FlightPath::Return&&a.path!=FlightPath::Orbit)return result;
    if(v.age>=a.duration-.05f)return result;
    float t=std::clamp((v.age-a.launch)/std::max(.01f,a.contact-a.launch),0.f,1.f);
    if(returning)t=1-std::clamp((v.age-a.contact)/std::max(.01f,a.duration-a.contact-.05f),0.f,1.f);
    const int count=std::max(a.count,int(v.targets.size()));
    for(int i=0;i<count;++i){
        const auto& target=v.targets[i%v.targets.size()];float sx=v.source_x,sy=v.source_y,tx=target.x,ty=target.y;
        const float lane=count>1?(2.f*i/(count-1)-1):0;
        if(a.path==FlightPath::Orbit){
            const float angle=(v.age-a.launch)*6+2*Pi*i/count,radius=a.arc*std::min(1.f,(v.age-a.launch)/.12f);
            result.push_back({target.x+std::cos(angle)*radius,target.y+std::sin(angle)*radius*.55f,angle+Pi/2,t,target.unit,false});continue;
        }
        if(a.path==FlightPath::Drain){std::swap(sx,tx);std::swap(sy,ty);}
        if(a.path==FlightPath::Rain){sx=tx+lane*a.spread;sy=std::max(-20.f,50-a.arc*.3f);}
        if(a.path==FlightPath::Ground)sy=ty=304;
        float x=sx+(tx-sx)*t,y=sy+(ty-sy)*t,dy=ty-sy;
        if(a.path==FlightPath::Arc||a.path==FlightPath::Return){y-=a.arc*std::sin(Pi*t);dy-=a.arc*Pi*std::cos(Pi*t);}
        if(a.path==FlightPath::Fan){y+=(lane*a.spread-a.arc)*std::sin(Pi*t);dy+=(lane*a.spread-a.arc)*Pi*std::cos(Pi*t);}
        if(a.path==FlightPath::Helix){const float phase=t*2*Pi+2*Pi*i/count;y+=std::sin(phase)*a.arc*std::sin(Pi*t);dy+=a.arc*Pi*(2*std::cos(phase)*std::sin(Pi*t)+std::sin(phase)*std::cos(Pi*t));}
        if(a.path==FlightPath::Drain){y+=lane*a.spread*std::sin(Pi*t);dy+=lane*a.spread*Pi*std::cos(Pi*t);}
        result.push_back({x,y,std::atan2(dy,tx-sx)+(returning?Pi:0),t,target.unit,returning});
    }
    return result;
}
}
