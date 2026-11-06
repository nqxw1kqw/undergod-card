#include "ui.hpp"
#include <DxLib.h>
#include <cmath>

namespace undergod {
namespace {
constexpr float Pi=3.14159265358979323846f;
void Alpha(int value){SetDrawBlendMode(value>=255?DX_BLENDMODE_NOBLEND:DX_BLENDMODE_ALPHA,std::clamp(value,0,255));}
}
void Ui::DrawFx(const std::string& effect,float x,float y,float size,float progress,float angle,int alpha)const{
    if(effect.empty()||alpha<=0)return;
    const int frame=std::clamp(int(progress*4),0,3);
    int graph=Image("skill_"+effect+"_"+std::to_string(frame));
    if(graph<0)graph=Image(effect);
    if(graph<0)return;
    Alpha(alpha);
    if(effect=="spear_impact")DrawRectRotaGraph3F(x,y,(frame%2)*256,(frame/2)*256,256,256,128,128,size/256,size/256,angle,graph,TRUE);
    else{
        int width=0,height=0;GetGraphSize(graph,&width,&height);
        const float rotation=angle+(effect=="spear"?Pi/2:0);
        DrawRotaGraph3F(x,y,width/2.f,height/2.f,size/std::max(width,height),size/std::max(width,height),rotation,graph,TRUE);
    }
    Alpha(255);
}
void Ui::SkillActor(const Game& game,bool enemy,int target)const{
    const int unit=enemy?target:-1;const auto& reaction=game.Reaction(unit);
    const bool dead=(enemy?game.state.Target(target).hp:game.state.player.hp)<=0;
    if(dead&&reaction.hit_age>=.32f)return;
    const auto id=enemy?game.state.Sprite(target):FindClass(game.state.config,game.run.class_id).sprite;
    std::string pose="idle";int frame=int(game.time*4)%4;
    if(game.SkillActor(enemy,target)){pose=game.SkillVisual().pose;frame=game.SkillFrame();}
    else if(!enemy&&game.Cinematic()){
        pose=game.poses[0].name;frame=std::min(3,int(game.poses[0].age*(pose=="ult_throw"?4.f:8.f)));
    }else if(reaction.hit_age<.32f){pose="hit";frame=std::min(3,int(reaction.hit_age*12));}
    int graph=Image(id+"_"+pose+"_"+std::to_string(frame));
    if(graph<0)graph=Image(id+"_idle_0");
    const float scale=game.ActorScale(enemy,target);
    const float x=(enemy?game.TargetX(target):Game::PlayerHome)+game.ActorOffset(enemy,target);
    const float y=320+game.ActorLift(enemy,target)-(pose=="idle"?std::sin(game.time*3.14f)*2:0);
    Alpha(38);DrawOval(int(x),324,int(105*scale),9,GetColor(80,107,76),TRUE);Alpha(255);
    const float opacity=dead?std::max(0.f,1-reaction.hit_age/.32f):std::min(1.f,reaction.spawn_age/.24f);
    if(graph>=0){
        Alpha(int(opacity*255));
        DrawRotaGraph3F(x,y,256,456,scale,scale,game.ActorAngle(enemy,target),graph,TRUE);
        if(reaction.hit_age<.09f){SetDrawBlendMode(DX_BLENDMODE_ADD,int(110*opacity));DrawRotaGraph3F(x,y,256,456,scale,scale,game.ActorAngle(enemy,target),graph,TRUE);}
        Alpha(255);
    }
}
void Ui::DrawWard(const Game& game,int unit)const{
    const bool enemy=unit>=0;const auto& reaction=game.Reaction(unit);
    if((enemy?game.state.Target(unit).hp:game.state.player.hp)<=0)return;
    const bool breaking=game.UnitBlock(unit)==0;
    if(breaking&&reaction.break_age>=.30f)return;
    const auto at=game.UnitAim(unit);const float scale=game.ActorScale(enemy,std::max(0,unit));
    const float pulse=std::max(0.f,1-reaction.guard_age/.32f);
    float opacity=105+100*pulse,size=555*scale*(1+.05f*pulse);
    if(breaking){const float t=reaction.break_age/.30f;opacity=145*(1-t);size*=1+t*.3f;}
    DrawFx(game.WardFor(unit),at.first,320+game.ActorLift(enemy,std::max(0,unit))-200*scale,size,std::fmod(game.time*2,1.f),0,int(opacity));
}
void Ui::DrawSkillEffects(const Game& game)const{
    const auto& v=game.SkillVisual();
    if(v.active){
        const auto& a=v.animation;
        const float age=v.age,charge=std::clamp(age/std::max(.01f,a.contact),0.f,1.f);
        const float boost=v.amplified?1.14f:1.f;
        const auto source=game.UnitAim(v.actor);
        if(age<a.contact&&!a.cast_fx.empty()){
            const float size=std::min(160.f,a.size)*(.45f+.45f*charge)*boost;
            const bool hand=a.motion==SkillMotion::Cast;
            DrawFx(a.cast_fx,hand?v.source_x:source.first,hand?v.source_y:source.second,size,charge,0,int(100+120*charge));
        }
        if(!a.prop.empty()&&age<a.contact){
            const float consume=a.self?charge:0;
            DrawFx(a.prop,v.source_x+(source.first-v.source_x)*consume,v.source_y-12-std::sin(charge*Pi)*18,std::min(105.f,a.size*.6f)*boost,charge,0,int(255*(1-.45f*consume)));
        }
        for(const auto& shot:game.Projectiles()){
            const float animation=std::fmod(std::max(0.f,age-a.launch)*7,1.f);
            if(a.projectile=="tongue"){
                const auto key="skill_tongue_"+std::to_string(std::min(3,int(animation*4)));const int graph=Image(key);const auto clip=image_bounds_.find(key);
                if(graph>=0&&clip!=image_bounds_.end()){
                    const float dx=shot.x-v.source_x,dy=shot.y-v.source_y,length=std::sqrt(dx*dx+dy*dy);
                    const auto& b=clip->second;
                    DrawRectRotaGraph3F((shot.x+v.source_x)*.5f,(shot.y+v.source_y)*.5f,b.x,b.y,b.w,b.h,b.w*.5f,b.h*.5f,std::max(1.f,length)/b.w,24.f/b.h,std::atan2(dy,dx),graph,TRUE);
                }
            }else if(a.path==FlightPath::Beam){
                const float dx=shot.x-v.source_x,dy=shot.y-v.source_y,length=std::sqrt(dx*dx+dy*dy);
                const int segments=std::max(1,int(length/55));
                for(int i=0;i<=segments;++i){const float t=float(i)/segments;DrawFx(a.projectile,v.source_x+dx*t,v.source_y+dy*t,a.size*.64f,animation,shot.angle,180);}
            }else{
                if(a.projectile=="spear"&&a.impact_fx=="fire_orb")DrawFx("fire_orb",shot.x-std::cos(shot.angle)*45,shot.y-std::sin(shot.angle)*45,a.size*.6f,animation,shot.angle,210);
                DrawFx(a.projectile,shot.x,shot.y,a.size*boost,animation,shot.angle);
            }
        }
        if(v.applied&&age>=a.contact&&!a.impact_fx.empty()){
            const float t=std::clamp((age-a.contact)/std::max(.01f,a.duration-a.contact),0.f,1.f);
            const int opacity=int(255*std::min(1.f,(1-t)*2.5f));
            for(const auto& target:v.targets){
                const bool ground=a.motion==SkillMotion::Stomp||a.impact_fx=="summon"||a.path==FlightPath::Ground;
                const int bursts=a.path==FlightPath::None?std::min(4,a.count):1;
                for(int i=0;i<bursts;++i){
                    const float offset=bursts>1?(i-(bursts-1)*.5f)*a.spread:0;
                    DrawFx(a.impact_fx,target.x+offset,ground?304:target.y,a.size*boost*(1+.22f*t),std::clamp(t*1.15f,0.f,1.f),a.impact_fx=="slash"&&v.enemy?Pi:0,opacity);
                }
            }
        }
    }
    for(const auto& fx:game.FloatingEffects()){
        const float t=fx.age/.65f;
        DrawFx(fx.effect,fx.x,fx.y-fx.age*20,fx.size*(.8f+.3f*t),t,0,int(220*std::min(1.f,(1-t)*2)));
    }
}
}
