#include "quest_core.hpp"
#include <type_traits>

namespace undergod {
namespace {
// Exact byte key, not a hash: comparison cannot reuse a colliding state.
// Configuration/encounter definitions are fixed between controller resets.
struct Key {
    std::string& out;
    template<class T,std::enable_if_t<std::is_arithmetic_v<T>||std::is_enum_v<T>,int> =0>
    void Put(const T& v){out.append(reinterpret_cast<const char*>(&v),sizeof(v));}
    void Put(const std::string& s){Put(s.size());out.append(s);}
    template<class T>void Put(const std::vector<T>& v){Put(v.size());for(const auto& item:v)Put(item);}
    void Put(const std::vector<bool>& v){Put(v.size());for(bool item:v)Put(item);}
    template<class... T>void All(const T&... v){(Put(v),...);}
    void Put(const StatusSet& v){All(v.poison,v.weak,v.vulnerable,v.strength);}
    void Put(const Fighter& v){All(v.hp,v.max_hp,v.block,v.status);}
    void Put(const Intent& v){All(v.attack,v.value,v.kind,v.status,v.status_amount,v.hits,v.summon_id,v.card_id,v.deck_card_id);}
    void Put(const Effect& v){All(v.op,v.target,v.status,v.amount,v.times,v.pierce,v.ranged);}
    void Put(const CombatMods& v){All(v.bonus_damage,v.bonus_block,v.combat_start,v.turn_start,v.class_id,v.ultimate,v.energy,v.draw,v.retain_block,v.first_attack,v.third_attack,v.ultimate_bonus,v.ultimate_discount,v.ultimate_vulnerable,v.ultimate_hits,v.spear_bonus,v.poison_bonus,v.max_mana,v.ultimate_damage,v.enemy_hp_percent,v.enemy_damage,v.mana,v.revive_spent,v.preview,v.comet);}
    void Put(const EnemyUnit& v){All(v.id,v.name,v.sprite,v.fighter,v.intent,v.illusion);}
    void Put(const StarPlayerCard& v){All(v.hand_index,v.target,v.id);}
    void Put(const StarEnemyCard& v){All(v.actor,v.action,v.wait);}
    void Put(const TurnStarPlan& v){All(v.enabled,v.resolving,v.cursor,v.player,v.enemy);}
    void Put(const BattleRuntime& v){All(v.target,v.mana,v.locked,v.retain_turns,v.temp_strength,v.next_energy,v.thorns,v.double_attack,v.end_block,v.ult_discount,v.revives,v.flight_damage,v.land_turns,v.charge_left,v.constrict_left,v.doom_left,v.played_turn,v.damage_dealt,v.damage_taken,v.poison_applied,v.ultimates,v.attacks_turn,v.attack_count,v.enemy_thorns,v.enemy_end_block,v.enemy_double,v.enemy_retain,v.enemy_revive,v.revive_ready,v.first_attack_used,v.double_used,v.airborne,v.curse_next,v.doom_triggered,v.card_multiplier,v.card_bonus,v.history,v.last_turn,v.slots);}
};
}
void Combat::AppendForecastKey(std::string& out)const{
    Key k{out};k.All(enemy_id_,role_,player,enemy,phase,intent,energy,rage,turn,draw,hand,discard,live,adds,exhaust,created,stars,mods_,shuffle_rng_.State(),intent_rng_.State(),target_rng_.State(),ultimate_pending_,phase_opening_,guard_streak_,phase_index_,attacker_,reserved_play_,star_phase_start_,star_curses_,star_enemy_started_,star_player_decay_,star_enemy_decay_,star_add_decay_);
}
}
