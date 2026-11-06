#pragma once
#include "quest_status.hpp"
#include <vector>

namespace undergod {
enum class EffectOp { Damage, Block, Heal, Apply, Draw, Energy, Rage, Retain, Thorns, Revive, DoubleAttack, EndBlock, UltDiscount, TempStrength, NextEnergy };
enum class EffectTarget { Self, Enemy, AllEnemies, RandomEnemy };

// `times` repeats the effect. Strength is added per hit, so two hits of 4 with
// Strength 2 deal more than one hit of 8.
struct Effect {
    EffectOp op=EffectOp::Damage;
    EffectTarget target=EffectTarget::Enemy;
    StatusId status=StatusId::Poison;
    int amount=0;
    int times=1;
    bool pierce=false,ranged=false;
};

struct CombatMods {
    int bonus_damage=0,bonus_block=0;
    std::vector<Effect> combat_start,turn_start;
    std::string class_id="chick",ultimate="spear";
    int energy=0,draw=0,retain_block=0,first_attack=0,third_attack=0,ultimate_bonus=0,ultimate_discount=0,ultimate_vulnerable=0,ultimate_hits=1;
    int spear_bonus=0,poison_bonus=0,max_mana=3;
    int ultimate_damage=0;
    int enemy_hp_percent=100,enemy_damage=0;
    bool mana=false,revive_spent=false,preview=false,comet=false;
};

inline const char* RarityLabel(const std::string& rarity){
    if(rarity=="legendary") return "Huyền thoại";
    if(rarity=="curse") return "Lời nguyền";
    if(rarity=="rare") return "Hiếm";
    if(rarity=="uncommon") return "Phi thường";
    if(rarity=="common") return "Thường";
    if(rarity=="starter") return "Cơ bản";
    return "";
}
}
