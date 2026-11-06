#pragma once
// Persistence-only adapters; the simulation headers do not depend on JSON.
#include "quest_run.hpp"
#include "../third_party/nlohmann/json.hpp"
namespace undergod {
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(StatusSet,poison,weak,vulnerable,strength)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Fighter,hp,max_hp,block,status)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Effect,op,target,status,amount,times,pierce,ranged)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CombatMods,bonus_damage,bonus_block,combat_start,turn_start,class_id,ultimate,energy,draw,retain_block,first_attack,third_attack,ultimate_bonus,ultimate_discount,ultimate_vulnerable,ultimate_hits,spear_bonus,poison_bonus,max_mana,ultimate_damage,enemy_hp_percent,enemy_damage,mana,revive_spent,preview,comet)
inline void to_json(nlohmann::json& j,const Intent& a){
    j={{"attack",a.attack},{"value",a.value},{"kind",a.kind},{"status",a.status},{"status_amount",a.status_amount},{"hits",a.hits},{"summon_id",a.summon_id},{"card_id",a.card_id},{"deck_card_id",a.deck_card_id}};
}
inline void from_json(const nlohmann::json& j,Intent& a){
    j.at("attack").get_to(a.attack);j.at("value").get_to(a.value);j.at("kind").get_to(a.kind);j.at("status").get_to(a.status);
    j.at("status_amount").get_to(a.status_amount);j.at("hits").get_to(a.hits);j.at("summon_id").get_to(a.summon_id);j.at("card_id").get_to(a.card_id);
    a.deck_card_id=j.value("deck_card_id",std::string{});
}
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EnemyUnit,id,name,sprite,fighter,intent,illusion)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(StarPlayerCard,hand_index,target,id)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(StarEnemyCard,actor,action,wait)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TurnStarPlan,enabled,resolving,cursor,player,enemy)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BattleRuntime,target,mana,locked,retain_turns,temp_strength,next_energy,thorns,double_attack,end_block,ult_discount,revives,flight_damage,land_turns,charge_left,constrict_left,doom_left,played_turn,damage_dealt,damage_taken,poison_applied,ultimates,attacks_turn,attack_count,revive_ready,first_attack_used,double_used,airborne,curse_next,doom_triggered,card_multiplier,card_bonus,history,last_turn,slots,enemy_thorns,enemy_end_block,enemy_double,enemy_retain,enemy_revive)
inline void to_json(nlohmann::json& j,const CombatSnapshot& s){
    j={{"enemy_id",s.enemy_id},{"player",s.player},{"enemy",s.enemy},{"phase",s.phase},{"intent",s.intent},{"energy",s.energy},{"rage",s.rage},{"turn",s.turn},{"guard_streak",s.guard_streak},{"phase_index",s.phase_index},{"shuffle_rng",s.shuffle_rng},{"intent_rng",s.intent_rng},{"phase_opening",s.phase_opening},{"draw",s.draw},{"hand",s.hand},{"discard",s.discard},{"target_rng",s.target_rng},{"live",s.live},{"adds",s.adds},{"exhaust",s.exhaust},{"created",s.created},{"stars",s.stars}};
}
inline void from_json(const nlohmann::json& j,CombatSnapshot& s){
    auto get=[&](auto& value,const char* key){j.at(key).get_to(value);};
    get(s.enemy_id,"enemy_id");get(s.player,"player");get(s.enemy,"enemy");get(s.phase,"phase");get(s.intent,"intent");
    get(s.energy,"energy");get(s.rage,"rage");get(s.turn,"turn");get(s.guard_streak,"guard_streak");get(s.phase_index,"phase_index");
    get(s.shuffle_rng,"shuffle_rng");get(s.intent_rng,"intent_rng");get(s.phase_opening,"phase_opening");get(s.draw,"draw");get(s.hand,"hand");get(s.discard,"discard");
    get(s.target_rng,"target_rng");get(s.live,"live");get(s.adds,"adds");get(s.exhaust,"exhaust");get(s.created,"created");
    s.stars=j.value("stars",TurnStarPlan{});
}
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MapNode,kind,encounter_id,event_id,floor,column,reward_gold,visited,next)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ShopItem,ware,id,price,sold)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EventChoice,label,outcome,card,relic,potion,heal,damage,gold,max_hp,upgrade,action,amount,chance)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RunState,seed,index,node,hp,max_hp,rage,gold,potion_slots,class_id,event_title,event_text,deck,relics,potions,rewards,event_choices,map,options,shop,place,pending_reward,victory,defeat,fight_resolved,rest_used,act,challenge,trial_limit,reward_multiplier,fog_until,sequence_gold,advance_act,reward_from_event,sequence_relic,meta_claimed,phoenix_spent,run_id,active_encounter_id,selection,last_outcome,skills,relic_rewards,sequence,defeated_bosses,bonuses,metrics)
}
