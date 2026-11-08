#pragma once
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace undergod {
struct CardDef { std::string id,name,description; int cost=0,damage=0,block=0,heal=0,copies=0; };
struct Balance { int player_hp=50,energy=3,draw=5,rage_max=100,rage_attack=8,rage_block=5,ultimate_damage=40; };
struct EnemyDef { std::string name="Ếch Vương"; int hp=120,min_damage=8,max_damage=13,block=6; double attack_chance=.7; bool opening_attack=true; int max_guards=1; int stage_threshold_percent=60,gentle_reduction=2,gentle_charge=14,challenge_charge=34; };
enum class Difficulty { Gentle,Standard,Challenge };
inline const char* DifficultyName(Difficulty d){return d==Difficulty::Gentle?"gentle":(d==Difficulty::Challenge?"challenge":"standard");}
struct Config { Balance balance; EnemyDef enemy; std::string player_name="Gà Chiến Binh"; std::vector<CardDef> cards; std::string player_id="chick"; };
Config LoadConfig(const std::string& directory="data");

// Mulberry32, matching the web prototype's unsigned JS operations exactly.
class Rng {
    uint32_t value_=0;
public:
    void Seed(uint32_t seed){value_=seed;}
    double Next(){uint32_t t=(value_+=0x6D2B79F5u);t=(t^(t>>15))*(t|1u);t^=t+((t^(t>>7))*(t|61u));return double(t^(t>>14))/4294967296.0;}
};
struct Fighter { int hp=0,max_hp=0,block=0; };
struct DamageResult { int lost=0,blocked=0; };
inline DamageResult ResolveDamage(Fighter& target,int damage){
    const int blocked=std::min(target.block,std::max(0,damage));target.block-=blocked;
    const int lost=std::min(target.hp,std::max(0,damage-blocked));target.hp-=lost;return {lost,blocked};
}
enum class Phase { Player,Enemy,CheckEnd,Cinematic,Won,Lost };
enum class EventType { CardPlayed,Damage,Block,Heal,TurnEnd,Ultimate };
struct Event { EventType type; bool enemy=false; int amount=0,blocked=0; };
struct Intent { bool attack=false; int value=0; bool prepare=false,charged=false; };
class Combat {
    Rng shuffle_rng_,intent_rng_;
    bool ultimate_pending_=false;
    int guard_streak_=0,pattern_step_=0;
    void Shuffle(std::vector<std::string>& pile){for(int i=int(pile.size())-1;i>0;--i)std::swap(pile[i],pile[int(shuffle_rng_.Next()*(i+1))]);}
    void RollIntent(){
        const int threshold=difficulty==Difficulty::Standard?0:config.enemy.stage_threshold_percent;
        const int charge=difficulty==Difficulty::Gentle?config.enemy.gentle_charge:config.enemy.challenge_charge;
        if(!stage_entries&&threshold&&enemy.hp*100<=enemy.max_hp*threshold){stage_entries=1;pattern_step_=0;}
        if(stage_entries){
            const int step=pattern_step_++%3;
            if(step==0){intent={false,charge,true,false};return;}
            if(step==1){intent={true,charge,false,true};guard_streak_=0;return;}
        }
        intent={};
        const bool rolled_attack=intent_rng_.Next()<config.enemy.attack_chance;
        intent.attack=rolled_attack||(turn==0&&config.enemy.opening_attack)||guard_streak_>=config.enemy.max_guards;
        guard_streak_=intent.attack?0:guard_streak_+1;
        intent.value=intent.attack?config.enemy.min_damage+int(intent_rng_.Next()*(config.enemy.max_damage-config.enemy.min_damage+1)):config.enemy.block;
        if(difficulty==Difficulty::Gentle&&intent.attack)intent.value=std::max(1,intent.value-config.enemy.gentle_reduction);
    }
    void BeginPlayer(){++turn;phase=Phase::Player;player.block=0;energy=config.balance.energy;Draw(config.balance.draw);}
    void CheckEnd(){if(player.hp<=0)phase=Phase::Lost;else if(enemy.hp<=0)phase=Phase::Won;}
    void Hit(bool target_enemy,int damage){auto r=ResolveDamage(target_enemy?enemy:player,damage);events.push_back({EventType::Damage,target_enemy,r.lost,r.blocked});}
public:
    Difficulty difficulty=Difficulty::Standard;
    int stage_entries=0;
    Config config;
    Fighter player,enemy;
    Phase phase=Phase::Player;
    Intent intent;
    int energy=0,rage=0,turn=0;
    std::vector<std::string> draw,hand,discard;
    std::vector<Event> events;
    explicit Combat(Config c):config(std::move(c)){Reset();}
    void Reset(uint32_t seed=1337){
        if(difficulty!=Difficulty::Gentle&&difficulty!=Difficulty::Standard&&difficulty!=Difficulty::Challenge)throw std::runtime_error("Invalid difficulty");
        const auto& e=config.enemy;
        if(e.stage_threshold_percent<1||e.stage_threshold_percent>99||e.gentle_reduction<0||e.gentle_reduction>e.min_damage||e.gentle_charge<1||e.gentle_charge>100||e.challenge_charge<1||e.challenge_charge>100)throw std::runtime_error("Invalid difficulty profile");
        shuffle_rng_.Seed(seed);intent_rng_.Seed(seed^0x19ABCu);ultimate_pending_=false;guard_streak_=pattern_step_=stage_entries=0;
        player={config.balance.player_hp,config.balance.player_hp,0};enemy={config.enemy.hp,config.enemy.hp,0};
        energy=rage=turn=0;draw.clear();hand.clear();discard.clear();events.clear();
        for(const auto& c:config.cards)for(int i=0;i<c.copies;++i)draw.push_back(c.id);
        Shuffle(draw);RollIntent();BeginPlayer();
    }
    const CardDef& Definition(const std::string& id) const {for(const auto& c:config.cards)if(c.id==id)return c;throw std::runtime_error("Unknown card: "+id);}
    void Draw(int count){for(int i=0;i<count;++i){if(draw.empty()){draw=std::move(discard);discard.clear();Shuffle(draw);}if(draw.empty())break;hand.push_back(draw.back());draw.pop_back();}}
    int RageGain(const CardDef& c)const{return std::min(config.balance.rage_max-rage,(c.damage>0?config.balance.rage_attack:0)+(c.block>0?config.balance.rage_block:0));}
    int EffectiveHeal(const CardDef& c)const{return std::min(c.heal,player.max_hp-player.hp);}
    bool CanPlay(int index) const {return phase==Phase::Player && index>=0 && index<int(hand.size()) && energy>=Definition(hand[index]).cost;}
    bool Play(int index){
        if(!CanPlay(index))return false;
        const auto id=hand[index];const auto& c=Definition(id);hand.erase(hand.begin()+index);discard.push_back(id);energy-=c.cost;
        events.push_back({EventType::CardPlayed});
        if(c.damage>0){Hit(true,c.damage);rage=std::min(config.balance.rage_max,rage+config.balance.rage_attack);}
        if(c.block>0){player.block+=c.block;rage=std::min(config.balance.rage_max,rage+config.balance.rage_block);events.push_back({EventType::Block,false,c.block});}
        if(c.heal>0){int amount=std::min(c.heal,player.max_hp-player.hp);player.hp+=amount;events.push_back({EventType::Heal,false,amount});}
        CheckEnd();return true;
    }
    bool EndPlayer(){if(phase!=Phase::Player)return false;discard.insert(discard.end(),hand.begin(),hand.end());hand.clear();phase=Phase::Enemy;events.push_back({EventType::TurnEnd});return true;}
    bool ActEnemy(){
        if(phase!=Phase::Enemy)return false;
        enemy.block=0;
        if(intent.attack)Hit(false,intent.value);else if(!intent.prepare){enemy.block=intent.value;events.push_back({EventType::Block,true,intent.value});}
        events.push_back({EventType::TurnEnd,true});phase=Phase::CheckEnd;CheckEnd();
        if(phase==Phase::CheckEnd){RollIntent();BeginPlayer();}return true;
    }
    bool StartUltimate(){if(phase!=Phase::Player || rage<config.balance.rage_max)return false;rage=0;phase=Phase::Cinematic;ultimate_pending_=true;events.push_back({EventType::Ultimate});return true;}
    bool UltimateImpact(){if(!ultimate_pending_)return false;ultimate_pending_=false;Hit(true,config.balance.ultimate_damage);CheckEnd();return true;}
    void FinishUltimate(){if(phase!=Phase::Cinematic)return;UltimateImpact();if(phase==Phase::Cinematic)phase=Phase::Player;}
};
} // namespace undergod
