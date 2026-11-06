#pragma once
#include <map>
#include <string>
#include <vector>

namespace undergod {
struct Config;
enum class SkillMotion { Still,Thrust,Slash,Peck,Stomp,Dash,Pounce,Swoop,Guard,Cast,Roar,Charge };
enum class FlightPath { None,Straight,Arc,Fan,Rain,Return,Helix,Drain,Orbit,Ground,Beam };
struct SkillAnimation {
    std::string id,pose="cast",cast_fx,projectile,impact_fx,ward,prop;
    SkillMotion motion=SkillMotion::Cast;
    FlightPath path=FlightPath::None;
    float launch=.10f,contact=.32f,duration=.70f,size=100,spread=24,arc=0,lunge=1;
    int count=1;
    bool self=false;
};
struct BodyAction { std::string actor,pose;float emitter_x=400,emitter_y=240; };
struct AnimationLibrary {
    std::map<std::string,SkillAnimation> profiles;
    std::vector<BodyAction> bodies;
    std::vector<std::string> effects;
};
struct SkillTarget { int unit=-1;float x=0,y=0; };
struct ActionVisual {
    bool active=false,enemy=false,applied=false,amplified=false;
    int actor=-1;
    float age=0,source_x=0,source_y=0;
    std::string card_id,pose;
    SkillAnimation animation;
    std::vector<SkillTarget> targets;
};
struct ActorReaction {
    float hit_age=10,guard_age=10,break_age=10,spawn_age=10;
    int last_block=0;
    std::string ward;
};
struct FloatingFx { std::string effect;float x=0,y=0,age=0,size=100; };
struct ProjectileSample { float x=0,y=0,angle=0,progress=0;int target=-1;bool returning=false; };
void LoadSkillAnimations(Config& config,const std::string& directory);
const SkillAnimation& FindSkillAnimation(const Config& config,const std::string& id);
}
