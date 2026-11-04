#pragma once
#include "quest_core.hpp"

namespace undergod {
// Linear route for v0.2. MapNode::next is a single edge today; a later MapGen
// can fill a branching graph of combat, elite, event, shop, rest and boss nodes
// without changing this state. relics, potions, gold and class_id are kept so
// those systems can attach without another save-shape change.
struct MapNode {
    NodeKind kind=NodeKind::Combat;
    std::string encounter_id;
    std::vector<int> next;
};

struct RunState {
    unsigned seed=1337;
    int index=0;
    int hp=50,max_hp=50,rage=0,gold=0;
    std::string class_id="chick";
    std::vector<std::string> deck,relics,potions,rewards;
    std::vector<MapNode> map;
    bool pending_reward=false,victory=false,defeat=false,fight_resolved=false;
};

std::vector<MapNode> BuildRoute(const std::vector<RouteNode>& route);
const EnemyDef& FindEnemy(const Config& config,const std::string& id);
uint32_t EncounterSeed(unsigned seed,int index);
void StartRun(RunState& run,Combat& combat,unsigned seed);
void ObserveCombat(RunState& run,Combat& combat);
bool ChooseReward(RunState& run,Combat& combat,int index);
int SuggestCard(const Combat& combat);
int SuggestReward(const RunState& run,const Config& config);
std::string NextEncounterName(const RunState& run,const Config& config);
}
