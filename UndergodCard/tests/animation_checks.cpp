#include "game.hpp"
#include "quest_json.hpp"
#include <cmath>
#include <set>
#include <iostream>
using namespace undergod;
extern void Check(bool,const char*);
namespace {
void AdvanceSkill(Game& g,float seconds){while(seconds>.00001f){const float n=std::min(.01f,seconds);g.Update(n/g.PlaybackRate());seconds-=n;}}
void Settle(Game& g){for(int i=0;i<2200&&g.Busy();++i)AdvanceSkill(g,.01f);Check(!g.Busy(),"skill action finishes without stalling");}
Intent Idle(){Intent i;i.kind=IntentKind::Defend;i.value=0;return i;}
void Fixture(Game& g,const std::string& card,const std::string& cls="chick",const std::string& boss="tiger"){
    g.StartBossPractice(boss,1337,cls);Settle(g);g.state.player.hp=g.state.player.max_hp=500;g.state.enemy.hp=g.state.enemy.max_hp=500;
    g.state.hand={card};g.state.live.locked=-1;g.state.player.block=g.state.enemy.block=0;
    g.state.stars.enemy={{0,Idle(),true},{0,Idle(),true},{0,Idle(),true}};
}
void Attack(Game& g){g.Play(0);g.QueueWait();g.QueueWait();g.EndTurn();}
void EnemyFirst(Game& g,const Intent& action){g.state.stars.enemy[0]={0,action,false};g.QueueWait();g.QueueWait();g.QueueWait();g.EndTurn();for(int n=0;n<100&&g.ActiveActor()<0;++n)AdvanceSkill(g,.01f);}
}
void AnimationChecks(const Config& config,const Timeline& timeline){
    Check(config.animations.bodies.size()==18&&config.animations.effects.size()==17,"complete generated body and FX libraries");
    for(const auto& c:config.cards){const auto& a=FindSkillAnimation(config,c.id);Check(!a.id.empty()&&a.duration>a.contact,"every playable and upgraded card has a valid skill timeline");}
    for(const auto& c:config.boss_cards){const auto& a=FindSkillAnimation(config,c.id);Check(a.id==c.id,"every boss card has its own animation binding");}
    {
        Game g(config,timeline);Fixture(g,"punch");const int hp=g.state.enemy.hp;Attack(g);AdvanceSkill(g,.30f);
        Check(g.SkillVisual().pose=="thrust"&&g.ActorOffset(false)>500&&g.state.enemy.hp==hp,"spear closes the distance before contact without early damage");
        AdvanceSkill(g,.04f);Check(g.state.enemy.hp<hp&&g.SkillVisual().applied&&g.SkillFrame()==2,"spear contact pose and damage occur together");
        const int after=g.state.enemy.hp;AdvanceSkill(g,.2f);Check(g.state.enemy.hp==after,"one animation applies one atomic card action");Settle(g);
    }
    {
        Game g(config,timeline);Fixture(g,"spellbolt","mage");const int hp=g.state.enemy.hp;const auto rng=g.state.Snapshot().intent_rng;Attack(g);AdvanceSkill(g,.20f);
        const auto first=g.Projectiles();Check(g.SkillVisual().pose=="cast"&&std::abs(g.ActorOffset(false))<.01f&&!first.empty(),"mage casts from home and launches an actual projectile");
        AdvanceSkill(g,.20f);const auto second=g.Projectiles();Check(!second.empty()&&second[0].x>first[0].x+100&&g.state.enemy.hp==hp,"spell flies across the arena before damage");
        Check(g.state.Snapshot().intent_rng==rng,"visual sampling consumes no intent RNG");AdvanceSkill(g,.20f);Check(g.state.enemy.hp<hp&&g.Projectiles().empty(),"spell damage waits for projectile arrival");Settle(g);
    }
    {
        Game g(config,timeline);Fixture(g,"shield");Attack(g);AdvanceSkill(g,.29f);Check(g.UnitBlock(-1)==0,"shield does not exist before the guard contact");AdvanceSkill(g,.05f);
        Check(g.UnitBlock(-1)>0&&g.WardFor(-1)=="ward_blue"&&g.Reaction(-1).guard_age<.04f,"player guard produces a persistent ward with real armor");
        AdvanceSkill(g,.60f);Check(g.UnitBlock(-1)>0&&!g.SkillActor(false),"shield remains visible after its casting action");
        g.state.player.block=0;AdvanceSkill(g,.01f);Check(g.Reaction(-1).break_age<.02f,"losing all armor starts the shield break animation");Settle(g);
    }
    {
        Game g(config,timeline);Fixture(g,"cookie","mage","fox");Intent i;i.kind=IntentKind::Attack;i.attack=true;i.value=8;i.deck_card_id="boss_fox_fire";const int hp=g.state.player.hp;EnemyFirst(g,i);AdvanceSkill(g,.57f);
        auto first=g.Projectiles();Check(g.SkillVisual().pose=="cast"&&std::abs(g.ActorOffset(true))<.01f&&!first.empty(),"fox fireball uses a casting pose and never lunges");AdvanceSkill(g,.15f);auto second=g.Projectiles();
        Check(!second.empty()&&second[0].x<first[0].x-100&&g.state.player.hp==hp,"boss fireball travels toward the player before damage");AdvanceSkill(g,.19f);Check(g.state.player.hp==hp-8&&g.Reaction(-1).hit_age<.06f,"boss projectile arrival triggers player hit reaction");Settle(g);
    }
    {
        Game g(config,timeline);Fixture(g,"punch");Intent i;i.kind=IntentKind::Defend;i.value=15;i.deck_card_id="boss_tiger_guard";EnemyFirst(g,i);AdvanceSkill(g,.48f);
        Check(g.UnitBlock(0)==15&&g.WardFor(0)=="ward_fire"&&g.SkillVisual().pose=="guard","boss gets its own persistent fiery shield and guard pose");AdvanceSkill(g,.72f);Check(g.UnitBlock(0)==15,"boss shield outlives the active guard card");Settle(g);
    }
    {
        Game g(config,timeline);Fixture(g,"punch","ninja","undergod");Intent i;i.kind=IntentKind::Mimic;i.card_id="spellbolt";i.deck_card_id="boss_undergod_mirror";EnemyFirst(g,i);AdvanceSkill(g,.66f);
        Check(g.SkillVisual().card_id=="boss_undergod_mirror"&&g.SkillVisual().animation.projectile=="arcane_orb"&&!g.Projectiles().empty(),"mirror boss copies the selected spell motion while preserving its own card identity");Settle(g);
    }
    {
        Game g(config,timeline);Fixture(g,"meteor_rain","mage","fox");g.state.adds.push_back({"froglet","Frog","froglet",{500,500,0,{}},Idle(),false});g.state.live.slots={0,1};
        const auto before=g.state.Snapshot().intent_rng;Attack(g);AdvanceSkill(g,.4f);const auto targets=g.SkillVisual().targets;
        Check(targets.size()==6&&!g.Projectiles().empty()&&g.state.Snapshot().intent_rng==before,"random multi-projectile preview keeps the real RNG untouched");
        std::vector<int> predicted;for(const auto& t:targets)predicted.push_back(t.unit);Combat copy=g.state;copy.events.clear();copy.ResolveStarAction();std::vector<int> actual;
        for(const auto& e:copy.events)if(e.enemy&&e.type==EventType::Damage)actual.push_back(e.target);
        Check(predicted==actual,"random projectiles point at the actual deterministic damage targets");Settle(g);
    }
    {
        Game g(config,timeline);g.NewRun(1337);g.ChooseNode(0);Settle(g);g.state.enemy.hp=1;g.state.hand={"punch"};g.state.live.locked=-1;g.state.stars.enemy={{0,Idle(),true},{0,Idle(),true},{0,Idle(),true}};
        Attack(g);AdvanceSkill(g,.34f);Check(g.state.phase==Phase::Won&&g.run.place==Place::Fight&&g.Busy()&&g.SkillVisual().active,"fatal strike keeps the stage visible through its impact tail");
        AdvanceSkill(g,.35f);Check(g.run.place!=Place::Fight&&!g.SkillVisual().active,"loot transition happens only after the killing animation finishes");
        g.NewRun();Check(!g.SkillVisual().active&&g.FloatingEffects().empty()&&g.Reaction(-1).break_age>1,"a new run clears all old visual reactions");
    }
    std::string reference;
    for(int speed=1;speed<=3;++speed){
        Game g(config,timeline);Fixture(g,"comet_lance");g.SetSpeed(speed);Attack(g);Settle(g);
        const auto snapshot=nlohmann::json(g.state.Snapshot()).dump();if(reference.empty())reference=snapshot;else Check(snapshot==reference,"x1 x2 x3 preserve identical combat and RNG outcomes");
    }
    std::cout<<"SKILL_ANIMATIONS profiles="<<config.animations.profiles.size()<<" bodies="<<config.animations.bodies.size()<<" effects="<<config.animations.effects.size()<<"\n";
}
