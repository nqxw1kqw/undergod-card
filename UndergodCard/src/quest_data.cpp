#include "quest_core.hpp"
#include "quest_timeline.hpp"
#include "../third_party/nlohmann/json.hpp"
#include <fstream>
#include <set>
#include <cmath>

namespace undergod {
namespace {
using Json=nlohmann::json;
Json Read(const std::string& path){std::ifstream f(path);if(!f)throw std::runtime_error("Cannot open "+path);Json data;f>>data;return data;}
bool Ident(const std::string& id){return !id.empty()&&id.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_")==std::string::npos;}
StatusId ParseStatus(const std::string& name){
    if(name=="block")return StatusId::Block;
    if(name=="poison")return StatusId::Poison;
    if(name=="weak")return StatusId::Weak;
    if(name=="vulnerable")return StatusId::Vulnerable;
    if(name=="strength")return StatusId::Strength;
    throw std::runtime_error("Unknown status: "+name);
}
NodeKind ParseKind(const std::string& name){
    if(name=="combat")return NodeKind::Combat;
    if(name=="elite")return NodeKind::Elite;
    if(name=="event")return NodeKind::Event;
    if(name=="shop")return NodeKind::Shop;
    if(name=="rest")return NodeKind::Rest;
    if(name=="boss")return NodeKind::Boss;
    throw std::runtime_error("Unknown node kind: "+name);
}
Effect ParseEffect(const Json& j){
    Effect effect;const std::string op=j.at("op");
    if(op=="damage")effect.op=EffectOp::Damage;
    else if(op=="block")effect.op=EffectOp::Block;
    else if(op=="heal")effect.op=EffectOp::Heal;
    else if(op=="apply")effect.op=EffectOp::Apply;
    else if(op=="draw")effect.op=EffectOp::Draw;
    else if(op=="energy")effect.op=EffectOp::Energy;
    else throw std::runtime_error("Unknown effect: "+op);
    effect.amount=j.at("amount");
    effect.times=j.value("times",1);
    const std::string target=j.value("target",effect.op==EffectOp::Damage||effect.op==EffectOp::Apply?"enemy":"self");
    if(target=="enemy")effect.target=EffectTarget::Enemy;
    else if(target=="self")effect.target=EffectTarget::Self;
    else throw std::runtime_error("Unknown effect target");
    if(effect.op==EffectOp::Apply)effect.status=ParseStatus(j.at("status"));
    if(effect.amount<0||effect.times<1||effect.times>8)throw std::runtime_error("Invalid effect magnitude");
    return effect;
}
void Summarize(CardDef& card){
    card.damage=card.block=card.heal=card.poison=card.strength=card.weak=card.vulnerable=card.cards_drawn=card.energy_gain=0;
    for(const auto& effect:card.effects){
        const int total=effect.amount*std::max(1,effect.times);
        if(effect.op==EffectOp::Damage&&effect.target==EffectTarget::Enemy)card.damage+=total;
        else if(effect.op==EffectOp::Block&&effect.target==EffectTarget::Self)card.block+=total;
        else if(effect.op==EffectOp::Heal&&effect.target==EffectTarget::Self)card.heal+=total;
        else if(effect.op==EffectOp::Apply&&effect.target==EffectTarget::Enemy&&effect.status==StatusId::Poison)card.poison+=total;
        else if(effect.op==EffectOp::Apply&&effect.target==EffectTarget::Self&&effect.status==StatusId::Strength)card.strength+=total;
        else if(effect.op==EffectOp::Apply&&effect.target==EffectTarget::Enemy&&effect.status==StatusId::Weak)card.weak+=total;
        else if(effect.op==EffectOp::Apply&&effect.target==EffectTarget::Enemy&&effect.status==StatusId::Vulnerable)card.vulnerable+=total;
        else if(effect.op==EffectOp::Draw)card.cards_drawn+=total;
        else if(effect.op==EffectOp::Energy&&effect.target==EffectTarget::Self)card.energy_gain+=total;
    }
}
std::vector<IntentPattern> ParsePatterns(const Json& list){
    std::vector<IntentPattern> patterns;
    for(const auto& j:list){
        IntentPattern pattern;const std::string kind=j.at("kind");
        if(kind=="attack")pattern.kind=IntentKind::Attack;
        else if(kind=="defend"||kind=="block")pattern.kind=IntentKind::Defend;
        else if(kind=="buff")pattern.kind=IntentKind::Buff;
        else if(kind=="debuff")pattern.kind=IntentKind::Debuff;
        else throw std::runtime_error("Unknown intent kind: "+kind);
        pattern.min_damage=j.value("minDamage",0);
        pattern.max_damage=j.value("maxDamage",pattern.min_damage);
        pattern.block=j.value("block",0);
        pattern.weight=j.value("weight",1);
        pattern.status_amount=j.value("amount",0);
        if(j.contains("status"))pattern.status=ParseStatus(j.at("status"));
        if(pattern.weight<1||pattern.min_damage<0||pattern.max_damage<pattern.min_damage||pattern.block<0||pattern.status_amount<0)throw std::runtime_error("Invalid intent pattern");
        if(pattern.kind==IntentKind::Attack&&pattern.max_damage<1)throw std::runtime_error("Attack pattern needs damage");
        if((pattern.kind==IntentKind::Buff||pattern.kind==IntentKind::Debuff)&&pattern.status_amount<1&&pattern.block<1&&pattern.max_damage<1)throw std::runtime_error("Empty status pattern");
        patterns.push_back(pattern);
    }
    if(patterns.empty())throw std::runtime_error("Intent list is empty");
    return patterns;
}
void ReadLegacyStats(const Json& j,int& min_damage,int& max_damage,int& block,double& chance){
    min_damage=j.at("minDamage");max_damage=j.at("maxDamage");block=j.at("block");chance=j.at("attackChance");
    if(chance<0||chance>1||min_damage<0||max_damage<min_damage||block<0)throw std::runtime_error("Invalid enemy attack stats");
}
PhaseDef ParsePhase(const Json& j){
    PhaseDef phase;
    phase.name=j.at("name");
    phase.hp=j.at("maxHp");
    phase.opening_attack=j.value("openingAttack",true);
    phase.max_guards=j.value("maxConsecutiveGuards",1);
    phase.strength=j.value("strength",0);
    if(phase.name.empty()||phase.hp<1||phase.max_guards<0||phase.strength<0)throw std::runtime_error("Invalid enemy phase");
    if(j.contains("patterns"))phase.patterns=ParsePatterns(j.at("patterns"));
    else ReadLegacyStats(j,phase.min_damage,phase.max_damage,phase.block,phase.attack_chance);
    return phase;
}
EnemyDef ParseEnemy(const Json& j){
    EnemyDef enemy;
    enemy.id=j.at("id");enemy.name=j.at("name");enemy.role=j.value("role","combat");
    enemy.hp=j.at("maxHp");
    enemy.opening_attack=j.value("openingAttack",true);
    enemy.max_guards=j.value("maxConsecutiveGuards",1);
    if(!Ident(enemy.id)||enemy.name.empty()||enemy.hp<1||enemy.max_guards<0)throw std::runtime_error("Invalid enemy definition");
    if(enemy.role!="combat"&&enemy.role!="elite"&&enemy.role!="boss")throw std::runtime_error("Invalid enemy role");
    if(j.contains("patterns"))enemy.patterns=ParsePatterns(j.at("patterns"));
    else ReadLegacyStats(j,enemy.min_damage,enemy.max_damage,enemy.block,enemy.attack_chance);
    if(j.contains("phases"))for(const auto& phase:j.at("phases"))enemy.phases.push_back(ParsePhase(phase));
    return enemy;
}
}
Config LoadConfig(const std::string& directory){
    Config c;const auto b=Read(directory+"/balance.json");
    c.balance={b.at("playerHp"),b.at("energy"),b.at("draw"),b.at("rageMax"),b.at("rageAttack"),b.at("rageBlock"),b.at("ultimateDamage")};
    const auto character=Read(directory+"/characters.json").at(0);c.player_name=character.at("name");c.player_id=character.at("id");
    if(!Ident(c.player_id))throw std::runtime_error("Invalid character ID");
    std::set<std::string> ids;
    for(const auto& j:Read(directory+"/enemies.json")){
        EnemyDef enemy=ParseEnemy(j);
        if(!ids.insert(enemy.id).second)throw std::runtime_error("Duplicate enemy id");
        c.bestiary.push_back(std::move(enemy));
    }
    if(c.bestiary.empty())throw std::runtime_error("Enemy list is empty");
    const auto route=Read(directory+"/run.json");
    c.reward_choices=route.value("rewardChoices",3);
    for(const auto& node:route.at("nodes")){
        RouteNode entry;entry.kind=ParseKind(node.at("kind"));entry.enemy_id=node.value("enemy","");
        if(IsFightNode(entry.kind)){
            if(!Ident(entry.enemy_id))throw std::runtime_error("Fight node needs an enemy id");
            bool found=false;for(const auto& enemy:c.bestiary)found=found||enemy.id==entry.enemy_id;
            if(!found)throw std::runtime_error("Route enemy was not defined: "+entry.enemy_id);
        }
        c.route.push_back(entry);
    }
    if(c.route.empty()||!IsFightNode(c.route.front().kind))throw std::runtime_error("Run route must start with a fight");
    for(const auto& enemy:c.bestiary)if(enemy.id==c.route.front().enemy_id)c.enemy=enemy;
    ids.clear();int copies=0,rewardable=0;
    for(const auto& j:Read(directory+"/cards.json")){
        CardDef card;
        card.id=j.at("id");card.name=j.at("name");card.description=j.at("description");card.cost=j.at("cost");card.copies=j.value("copies",0);
        card.rarity=j.value("rarity",card.copies>0?"starter":"common");
        if(j.contains("keywords"))for(const auto& word:j.at("keywords"))card.keywords.push_back(word.get<std::string>());
        if(j.contains("effects"))for(const auto& effect:j.at("effects"))card.effects.push_back(ParseEffect(effect));
        else{
            const int damage=j.value("damage",0),block=j.value("block",0),heal=j.value("heal",0);
            if(damage<0||block<0||heal<0)throw std::runtime_error("Invalid card definition");
            if(damage>0)card.effects.push_back({EffectOp::Damage,EffectTarget::Enemy,StatusId::Poison,damage,1});
            if(block>0)card.effects.push_back({EffectOp::Block,EffectTarget::Self,StatusId::Block,block,1});
            if(heal>0)card.effects.push_back({EffectOp::Heal,EffectTarget::Self,StatusId::Poison,heal,1});
        }
        Summarize(card);
        const bool useful=card.damage||card.block||card.heal||card.poison||card.strength||card.weak||card.vulnerable||card.cards_drawn||card.energy_gain;
        if(!Ident(card.id)||!ids.insert(card.id).second||card.name.empty()||card.cost<0||card.cost>10||card.copies<0||!useful)throw std::runtime_error("Invalid card definition");
        if(card.rarity!="starter"&&card.rarity!="common"&&card.rarity!="uncommon"&&card.rarity!="rare")throw std::runtime_error("Invalid card rarity");
        if(card.keywords.size()>4)throw std::runtime_error("Too many keywords");
        for(const auto& word:card.keywords)if(word.empty())throw std::runtime_error("Empty keyword");
        if(card.rarity!="starter"&&card.copies==0)++rewardable;
        copies+=card.copies;c.cards.push_back(std::move(card));
    }
    if(copies<1||copies>1000||rewardable<c.reward_choices||c.reward_choices<1||c.reward_choices>5||c.balance.player_hp<1||c.enemy.hp<1||c.balance.draw<1||c.balance.draw>10||c.balance.energy<1||c.balance.rage_max<1||c.balance.rage_attack<0||c.balance.rage_block<0||c.balance.ultimate_damage<1)throw std::runtime_error("Invalid balance configuration");
    return c;
}
Timeline LoadTimeline(const std::string& file){
    const auto j=Read(file);Timeline timeline;timeline.skippable=j.value("skippable",true);int impacts=0,ends=0,falls=0;float impact_time=0,end_time=0,fall_end=0;
    for(const auto& s:j.at("steps")){
        Step step;step.t=s.at("t");const std::string action=s.at("action");
        if(action=="dimScreen"){step.cue=Cue::Dim;step.value=s.at("alpha");}
        else if(action=="cameraZoom"){step.cue=Cue::Zoom;step.value=s.at("to");step.duration=s.at("duration");}
        else if(action=="playSprite"){
            step.cue=Cue::Pose;const std::string target=s.at("target");if(target!="player"&&target!="enemy")throw std::runtime_error("Unknown cinematic actor");step.enemy=target=="enemy";step.pose=s.at("anim");
            if(step.pose!="idle"&&step.pose!="attack"&&step.pose!="hit"&&step.pose!="defend"&&step.pose!="ult_charge"&&step.pose!="ult_throw")throw std::runtime_error("Unknown cinematic pose");
        }
        else if(action=="spawnVFX"){
            step.cue=Cue::Vfx;step.value=s.at("frames");const std::string target=s.at("at");if(target!="player"&&target!="enemy")throw std::runtime_error("Unknown VFX target");step.enemy=target=="enemy";
            if(s.at("sheet")!="vfx/spear_impact.png"||step.value<1||step.value>4)throw std::runtime_error("Unsupported VFX sheet or frame count");
        }
        else if(action=="moveSpear"){
            step.cue=Cue::Spear;const std::string phase=s.at("phase");if(phase!="rise"&&phase!="fall")throw std::runtime_error("Invalid spear phase");step.falling=phase=="fall";
            step.duration=s.at("duration");step.from_x=s.at("from").at(0);step.from_y=s.at("from").at(1);step.to_x=s.at("to").at(0);step.to_y=s.at("to").at(1);
            if(step.duration<=0)throw std::runtime_error("Spear movement needs a positive duration");
            if(step.falling){++falls;fall_end=step.t+step.duration;}
        }
        else if(action=="shake"){step.cue=Cue::Shake;step.value=s.at("intensity");step.duration=s.at("duration");}
        else if(action=="impact"){if(s.at("callback")!="applyDamage")throw std::runtime_error("Unknown cinematic callback");step.cue=Cue::Impact;++impacts;impact_time=step.t;}
        else if(action=="flashWhite"){step.cue=Cue::Flash;step.duration=s.at("duration");}
        else if(action=="end"){step.cue=Cue::End;++ends;end_time=step.t;}
        else throw std::runtime_error("Unknown cinematic action: "+action);
        if(step.t<0||step.duration<0)throw std::runtime_error("Invalid cinematic timing");
        timeline.steps.push_back(step);
    }
    if(impacts!=1||ends!=1||impact_time>end_time)throw std::runtime_error("Cinematic needs one impact before end");
    if(falls!=1||std::abs(fall_end-impact_time)>.1f)throw std::runtime_error("Spear contact must match damage timing");
    return timeline;
}
}
