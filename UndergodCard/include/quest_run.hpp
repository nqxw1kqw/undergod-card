#pragma once
#include "quest_core.hpp"

namespace undergod {
enum class Place { Map, Fight, Reward, Event, Shop, Rest };
enum class Ware { Card, Relic, Potion };
enum class AutoKind { None, Node, Reward, Event, Buy, Remove, Leave, Heal, Upgrade, Potion, Play, EndTurn, Ultimate };

struct MapNode {
    NodeKind kind=NodeKind::Combat;
    std::string encounter_id,event_id;
    int floor=0,column=0,reward_gold=0;
    bool visited=false;
    std::vector<int> next;
};

struct ShopItem { Ware ware=Ware::Card; std::string id; int price=0; bool sold=false; };
struct AutoStep { AutoKind kind=AutoKind::None; int index=0; };

struct RunState {
    unsigned seed=1337;
    int index=-1,node=-1;
    int hp=50,max_hp=50,rage=0,gold=0,potion_slots=2;
    std::string class_id="chick";
    std::string event_title,event_text;
    std::vector<std::string> deck,relics,potions,rewards;
    std::vector<EventChoice> event_choices;
    std::vector<MapNode> map;
    std::vector<int> options;
    std::vector<ShopItem> shop;
    Place place=Place::Fight;
    bool pending_reward=false,victory=false,defeat=false,fight_resolved=false,rest_used=false;
};

std::vector<MapNode> GenerateMap(const Config& config,unsigned seed);
const EnemyDef& FindEnemy(const Config& config,const std::string& id);
const CardDef& FindCard(const Config& config,const std::string& id);
const RelicDef& FindRelic(const Config& config,const std::string& id);
const PotionDef& FindPotion(const Config& config,const std::string& id);
uint32_t EncounterSeed(unsigned seed,int index);
CombatMods ModsFor(const RunState& run,const Config& config);
void StartRun(RunState& run,Combat& combat,unsigned seed);
void ObserveCombat(RunState& run,Combat& combat);
bool ChooseNode(RunState& run,Combat& combat,int option);
bool ChooseReward(RunState& run,Combat& combat,int index);
bool ChooseEvent(RunState& run,const Config& config,int index);
bool BuyItem(RunState& run,const Config& config,int index);
bool RemoveCard(RunState& run,const Config& config,int index);
void LeaveShop(RunState& run);
bool RestHeal(RunState& run,const Config& config);
int RestHealAmount(const RunState& run,const Config& config);
bool CanChooseEvent(const RunState& run,int index);
bool UpgradeCard(RunState& run,const Config& config,int index);
bool DrinkPotion(RunState& run,Combat& combat,int index);
int SuggestCard(const Combat& combat);
int SuggestReward(const RunState& run,const Config& config);
AutoStep SuggestAuto(const RunState& run,const Combat& combat);
std::string NextEncounterName(const RunState& run,const Config& config);
}
