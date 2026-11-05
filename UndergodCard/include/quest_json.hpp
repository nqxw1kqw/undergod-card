#pragma once
// Persistence-only adapters; the simulation headers do not depend on JSON.
#include "quest_run.hpp"
#include "../third_party/nlohmann/json.hpp"
namespace undergod {
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(StatusSet,poison,weak,vulnerable,strength)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Fighter,hp,max_hp,block,status)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Effect,op,target,status,amount,times,pierce,ranged)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CombatMods,bonus_damage,bonus_block,combat_start,turn_start,class_id,ultimate,energy,draw,retain_block,first_attack,third_attack,ultimate_bonus,ultimate_discount,ultimate_vulnerable,ultimate_hits,spear_bonus,poison_bonus,max_mana,ultimate_damage,enemy_hp_percent,enemy_damage,mana,revive_spent,preview,comet)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Intent,attack,value,kind,status,status_amount,hits,summon_id,card_id)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EnemyUnit,id,name,sprite,fighter,intent,illusion)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BattleRuntime,target,mana,locked,retain_turns,temp_strength,next_energy,thorns,double_attack,end_block,ult_discount,revives,flight_damage,land_turns,charge_left,constrict_left,doom_left,played_turn,damage_dealt,damage_taken,poison_applied,ultimates,attacks_turn,attack_count,revive_ready,first_attack_used,double_used,airborne,curse_next,doom_triggered,card_multiplier,card_bonus,history,last_turn,slots,enemy_thorns,enemy_end_block,enemy_double,enemy_retain,enemy_revive)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CombatSnapshot,enemy_id,player,enemy,phase,intent,energy,rage,turn,guard_streak,phase_index,shuffle_rng,intent_rng,phase_opening,draw,hand,discard,target_rng,live,adds,exhaust,created)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MapNode,kind,encounter_id,event_id,floor,column,reward_gold,visited,next)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ShopItem,ware,id,price,sold)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EventChoice,label,outcome,card,relic,potion,heal,damage,gold,max_hp,upgrade,action,amount,chance)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RunState,seed,index,node,hp,max_hp,rage,gold,potion_slots,class_id,event_title,event_text,deck,relics,potions,rewards,event_choices,map,options,shop,place,pending_reward,victory,defeat,fight_resolved,rest_used,act,challenge,trial_limit,reward_multiplier,fog_until,sequence_gold,advance_act,reward_from_event,sequence_relic,meta_claimed,phoenix_spent,run_id,active_encounter_id,selection,last_outcome,skills,relic_rewards,sequence,defeated_bosses,bonuses,metrics)
}
