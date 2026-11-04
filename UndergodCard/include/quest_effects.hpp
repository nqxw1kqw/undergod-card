#pragma once
#include "quest_status.hpp"

namespace undergod {
enum class EffectOp { Damage, Block, Heal, Apply, Draw, Energy };
enum class EffectTarget { Self, Enemy };

// `times` repeats the effect. Strength is added per hit, so two hits of 4 with
// Strength 2 deal more than one hit of 8.
struct Effect {
    EffectOp op=EffectOp::Damage;
    EffectTarget target=EffectTarget::Enemy;
    StatusId status=StatusId::Poison;
    int amount=0;
    int times=1;
};

inline const char* RarityLabel(const std::string& rarity){
    if(rarity=="rare") return "Hiếm";
    if(rarity=="uncommon") return "Phi thường";
    if(rarity=="common") return "Thường";
    if(rarity=="starter") return "Cơ bản";
    return "";
}
}
