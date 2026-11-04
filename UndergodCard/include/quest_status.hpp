#pragma once
#include <algorithm>
#include <string>

namespace undergod {
enum class StatusId { Block, Poison, Weak, Vulnerable, Strength };

struct StatusSet {
    int poison=0, weak=0, vulnerable=0, strength=0;
    int Get(StatusId id) const {
        switch(id){
        case StatusId::Poison: return poison;
        case StatusId::Weak: return weak;
        case StatusId::Vulnerable: return vulnerable;
        case StatusId::Strength: return strength;
        case StatusId::Block: return 0;
        }
        return 0;
    }
    void Add(StatusId id, int amount){
        if(amount==0) return;
        switch(id){
        case StatusId::Poison: poison=std::max(0,poison+amount); break;
        case StatusId::Weak: weak=std::max(0,weak+amount); break;
        case StatusId::Vulnerable: vulnerable=std::max(0,vulnerable+amount); break;
        case StatusId::Strength: strength=std::max(0,strength+amount); break;
        case StatusId::Block: break;
        }
    }
    void DecayDebuffs(){if(weak>0)--weak; if(vulnerable>0)--vulnerable;}
};

inline const char* StatusName(StatusId id){
    switch(id){
    case StatusId::Block: return "Giáp";
    case StatusId::Poison: return "Độc";
    case StatusId::Weak: return "Suy Yếu";
    case StatusId::Vulnerable: return "Dễ Thương";
    case StatusId::Strength: return "Sức Mạnh";
    }
    return "";
}

// Strength, then Weak (×3/4), then Vulnerable (×3/2). No modifiers leaves the base unchanged.
inline int ScaleDamage(int base, const StatusSet& source, const StatusSet& target){
    int damage=base+source.strength;
    if(damage<0) damage=0;
    if(source.weak>0) damage=damage*3/4;
    if(target.vulnerable>0) damage=damage*3/2;
    return damage;
}
}
