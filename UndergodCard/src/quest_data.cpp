#include "quest_core.hpp"
#include "quest_timeline.hpp"
#include "../third_party/nlohmann/json.hpp"
#include <fstream>
#include <set>
#include <cmath>

namespace undergod {
namespace {
using Json=nlohmann::json;
Json Read(const std::string& path){std::ifstream f(path);if(!f)throw std::runtime_error("Cannot open "+path);Json data;f>>data;return data;}
bool Ident(const std::string& id){return !id.empty()&&id.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_")==std::string::npos;}
StatusId ParseStatus(const std::string& name){
    if(name=="block")return StatusId::Block;
    if(name=="poison")return StatusId::Poison;
    if(name=="weak")return StatusId::Weak;
    if(name=="vulnerable")return StatusId::Vulnerable;
    if(name=="strength")return StatusId::Strength;
    throw std::runtime_error("Unknown status: "+name);
}
Effect ParseEffect(const Json& j){
    Effect effect;const std::string op=j.at("op");
    if(op=="damage")effect.op=EffectOp::Damage;
    else if(op=="block")effect.op=EffectOp::Block;
    else if(op=="heal")effect.op=EffectOp::Heal;
    else if(op=="apply")effect.op=EffectOp::Apply;
    else if(op=="draw")effect.op=EffectOp::Draw;
    else if(op=="energy")effect.op=EffectOp::Energy;
    else if(op=="rage")effect.op=EffectOp::Rage;
    else if(op=="retain")effect.op=EffectOp::Retain;
    else if(op=="thorns")effect.op=EffectOp::Thorns;
    else if(op=="revive")effect.op=EffectOp::Revive;
    else if(op=="double_attack")effect.op=EffectOp::DoubleAttack;
    else if(op=="end_block")effect.op=EffectOp::EndBlock;
    else if(op=="ult_discount")effect.op=EffectOp::UltDiscount;
    else if(op=="temp_strength")effect.op=EffectOp::TempStrength;
    else if(op=="next_energy")effect.op=EffectOp::NextEnergy;
    else throw std::runtime_error("Unknown effect: "+op);
    effect.amount=j.at("amount");
    effect.times=j.value("times",1);
    const std::string target=j.value("target",effect.op==EffectOp::Damage||effect.op==EffectOp::Apply?"enemy":"self");
    if(target=="enemy")effect.target=EffectTarget::Enemy;
    else if(target=="self")effect.target=EffectTarget::Self;
    else if(target=="all")effect.target=EffectTarget::AllEnemies;
    else if(target=="random")effect.target=EffectTarget::RandomEnemy;
    else throw std::runtime_error("Unknown effect target");
    if(effect.op==EffectOp::Apply)effect.status=ParseStatus(j.at("status"));
    effect.pierce=j.value("pierce",false);effect.ranged=j.value("ranged",false);
    if(effect.amount<0||effect.times<1||effect.times>8)throw std::runtime_error("Invalid effect magnitude");
    return effect;
}
void Summarize(CardDef& card){
    card.damage=card.block=card.heal=card.poison=card.strength=card.weak=card.vulnerable=card.cards_drawn=card.energy_gain=0;
    for(const auto& effect:card.effects){
        const int total=effect.amount*std::max(1,effect.times);
        if(effect.op==EffectOp::Damage&&effect.target!=EffectTarget::Self)card.damage+=total;
        else if(effect.op==EffectOp::Block&&effect.target==EffectTarget::Self)card.block+=total;
        else if(effect.op==EffectOp::Heal&&effect.target==EffectTarget::Self)card.heal+=total;
        else if(effect.op==EffectOp::Apply&&effect.target==EffectTarget::Enemy&&effect.status==StatusId::Poison)card.poison+=total;
        else if(effect.op==EffectOp::Apply&&effect.target==EffectTarget::Self&&effect.status==StatusId::Strength)card.strength+=total;
        else if(effect.op==EffectOp::TempStrength)card.strength+=total;
        else if(effect.op==EffectOp::Apply&&effect.target==EffectTarget::Enemy&&effect.status==StatusId::Weak)card.weak+=total;
        else if(effect.op==EffectOp::Apply&&effect.target==EffectTarget::Enemy&&effect.status==StatusId::Vulnerable)card.vulnerable+=total;
        else if(effect.op==EffectOp::Draw)card.cards_drawn+=total;
        else if(effect.op==EffectOp::Energy&&effect.target==EffectTarget::Self)card.energy_gain+=total;
    }
}
std::vector<IntentPattern> ParsePatterns(const Json& list){
    std::vector<IntentPattern> patterns;
    for(const auto& j:list){
        IntentPattern pattern;const std::string kind=j.at("kind");
        if(kind=="attack")pattern.kind=IntentKind::Attack;
        else if(kind=="defend"||kind=="block")pattern.kind=IntentKind::Defend;
        else if(kind=="buff")pattern.kind=IntentKind::Buff;
        else if(kind=="debuff")pattern.kind=IntentKind::Debuff;
        else if(kind=="multi")pattern.kind=IntentKind::MultiAttack;
        else if(kind=="summon")pattern.kind=IntentKind::Summon;
        else if(kind=="charge")pattern.kind=IntentKind::Charge;
        else if(kind=="curse")pattern.kind=IntentKind::Curse;
        else if(kind=="mimic")pattern.kind=IntentKind::Mimic;
        else if(kind=="drain")pattern.kind=IntentKind::Drain;
        else throw std::runtime_error("Unknown intent kind: "+kind);
        pattern.min_damage=j.value("minDamage",0);
        pattern.max_damage=j.value("maxDamage",pattern.min_damage);
        pattern.block=j.value("block",0);
        pattern.weight=j.value("weight",1);
        pattern.status_amount=j.value("amount",0);
        pattern.hits=j.value("hits",1);pattern.charge_turns=j.value("chargeTurns",0);pattern.summon_id=j.value("summon","");
        if(pattern.hits<1||pattern.hits>8||pattern.charge_turns<0||pattern.charge_turns>3)throw std::runtime_error("Invalid intent repetition");
        if(j.contains("status"))pattern.status=ParseStatus(j.at("status"));
        if(pattern.weight<1||pattern.min_damage<0||pattern.max_damage<pattern.min_damage||pattern.block<0||pattern.status_amount<0)throw std::runtime_error("Invalid intent pattern");
        if(pattern.kind==IntentKind::Attack&&pattern.max_damage<1)throw std::runtime_error("Attack pattern needs damage");
        if((pattern.kind==IntentKind::Buff||pattern.kind==IntentKind::Debuff)&&pattern.status_amount<1&&pattern.block<1&&pattern.max_damage<1)throw std::runtime_error("Empty status pattern");
        patterns.push_back(pattern);
    }
    if(patterns.empty())throw std::runtime_error("Intent list is empty");
    return patterns;
}
void ReadLegacyStats(const Json& j,int& min_damage,int& max_damage,int& block,double& chance){
    min_damage=j.at("minDamage");max_damage=j.at("maxDamage");block=j.at("block");chance=j.at("attackChance");
    if(chance<0||chance>1||min_damage<0||max_damage<min_damage||block<0)throw std::runtime_error("Invalid enemy attack stats");
}
PhaseDef ParsePhase(const Json& j){
    PhaseDef phase;
    phase.name=j.at("name");
    phase.hp=j.at("maxHp");
    phase.opening_attack=j.value("openingAttack",true);
    phase.max_guards=j.value("maxConsecutiveGuards",1);
    phase.strength=j.value("strength",0);
    phase.threshold=j.value("threshold",0);phase.cleanse=j.value("cleanse",false);phase.lock_card=j.value("lockCard",false);
    phase.summon_id=j.value("summon","");phase.summon_count=j.value("summonCount",0);phase.constrict=j.value("constrict",0);phase.doom=j.value("doom",0);
    if(phase.threshold<0||phase.threshold>=100||phase.summon_count<0||phase.summon_count>4||phase.constrict<0||phase.constrict>10||phase.doom<0||phase.doom>10)throw std::runtime_error("Invalid boss mechanic");
    if(phase.name.empty()||phase.hp<1||phase.max_guards<0||phase.strength<0)throw std::runtime_error("Invalid enemy phase");
    if(j.contains("patterns"))phase.patterns=ParsePatterns(j.at("patterns"));
    else ReadLegacyStats(j,phase.min_damage,phase.max_damage,phase.block,phase.attack_chance);
    return phase;
}
EnemyDef ParseEnemy(const Json& j){
    EnemyDef enemy;
    enemy.id=j.at("id");enemy.name=j.at("name");enemy.role=j.value("role","combat");
    enemy.hp=j.at("maxHp");
    enemy.min_floor=j.value("minFloor",0);
    enemy.sprite=j.value("sprite","frog");enemy.mechanic=j.value("mechanic","");enemy.airborne=j.value("airborne",false);
    enemy.opening_attack=j.value("openingAttack",true);
    enemy.max_guards=j.value("maxConsecutiveGuards",1);
    if(!Ident(enemy.id)||enemy.name.empty()||enemy.hp<1||enemy.max_guards<0||enemy.min_floor<0)throw std::runtime_error("Invalid enemy definition");
    if(enemy.role!="combat"&&enemy.role!="elite"&&enemy.role!="boss"&&enemy.role!="summon")throw std::runtime_error("Invalid enemy role");
    if(j.contains("patterns"))enemy.patterns=ParsePatterns(j.at("patterns"));
    else ReadLegacyStats(j,enemy.min_damage,enemy.max_damage,enemy.block,enemy.attack_chance);
    if(j.contains("phases"))for(const auto& phase:j.at("phases"))enemy.phases.push_back(ParsePhase(phase));
    return enemy;
}
std::vector<Effect> EffectList(const Json& j,const char* key){std::vector<Effect> effects;if(!j.contains(key))return effects;for(const auto& effect:j.at(key))effects.push_back(ParseEffect(effect));return effects;}
std::string Describe(const CardDef& card){
    std::string text;
    for(const auto& effect:card.effects){
        const auto amount=std::to_string(effect.amount);std::string line;
        switch(effect.op){
        case EffectOp::Damage:line="Gây "+amount+" sát thương";if(effect.times>1)line+=" ×"+std::to_string(effect.times);if(effect.pierce)line+=", xuyên giáp";break;
        case EffectOp::Block:line="Nhận "+amount+" giáp";break;
        case EffectOp::Heal:line="Hồi "+amount+" HP";break;
        case EffectOp::Apply:line=std::string(effect.target==EffectTarget::Self?"Nhận ":"Gây ")+amount+" "+StatusName(effect.status);break;
        case EffectOp::Draw:line="Rút "+amount+" lá";break;
        case EffectOp::Energy:line="Nhận "+amount+" năng lượng";break;
        case EffectOp::Rage:line="Nhận "+amount+" Nộ";break;
        case EffectOp::Retain:line="Giữ giáp "+amount+" lượt";break;
        case EffectOp::Thorns:line="Lượt này, phản "+amount+" sát thương mỗi khi bị đánh";break;
        case EffectOp::Revive:line="Hồi sinh với 30% HP, một lần trong hành trình";break;
        case EffectOp::DoubleAttack:line="Mỗi lượt, lá tấn công đầu tiên gây gấp "+amount+" sát thương";break;
        case EffectOp::EndBlock:line="Cuối mỗi lượt nhận "+amount+" giáp";break;
        case EffectOp::UltDiscount:line="Tuyệt kỹ cần ít hơn "+amount+" Nộ";break;
        case EffectOp::TempStrength:line="Nhận "+amount+" Sức Mạnh trong lượt này";break;
        case EffectOp::NextEnergy:line="Nhận "+amount+" năng lượng lượt sau";break;
        }
        if(effect.target==EffectTarget::AllEnemies)line+=" lên mọi địch";
        else if(effect.target==EffectTarget::RandomEnemy)line+=" lên địch ngẫu nhiên";
        text+=line+". ";
    }
    if(card.return_on_kill)text+="Trở về tay nếu kết liễu. ";
    if(card.power)text+="Năng lực. ";
    if(card.exhaust)text+="Tiêu hao.";
    return text;
}
CardDef UpgradeOf(CardDef card){
    card.copies=0;card.upgraded=true;card.upgrade_id.clear();card.id+="+";card.name+="+";
    bool boosted=false;
    for(auto& effect:card.effects){
        if(boosted)break;
        if(effect.op==EffectOp::Damage||effect.op==EffectOp::Block||effect.op==EffectOp::Heal){effect.amount+=3;boosted=true;}
        else if(effect.op==EffectOp::Apply){effect.amount+=2;boosted=true;}
    }
    if(!boosted&&card.cost>0){--card.cost;boosted=true;}
    if(!boosted&&!card.effects.empty())++card.effects.front().amount;
    Summarize(card);card.description=Describe(card);
    return card;
}
}
Config LoadConfig(const std::string& directory){
    Config c;const auto b=Read(directory+"/balance.json");
    c.balance={b.at("playerHp"),b.at("energy"),b.at("draw"),b.at("rageMax"),b.at("rageAttack"),b.at("rageBlock"),b.at("ultimateDamage")};
    const auto character=Read(directory+"/characters.json").at(0);c.player_name=character.at("name");c.player_id=character.at("id");
    if(!Ident(c.player_id))throw std::runtime_error("Invalid character ID");
    std::set<std::string> ids;
    for(const auto& j:Read(directory+"/enemies.json")){
        EnemyDef enemy=ParseEnemy(j);
        if(!ids.insert(enemy.id).second)throw std::runtime_error("Duplicate enemy id");
        c.bestiary.push_back(std::move(enemy));
    }
    if(c.bestiary.empty())throw std::runtime_error("Enemy list is empty");
    const auto route=Read(directory+"/run.json");
    c.reward_choices=route.value("rewardChoices",3);
    c.gold_start=route.value("goldStart",50);c.heal_percent=route.value("healPercent",30);c.remove_price=route.value("removePrice",70);c.potion_slots=route.value("potionSlots",2);
    const auto map=Read(directory+"/map.json");
    c.map_columns=map.at("columns");c.map_floors=map.at("floors");c.elite_min_floor=map.at("eliteMinFloor");
    const auto weights=map.at("weights");const char* names[]={"combat","elite","event","shop","rest"};
    int total_weight=0;for(int i=0;i<5;++i){c.map_weights[i]=weights.at(names[i]);if(c.map_weights[i]<0||c.map_weights[i]>1000)throw std::runtime_error("Invalid map weight");total_weight+=c.map_weights[i];}
    c.combat_gold_min=map.at("combatGold").at(0);c.combat_gold_max=map.at("combatGold").at(1);
    c.elite_gold_min=map.at("eliteGold").at(0);c.elite_gold_max=map.at("eliteGold").at(1);
    if(total_weight<1||total_weight-c.map_weights[int(NodeKind::Elite)]<1||c.elite_min_floor<1||c.elite_min_floor>=c.map_floors||c.combat_gold_min<0||c.combat_gold_max<c.combat_gold_min||c.elite_gold_min<0||c.elite_gold_max<c.elite_gold_min)throw std::runtime_error("Invalid map configuration");
    c.boss_id=route.value("boss","frog");c.elite_id=route.value("elite","elite_frog");
    if(route.contains("combats"))for(const auto& id:route.at("combats"))c.combat_ids.push_back(id.get<std::string>());
    if(route.contains("elites"))for(const auto& id:route.at("elites"))c.elite_ids.push_back(id.get<std::string>());
    if(c.elite_ids.empty())c.elite_ids.push_back(c.elite_id);
    auto known=[&](const std::string& id){for(const auto& enemy:c.bestiary)if(enemy.id==id)return true;return false;};
    if(c.combat_ids.empty()||!known(c.boss_id)||!known(c.elite_id)||c.map_columns<2||c.map_columns>4||c.map_floors<3||c.map_floors>15||c.gold_start<0||c.heal_percent<1||c.heal_percent>100||c.remove_price<1||c.potion_slots<1||c.potion_slots>4)throw std::runtime_error("Invalid run configuration");
    for(const auto& id:c.combat_ids)if(!known(id))throw std::runtime_error("Unknown combat enemy: "+id);
    for(const auto& id:c.elite_ids)if(!known(id))throw std::runtime_error("Unknown elite enemy: "+id);
    auto role_of=[&](const std::string& id)->const EnemyDef&{for(const auto& enemy:c.bestiary)if(enemy.id==id)return enemy;throw std::runtime_error("Unknown enemy: "+id);};
    for(const auto& id:c.combat_ids)if(role_of(id).role!="combat")throw std::runtime_error("Combat pool has non-combat enemy");
    for(const auto& id:c.elite_ids)if(role_of(id).role!="elite")throw std::runtime_error("Elite pool has non-elite enemy");
    if(role_of(c.boss_id).role!="boss"||role_of(c.combat_ids.front()).min_floor!=0)throw std::runtime_error("Invalid opening enemy or boss");
    for(const auto& enemy:c.bestiary)if(enemy.id==c.combat_ids.front())c.enemy=enemy;
    for(const auto& j:Read(directory+"/events.json")){
        EventDef event;event.id=j.at("id");event.title=j.at("title");event.text=j.at("text");
        if(!Ident(event.id)||event.title.empty()||event.text.empty())throw std::runtime_error("Invalid event");
        for(const auto& choice:j.at("choices")){
            EventChoice entry;entry.label=choice.at("label");entry.outcome=choice.at("outcome");
            entry.heal=choice.value("heal",0);entry.damage=choice.value("damage",0);entry.gold=choice.value("gold",0);entry.max_hp=choice.value("maxHp",0);
            entry.card=choice.value("card","");entry.relic=choice.value("relic","");entry.potion=choice.value("potion","");entry.upgrade=choice.value("upgrade",false);
            entry.action=choice.value("action","");entry.amount=choice.value("amount",0);entry.chance=choice.value("chance",.5);
            if(entry.label.empty()||entry.outcome.empty())throw std::runtime_error("Invalid event choice");
            event.choices.push_back(entry);
        }
        if(event.choices.size()<2)throw std::runtime_error("Event needs two choices");
        c.events.push_back(std::move(event));
    }
    for(const auto& j:Read(directory+"/relics.json")){
        RelicDef relic;relic.id=j.at("id");relic.name=j.at("name");relic.description=j.at("description");relic.price=j.value("price",50);
        relic.bonus_damage=j.value("bonusDamage",0);relic.bonus_block=j.value("bonusBlock",0);relic.gold_bonus=j.value("goldBonus",0);
        relic.rest_bonus=j.value("restBonus",0);relic.shop_discount=j.value("shopDiscount",0);relic.max_hp=j.value("maxHp",0);relic.potion_slot=j.value("potionSlot",0);
        relic.combat_start=EffectList(j,"onCombatStart");relic.turn_start=EffectList(j,"onTurnStart");
        relic.rarity=j.value("rarity","common");relic.rest_percent=j.value("restPercent",0);relic.first_attack=j.value("firstAttack",0);relic.retain_block=j.value("retainBlock",0);
        relic.energy=j.value("energy",0);relic.draw=j.value("draw",0);relic.start_rage=j.value("startRage",0);relic.ultimate_bonus=j.value("ultimateBonus",0);relic.preview=j.value("preview",false);relic.comet=j.value("comet",false);
        if(!Ident(relic.id)||relic.name.empty()||relic.price<1)throw std::runtime_error("Invalid relic");
        c.relics.push_back(std::move(relic));
    }
    for(const auto& j:Read(directory+"/potions.json")){
        PotionDef potion;potion.id=j.at("id");potion.name=j.at("name");potion.description=j.at("description");potion.price=j.value("price",40);potion.effects=EffectList(j,"effects");
        if(!Ident(potion.id)||potion.name.empty()||potion.effects.empty()||potion.price<1)throw std::runtime_error("Invalid potion");
        c.potions.push_back(std::move(potion));
    }
    if(c.events.size()<10||c.relics.size()<10||c.potions.size()<5)throw std::runtime_error("Content pool is too small");
    ids.clear();int copies=0,rewardable=0;
    for(const auto& j:Read(directory+"/cards.json")){
        CardDef card;
        card.id=j.at("id");card.name=j.at("name");card.description=j.at("description");card.cost=j.at("cost");card.copies=j.value("copies",0);
        card.rarity=j.value("rarity",card.copies>0?"starter":"common");
        card.type=j.value("type","attack");card.class_id=j.value("class","any");card.requires_skill=j.value("requiresSkill","");
        card.exhaust=j.value("exhaust",false);card.power=j.value("power",false);card.unplayable=j.value("unplayable",false);card.return_on_kill=j.value("returnOnKill",false);card.ranged=j.value("ranged",false);
        if(j.contains("keywords"))for(const auto& word:j.at("keywords"))card.keywords.push_back(word.get<std::string>());
        if(j.contains("effects"))for(const auto& effect:j.at("effects"))card.effects.push_back(ParseEffect(effect));
        else{
            const int damage=j.value("damage",0),block=j.value("block",0),heal=j.value("heal",0);
            if(damage<0||block<0||heal<0)throw std::runtime_error("Invalid card definition");
            if(damage>0)card.effects.push_back({EffectOp::Damage,EffectTarget::Enemy,StatusId::Poison,damage,1});
            if(block>0)card.effects.push_back({EffectOp::Block,EffectTarget::Self,StatusId::Block,block,1});
            if(heal>0)card.effects.push_back({EffectOp::Heal,EffectTarget::Self,StatusId::Poison,heal,1});
        }
        Summarize(card);
        const bool useful=!card.effects.empty()||card.rarity=="curse";
        if(!Ident(card.id)||!ids.insert(card.id).second||card.name.empty()||card.cost<0||card.cost>10||card.copies<0||!useful)throw std::runtime_error("Invalid card definition");
        if(card.rarity!="starter"&&card.rarity!="common"&&card.rarity!="uncommon"&&card.rarity!="rare"&&card.rarity!="legendary"&&card.rarity!="curse")throw std::runtime_error("Invalid card rarity");
        if(card.keywords.size()>4)throw std::runtime_error("Too many keywords");
        for(const auto& word:card.keywords)if(word.empty())throw std::runtime_error("Empty keyword");
        if(card.rarity!="starter"&&card.copies==0)++rewardable;
        copies+=card.copies;c.cards.push_back(std::move(card));
    }
    const int base_count=int(c.cards.size());
    for(int i=0;i<base_count;++i){if(c.cards[i].rarity=="curse")continue;CardDef upgraded=UpgradeOf(c.cards[i]);c.cards[i].upgrade_id=upgraded.id;c.cards.push_back(std::move(upgraded));}
    auto has_card=[&](const std::string& id){for(const auto& card:c.cards)if(card.id==id)return true;return false;};
    auto has_relic=[&](const std::string& id){for(const auto& relic:c.relics)if(relic.id==id)return true;return false;};
    auto has_potion=[&](const std::string& id){for(const auto& potion:c.potions)if(potion.id==id)return true;return false;};
    for(const auto& event:c.events)for(const auto& choice:event.choices){
        if(!choice.card.empty()&&!has_card(choice.card))throw std::runtime_error("Event card was not defined: "+choice.card);
        if(!choice.relic.empty()&&!has_relic(choice.relic))throw std::runtime_error("Event relic was not defined: "+choice.relic);
        if(!choice.potion.empty()&&!has_potion(choice.potion))throw std::runtime_error("Event potion was not defined: "+choice.potion);
    }
    if(copies<1||copies>1000||rewardable<c.reward_choices||c.reward_choices<1||c.reward_choices>3||c.balance.player_hp<1||c.enemy.hp<1||c.balance.draw<1||c.balance.draw>10||c.balance.energy<1||c.balance.rage_max<1||c.balance.rage_attack<0||c.balance.rage_block<0||c.balance.ultimate_damage<1)throw std::runtime_error("Invalid balance configuration");
    std::set<std::string> deck_owners,boss_card_ids;
    for(const auto& deck:Read(directory+"/boss_cards.json")){
        const std::string owner=deck.at("boss");const auto& boss=role_of(owner);
        if(boss.role!="boss"||!deck_owners.insert(owner).second)throw std::runtime_error("Invalid or duplicate boss deck: "+owner);
        int guards=0;
        for(const auto& j:deck.at("cards")){
            BossCardDef card;card.boss_id=owner;card.id=j.at("id");card.name=j.at("name");
            card.min_phase=j.value("minPhase",0);card.max_phase=j.value("maxPhase",int(boss.phases.size()));
            card.action=ParsePatterns(Json::array({j.at("action")})).front();
            if(!Ident(card.id)||card.id.rfind("boss_"+owner+"_",0)!=0||!boss_card_ids.insert(card.id).second||has_card(card.id)||card.name.empty()||card.min_phase<0||card.max_phase<card.min_phase||card.max_phase>int(boss.phases.size()))throw std::runtime_error("Invalid boss card: "+card.id);
            if(card.action.kind==IntentKind::Charge&&(card.action.charge_turns<1||card.action.max_damage<1))throw std::runtime_error("Boss charge needs a telegraph and damage");
            if(card.action.kind==IntentKind::Summon&&(card.action.summon_id.empty()||!known(card.action.summon_id)||role_of(card.action.summon_id).role!="summon"))throw std::runtime_error("Invalid boss card summon");
            if(card.action.kind==IntentKind::Defend&&card.min_phase==0&&card.max_phase==int(boss.phases.size()))++guards;
            c.boss_cards.push_back(std::move(card));
        }
        if(deck.at("cards").size()!=6||guards<1)throw std::runtime_error("Boss deck needs six cards and a guard in every phase");
        for(int phase=0;phase<=int(boss.phases.size());++phase){
            int eligible=0,attacks=0;
            for(const auto& card:c.boss_cards)if(card.boss_id==owner&&phase>=card.min_phase&&phase<=card.max_phase){++eligible;const auto k=card.action.kind;attacks+=k==IntentKind::Attack||k==IntentKind::MultiAttack||k==IntentKind::Mimic||k==IntentKind::Drain||k==IntentKind::Charge;}
            if(eligible<3||attacks<1)throw std::runtime_error("Boss phase has no usable deck");
        }
    }
    for(const auto& boss:c.bestiary)if(boss.role=="boss"&&!deck_owners.count(boss.id))throw std::runtime_error("Boss has no exclusive deck: "+boss.id);
    LoadWorld(c,directory);
    return c;
}
Timeline LoadTimeline(const std::string& file){
    const auto j=Read(file);Timeline timeline;timeline.skippable=j.value("skippable",true);int impacts=0,ends=0,falls=0;float impact_time=0,end_time=0,fall_end=0;
    for(const auto& s:j.at("steps")){
        Step step;step.t=s.at("t");const std::string action=s.at("action");
        if(action=="dimScreen"){step.cue=Cue::Dim;step.value=s.at("alpha");}
        else if(action=="cameraZoom"){step.cue=Cue::Zoom;step.value=s.at("to");step.duration=s.at("duration");}
        else if(action=="playSprite"){
            step.cue=Cue::Pose;const std::string target=s.at("target");if(target!="player"&&target!="enemy")throw std::runtime_error("Unknown cinematic actor");step.enemy=target=="enemy";step.pose=s.at("anim");
            if(step.pose!="idle"&&step.pose!="attack"&&step.pose!="hit"&&step.pose!="defend"&&step.pose!="ult_charge"&&step.pose!="ult_throw")throw std::runtime_error("Unknown cinematic pose");
        }
        else if(action=="spawnVFX"){
            step.cue=Cue::Vfx;step.value=s.at("frames");const std::string target=s.at("at");if(target!="player"&&target!="enemy")throw std::runtime_error("Unknown VFX target");step.enemy=target=="enemy";
            if(s.at("sheet")!="vfx/spear_impact.png"||step.value<1||step.value>4)throw std::runtime_error("Unsupported VFX sheet or frame count");
        }
        else if(action=="moveSpear"){
            step.cue=Cue::Spear;const std::string phase=s.at("phase");if(phase!="rise"&&phase!="fall")throw std::runtime_error("Invalid spear phase");step.falling=phase=="fall";
            step.duration=s.at("duration");step.from_x=s.at("from").at(0);step.from_y=s.at("from").at(1);step.to_x=s.at("to").at(0);step.to_y=s.at("to").at(1);
            if(step.duration<=0)throw std::runtime_error("Spear movement needs a positive duration");
            if(step.falling){++falls;fall_end=step.t+step.duration;}
        }
        else if(action=="shake"){step.cue=Cue::Shake;step.value=s.at("intensity");step.duration=s.at("duration");}
        else if(action=="impact"){if(s.at("callback")!="applyDamage")throw std::runtime_error("Unknown cinematic callback");step.cue=Cue::Impact;++impacts;impact_time=step.t;}
        else if(action=="flashWhite"){step.cue=Cue::Flash;step.duration=s.at("duration");}
        else if(action=="end"){step.cue=Cue::End;++ends;end_time=step.t;}
        else throw std::runtime_error("Unknown cinematic action: "+action);
        if(step.t<0||step.duration<0)throw std::runtime_error("Invalid cinematic timing");
        timeline.steps.push_back(step);
    }
    if(impacts!=1||ends!=1||impact_time>end_time)throw std::runtime_error("Cinematic needs one impact before end");
    if(falls!=1||std::abs(fall_end-impact_time)>.1f)throw std::runtime_error("Spear contact must match damage timing");
    return timeline;
}
}
