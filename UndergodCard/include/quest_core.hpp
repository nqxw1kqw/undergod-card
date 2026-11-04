#pragma once
#include "quest_effects.hpp"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include <array>

namespace undergod {
enum class NodeKind { Combat, Elite, Event, Shop, Rest, Boss };

struct CardDef {
    std::string id,name,description,rarity="common";
    int cost=0,damage=0,block=0,heal=0,copies=0;
    int poison=0,strength=0,weak=0,vulnerable=0,cards_drawn=0,energy_gain=0;
    std::vector<std::string> keywords;
    std::vector<Effect> effects;
    bool upgraded=false;
    std::string upgrade_id;
};

struct Balance { int player_hp=50,energy=3,draw=5,rage_max=100,rage_attack=8,rage_block=5,ultimate_damage=40; };

enum class IntentKind { Attack, Defend, Buff, Debuff };

struct IntentPattern {
    IntentKind kind=IntentKind::Attack;
    int min_damage=0,max_damage=0,block=0;
    StatusId status=StatusId::Strength;
    int status_amount=0;
    int weight=1;
};

struct PhaseDef {
    std::string name;
    int hp=1;
    int min_damage=0,max_damage=0,block=0;
    double attack_chance=.7;
    bool opening_attack=true;
    int max_guards=1;
    int strength=0;
    std::vector<IntentPattern> patterns;
};

struct EnemyDef {
    std::string id="frog";
    std::string name="Ếch Vương";
    std::string role="combat";
    int hp=120,min_damage=8,max_damage=13,block=6,min_floor=0;
    double attack_chance=.7;
    bool opening_attack=true;
    int max_guards=1;
    std::vector<IntentPattern> patterns;
    std::vector<PhaseDef> phases;
};

struct RouteNode { NodeKind kind=NodeKind::Combat; std::string enemy_id; };

struct EventChoice {
    std::string label,outcome,card,relic,potion;
    int heal=0,damage=0,gold=0,max_hp=0;
    bool upgrade=false;
};

struct EventDef { std::string id,title,text; std::vector<EventChoice> choices; };

struct RelicDef {
    std::string id,name,description;
    int price=50,bonus_damage=0,bonus_block=0,gold_bonus=0,rest_bonus=0,shop_discount=0,max_hp=0,potion_slot=0;
    std::vector<Effect> combat_start,turn_start;
};

struct PotionDef { std::string id,name,description; int price=40; std::vector<Effect> effects; };

struct Config {
    Balance balance;
    EnemyDef enemy;
    std::string player_name="Gà Chiến Binh";
    std::vector<CardDef> cards;
    std::string player_id="chick";
    std::vector<EnemyDef> bestiary;
    std::vector<RouteNode> route;
    int reward_choices=3;
    int gold_start=50,heal_percent=30,remove_price=75,potion_slots=3,map_columns=3,map_floors=12;
    int elite_min_floor=2,combat_gold_min=15,combat_gold_max=25,elite_gold_min=30,elite_gold_max=40;
    std::array<int,5> map_weights={45,12,22,8,13};
    std::string boss_id="frog",elite_id="elite_frog";
    std::vector<std::string> combat_ids;
    std::vector<std::string> elite_ids;
    std::vector<EventDef> events;
    std::vector<RelicDef> relics;
    std::vector<PotionDef> potions;
};

Config LoadConfig(const std::string& directory="data");
inline std::vector<std::string> StartingDeck(const Config& config){
    std::vector<std::string> deck;
    for(const auto& card:config.cards) for(int i=0;i<card.copies;++i) deck.push_back(card.id);
    return deck;
}
inline bool IsFightNode(NodeKind kind){return kind==NodeKind::Combat||kind==NodeKind::Elite||kind==NodeKind::Boss;}
inline const char* NodeKindName(NodeKind kind){
    switch(kind){
    case NodeKind::Combat: return "Trận";
    case NodeKind::Elite: return "Tinh anh";
    case NodeKind::Event: return "Sự kiện";
    case NodeKind::Shop: return "Cửa hàng";
    case NodeKind::Rest: return "Nghỉ";
    case NodeKind::Boss: return "Trùm";
    }
    return "";
}

class Rng {
    uint32_t value_=0;
public:
    void Seed(uint32_t seed){value_=seed;}
    uint32_t State()const{return value_;}
    double Next(){uint32_t t=(value_+=0x6D2B79F5u);t=(t^(t>>15))*(t|1u);t^=t+((t^(t>>7))*(t|61u));return double(t^(t>>14))/4294967296.0;}
};

struct Fighter { int hp=0,max_hp=0,block=0; StatusSet status; };
struct DamageResult { int lost=0,blocked=0; };
inline DamageResult ResolveDamage(Fighter& target,int damage,bool pierce=false){
    const int blocked=pierce?0:std::min(target.block,std::max(0,damage));
    if(!pierce) target.block-=blocked;
    const int lost=std::min(target.hp,std::max(0,damage-blocked));
    target.hp-=lost;
    return {lost,blocked};
}
inline int StatusOf(const Fighter& fighter,StatusId id){return id==StatusId::Block?fighter.block:fighter.status.Get(id);}
inline void AddStatusStack(Fighter& fighter,StatusId id,int amount){
    if(id==StatusId::Block){fighter.block=std::max(0,fighter.block+amount);return;}
    fighter.status.Add(id,amount);
}

enum class Phase { Player,Enemy,CheckEnd,Cinematic,Won,Lost };
enum class EventType { CardPlayed,Damage,Block,Heal,TurnEnd,Ultimate,Status,Phase };
struct Event { EventType type; bool enemy=false; int amount=0,blocked=0; StatusId status=StatusId::Poison; };
struct Intent {
    bool attack=false;
    int value=0;
    IntentKind kind=IntentKind::Attack;
    StatusId status=StatusId::Strength;
    int status_amount=0;
};

struct CombatSnapshot {
    std::string enemy_id;
    Fighter player,enemy;
    Phase phase=Phase::Player;
    Intent intent;
    int energy=0,rage=0,turn=1,guard_streak=0,phase_index=0;
    uint32_t shuffle_rng=0,intent_rng=0;
    bool phase_opening=false;
    std::vector<std::string> draw,hand,discard;
};

class Combat {
    Rng shuffle_rng_,intent_rng_;
    bool ultimate_pending_=false,phase_opening_=false;
    int guard_streak_=0,phase_index_=0;
    std::string role_="combat";
    std::string enemy_id_;
    std::vector<PhaseDef> phases_;
    void Shuffle(std::vector<std::string>& pile);
    void RollIntent();
    void BeginPlayer();
    void CheckEnd();
    void Hit(bool target_enemy,int damage,bool pierce=false);
    CombatMods mods_;
    void ApplyOne(const Effect& effect,bool from_card=true);
    void AddStatusEvent(Fighter& fighter,bool enemy_target,StatusId id,int amount);
    bool TickPoison(bool enemy_target,bool& phased);
    void AdvancePhaseIfAny();
    void ResolveIntent();
    const PhaseDef& CurrentPhase() const;
public:
    Config config;
    Fighter player,enemy;
    Phase phase=Phase::Player;
    Intent intent;
    int energy=0,rage=0,turn=0;
    std::vector<std::string> draw,hand,discard;
    std::vector<Event> events;
    explicit Combat(Config c);
    void Reset(uint32_t seed=1337);
    void BeginEncounter(const EnemyDef& enemy_def,uint32_t seed,int hp,int max_hp,int kept_rage,const std::vector<std::string>& deck,CombatMods mods={});
    void Cast(const std::vector<Effect>& effects);
    CombatSnapshot Snapshot()const;
    void Restore(const CombatSnapshot& snapshot,CombatMods mods);
    const CardDef& Definition(const std::string& id) const;
    void Draw(int count);
    bool CanPlay(int index) const;
    bool Play(int index);
    bool EndPlayer();
    bool ActEnemy();
    bool StartUltimate();
    bool UltimateImpact();
    void FinishUltimate();
    int PhaseIndex() const {return phase_index_;}
    int PhaseCount() const {return int(phases_.size());}
    const std::string& PhaseName() const;
    const std::string& Role() const {return role_;}
    int IncomingDamage() const;
    std::string IntentText() const;
    std::string StatusText(const Fighter& fighter) const;
};
}
