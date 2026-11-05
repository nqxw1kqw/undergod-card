#include "game.hpp"
#include "quest_json.hpp"
#include <fstream>
#include <iostream>
#include <set>
using namespace undergod;
using Json=nlohmann::json;
extern void Check(bool,const char*);
namespace {
void Play(Combat& c,const std::string& id){c.hand={id};c.energy=10;c.live.locked=-1;Check(c.Play(0),"mechanic fixture card is playable");}
void End(Combat& c){c.EndPlayer();c.ActEnemy();}
void Settle(Game& g){for(int i=0;i<120&&g.Busy();++i)g.Update(.1f);}
void EventAt(Game& g,const std::string& id){g.NewRun(19);g.run.node=3;g.run.map[3].kind=NodeKind::Event;g.run.map[3].event_id=id;g.run.map[3].reward_gold=0;g.run.place=Place::Event;for(const auto& e:g.state.config.events)if(e.id==id){g.run.event_choices=e.choices;g.run.event_title=e.title;}}
Combat Fight(const Config& config,const std::string& enemy="froglet",const std::string& character="chick"){
    Combat c(config);RunState r;StartRun(r,c,42,character);r.active_encounter_id=enemy;
    c.BeginEncounter(FindEnemy(config,enemy),42,r.hp,r.max_hp,0,r.deck,ModsFor(r,config));return c;
}
}

void WorldChecks(const Config& config,const Timeline& timeline){
    Check(config.classes.size()==3&&config.acts.size()==3&&config.skills.size()==45&&config.challenges.size()==10&&config.events.size()==12,"full campaign content counts");
    for(const auto& card:config.cards)if(!card.upgraded&&card.rarity!="curse"){
        const auto& upgrade=FindCard(config,card.upgrade_id);
        Check(upgrade.cost<card.cost||Json(upgrade.effects)!=Json(card.effects),"every upgrade changes an actual cost or effect");
        Check(upgrade.description.find("Bản nâng cấp")==std::string::npos&&!upgrade.description.empty(),"upgrades describe their actual effects");
    }
    {
        auto c=Fight(config,"frog");c.enemy.hp=76;Play(c,"punch");
        Check(c.PhaseIndex()==1&&c.enemy.hp==70&&c.adds.size()==2&&c.Targets().size()==3,"frog threshold keeps HP and summons two real adds");
        Check(c.live.locked>=0||c.hand.empty(),"frog lock applies only to an available card");
        c.hand={"shield","shield"};End(c);Check(c.live.locked>=0&&!c.CanPlay(c.live.locked),"frog locks one newly drawn card");
        const int player_hp=c.player.hp;c.intent={false,0};End(c);Check(c.player.hp<player_hp,"tadpoles independently attack");
        auto snap=c.Snapshot();Combat resumed(config);resumed.Restore(snap,c.Mods());Check(Json(snap)==Json(resumed.Snapshot()),"summons locks target positions and RNG round trip");
        c.SelectTarget(1);c.adds[0].fighter.hp=5;Play(c,"returning_spear");Check(c.adds[0].fighter.hp==0&&c.hand==std::vector<std::string>{"returning_spear"},"returning spear returns on killing a selected add");
        c.SelectTarget(0);const int boss_hp=c.enemy.hp,add_hp=c.adds[1].fighter.hp;Play(c,"whirlwind");Check(c.enemy.hp==boss_hp-8&&c.adds[1].fighter.hp==std::max(0,add_hp-8),"AOE hits every surviving target");
        c.enemy.hp=1;Play(c,"punch");Check(c.phase!=Phase::Won,"remaining adds keep battle active after boss death");
        c.SelectTarget(2);Play(c,"heavy");Check(c.phase==Phase::Won,"battle ends when every real enemy dies");
    }
    {
        auto c=Fight(config,"snake");c.enemy.hp=118;c.enemy.status.poison=10;c.enemy.status.weak=3;Play(c,"punch");
        Check(c.PhaseIndex()==1&&c.enemy.status.poison==0&&c.enemy.status.weak==0&&c.enemy.status.strength==3,"snake sheds debuffs and gains strength without refill");
        c.enemy.hp=46;Play(c,"punch");Check(c.PhaseIndex()==2&&c.live.constrict_left==3,"snake enters three-turn constriction below quarter HP");
        c.player.hp=c.player.max_hp=500;c.intent={};c.intent.kind=IntentKind::Charge;c.intent.value=27;c.live.charge_left=2;const int before=c.player.hp;End(c);
        Check(c.live.charge_left==1&&c.player.hp==before-4&&c.IntentText().find("30")!=std::string::npos,"large charged attack gives two visible player turns");
        End(c);Check(c.player.hp==before-4-4-30,"charge lands with strength after its announcement");
        c.intent={false,0};End(c);Check(c.live.constrict_left==0,"constriction expires after exactly three enemy turns");
    }
    {
        auto c=Fight(config,"hawk");const int hp=c.enemy.hp;Play(c,"dagger");Check(c.enemy.hp==hp-1,"flying hawk halves melee damage");
        Play(c,"piercing_spear");Check(c.enemy.hp==hp-11,"ranged spear bypasses flight reduction");
        Play(c,"piercing_spear");Check(!c.live.airborne&&c.live.land_turns==2,"twenty actual damage forces hawk to land");
        c.intent={false,0};End(c);Check(!c.live.airborne,"hawk stays grounded for the following player turn");c.intent={false,0};End(c);Check(c.live.airborne,"hawk takes flight after two enemy turns");
        c.player.block=5;c.intent={true,6};c.intent.kind=IntentKind::MultiAttack;c.intent.hits=2;const int player_hp=c.player.hp;End(c);Check(c.player.hp==player_hp-7,"enemy multi-hit consumes armor per hit");
    }
    {
        auto c=Fight(config,"fox");c.intent={};c.intent.kind=IntentKind::Curse;End(c);
        Check(c.created==std::vector<std::string>{"mud"}&&c.exhaust.size()==1&&std::find(c.hand.begin(),c.hand.end(),"mud")!=c.hand.end(),"fox transforms a real card with a conserved creation ledger");
        c.enemy.hp=111;Play(c,"punch");Check(c.adds.size()==2&&c.adds[0].illusion&&c.adds[1].illusion&&c.live.slots.size()==3,"fox creates two decoys with shuffled visible slots");
        c.SelectTarget(1);const int real=c.enemy.hp;Play(c,"dagger");Check(c.adds[0].fighter.hp==0&&c.enemy.hp==real,"hitting a decoy does not damage the real boss");
        c.SelectTarget(0);c.enemy.hp=1;Play(c,"punch");Check(c.phase==Phase::Won&&c.adds[1].fighter.hp==0,"fox decoys vanish when real boss dies");
    }
    {
        auto c=Fight(config,"undergod");c.player.hp=c.player.max_hp=500;c.intent={};c.intent.kind=IntentKind::Mimic;c.intent.card_id="double_peck";const int hp=c.player.hp;End(c);
        Check(c.player.hp==hp-9,"Undergod copies all hits of a previous card");
        c.enemy.hp=199;Play(c,"punch");Check(c.PhaseIndex()==1,"Undergod enters rage-drain phase");
        c.rage=80;c.intent={true,12};c.intent.kind=IntentKind::Drain;c.intent.status_amount=20;End(c);Check(c.rage==60,"Undergod drains rage as announced");
        c.enemy.hp=100;Play(c,"punch");Check(c.PhaseIndex()==2&&c.live.doom_left==5,"Undergod final phase starts five-turn doom");
        for(int n=0;n<4;++n){c.intent={false,0};End(c);}Check(c.player.hp>0&&c.live.doom_left==1,"doom leaves four full response turns");c.intent={false,0};End(c);Check(c.phase==Phase::Lost,"doom ends the run on fifth enemy turn");
    }
    {
        auto c=Fight(config,"elder_toad");c.enemy.hp=c.enemy.max_hp=1000;c.enemy.block=30;Play(c,"piercing_spear");Check(c.enemy.hp==990&&c.enemy.block==30,"piercing card preserves armor and damages HP");
        c.enemy.block=0;Play(c,"holy_spear");Play(c,"punch");Play(c,"punch");Check(c.enemy.hp==972&&c.exhaust.size()==1,"power exhausts itself and doubles first attack only");
        c.intent={false,0};End(c);Play(c,"punch");Check(c.enemy.hp==960,"double attack refreshes each turn");
        Play(c,"fortress");c.player.block=0;c.intent={false,0};c.EndPlayer();Check(c.player.block==4,"fortress adds armor at end of player turn");c.ActEnemy();
        Play(c,"counter");const int enemy_hp=c.enemy.hp;c.intent={true,2};End(c);Check(c.enemy.hp==enemy_hp-4,"counter thorns trigger even on fully absorbed hit");
        c.player.block=0;Play(c,"feather_shield");c.intent={false,0};End(c);Check(c.player.block==8,"retain keeps block for one following turn");c.intent={false,0};End(c);Check(c.player.block==0,"retain expires");
        const auto exhausted=c.exhaust.size();Play(c,"cookie");Check(c.exhaust.size()==exhausted+1,"cookie exhausts after healing");
        Play(c,"golden_egg");c.intent={false,0};End(c);Check(c.energy==4,"golden egg grants energy on next turn");c.intent={false,0};End(c);Check(c.energy==3,"next-turn energy is consumed once");
        Play(c,"crow");Check(c.player.status.strength==2,"crow gives temporary strength");c.intent={false,0};End(c);Check(c.player.status.strength==0,"temporary strength expires at end of turn");
        Play(c,"phoenix");c.player.hp=1;c.intent={true,10};End(c);Check(c.phase==Phase::Player&&c.player.hp==15&&c.live.revives==1,"phoenix revives at thirty percent max HP");
        Play(c,"phoenix");c.player.hp=1;c.intent={true,10};End(c);Check(c.phase==Phase::Lost,"phoenix cannot revive twice in a run");
        auto spent=Fight(config);auto spent_mods=spent.Mods();spent_mods.revive_spent=true;spent.BeginEncounter(FindEnemy(config,"froglet"),3,50,50,0,StartingDeck(config),spent_mods);Play(spent,"phoenix");spent.player.hp=1;spent.intent={true,10};End(spent);Check(spent.phase==Phase::Lost,"phoenix spent flag prevents revival in a later encounter");
        auto curse=Fight(config);curse.hand={"mud","fatigue"};Check(!curse.CanPlay(0)&&curse.Play(1)&&curse.exhaust.back()=="fatigue","curse cards enforce unplayable and exhaust rules");
    }
    {
        auto mage=Fight(config,"elder_toad","mage");Check(mage.player.max_hp==40&&mage.hand.size()==6,"mage starts with forty HP and six cards");
        mage.intent={false,0};End(mage);Check(mage.live.mana==3&&mage.energy==3,"mage banks three unused mana and refreshes energy");mage.hand={"heavy","heavy","heavy"};Check(mage.Play(0)&&mage.Play(0)&&mage.Play(0)&&mage.energy==0&&mage.live.mana==0,"cards can spend retained mana");
        mage.enemy.hp=mage.enemy.max_hp=1000;mage.rage=100;mage.StartUltimate();mage.FinishUltimate();Check(mage.enemy.hp==952,"mage ultimate deals four twelve-damage hits");
        auto ninja=Fight(config,"elder_toad","ninja");ninja.enemy.hp=ninja.enemy.max_hp=1000;Play(ninja,"dagger");Play(ninja,"shield");Play(ninja,"dagger");ninja.rage=100;ninja.StartUltimate();ninja.FinishUltimate();Check(ninja.enemy.hp==988&&ninja.player.block==10,"ninja replays three most recent cards for free");
        auto empty_shadow=Fight(config,"elder_toad","ninja");empty_shadow.rage=100;Check(!empty_shadow.StartUltimate()&&empty_shadow.rage==100,"ninja cannot waste ultimate with no card history");
        auto bonus_mods=ninja.Mods();bonus_mods.ultimate_bonus=15;ninja.BeginEncounter(FindEnemy(config,"elder_toad"),42,45,45,100,FindClass(config,"ninja").deck,bonus_mods);Play(ninja,"dagger");const int shadow_hp=ninja.enemy.hp;ninja.StartUltimate();ninja.FinishUltimate();Check(ninja.enemy.hp==shadow_hp-18,"temple ultimate bonus also strengthens ninja replays");
        auto r=RunState{};auto c=Fight(config);StartRun(r,c,1);r.bonuses["spear_bonus"]=1;auto mods=ModsFor(r,config);c.BeginEncounter(FindEnemy(config,"elder_toad"),1,50,50,0,r.deck,mods);Play(c,"punch");const int hp=c.enemy.hp;Play(c,"heavy");Check(c.enemy.hp==hp-12,"forge and spear skill bonus apply to Choc Giao only");
    }
    {
        RunState r;Combat c(config);StartRun(r,c,5);r.relics={"down_feather","lucky_spur","rage_pearl","frog_crown","shell_shield","steel_beak","hawk_eye","meteor_spear"};
        ChooseNode(r,c,0);c.enemy.hp=c.enemy.max_hp=1000;
        Check(c.player.block==8&&c.hand.size()==6&&c.energy==4&&c.rage==30,"opening relic effects combine with crown draw penalty");
        const auto rng=c.Snapshot().intent_rng;const auto preview=c.PreviewIntentText();Check(!preview.empty()&&preview.find("+2")!=std::string::npos&&c.Snapshot().intent_rng==rng,"hawk eye previews two future turns without consuming RNG");
        Play(c,"punch");Play(c,"punch");Check(c.enemy.hp==984,"iron beak applies once per encounter");c.rage=100;c.StartUltimate();c.FinishUltimate();Check(c.enemy.hp==936,"meteor spear makes two sixty-percent hits");
        c.player.block=12;c.intent={false,0};End(c);Check(c.player.block==5,"shell relic retains at most five block");
        auto vulnerable_mods=c.Mods();vulnerable_mods.ultimate_vulnerable=2;c.BeginEncounter(FindEnemy(config,"elder_toad"),42,50,50,100,StartingDeck(config),vulnerable_mods);c.enemy.hp=c.enemy.max_hp=1000;c.StartUltimate();c.FinishUltimate();Check(c.enemy.hp==952&&c.enemy.status.vulnerable==2,"ultimate vulnerable applies once after both meteor hits");
    }
    {
        Game g(config,timeline);EventAt(g,"forge");g.ChooseEvent(0);Check(g.CardSelection()&&g.ChooseCard(0)&&g.run.deck[0]=="punch+","forge opens real upgrade selection");
        EventAt(g,"forge");g.ChooseEvent(1);const auto n=g.run.deck.size();Check(g.ChooseCard(0)&&g.run.deck.size()==n-1,"forge removes selected card");
        EventAt(g,"forge");g.ChooseEvent(2);Check(g.run.hp==42&&RunBonus(g.run,"spear_bonus")==1,"forge spear trade pays eight HP");
        EventAt(g,"egg");g.ChooseEvent(0);Check(g.run.deck.size()==12&&g.run.deck.back()=="mud"&&FindCard(config,g.run.deck[10]).rarity=="legendary","egg grants legendary and curse");
        EventAt(g,"merchant");g.run.relics={"down_feather"};g.ChooseEvent(0);Check(g.RelicSelection()&&g.TradeRelic(0)&&g.run.relics.size()==2,"merchant exchanges one chosen relic for two commons");
        EventAt(g,"lost_frog");g.ChooseEvent(0);Check(RunBonus(g.run,"companion_heal")==2,"frog companion creates persistent fight healing");
        EventAt(g,"star_temple");g.ChooseEvent(0);Check(g.run.max_hp==40&&RunBonus(g.run,"ultimate_bonus")==15,"temple sacrifice changes max HP and ultimate");
        EventAt(g,"mist");g.ChooseEvent(0);Check(g.run.fog_until==g.run.map[g.run.node].floor+2&&g.run.potions.size()==1,"fog event hides two floors and gives a real potion");
        EventAt(g,"pact");g.ChooseEvent(0);g.run.active_encounter_id="undergod";Check(ModsFor(g.run,config).energy==1&&ModsFor(g.run,config).enemy_hp_percent==125,"pact grants energy and strengthens final boss");
        EventAt(g,"library");g.ChooseEvent(0);bool rare=g.Reward()&&g.run.rewards.size()==3;for(const auto& id:g.run.rewards)rare&=FindCard(config,id).rarity=="rare";Check(rare,"library offers three real rare cards");
        EventAt(g,"circus");const int gold=g.run.gold;g.ChooseEvent(0);Check(g.CanAct()&&g.run.trial_limit==3&&g.state.EnemyId()=="circus_dummy","circus starts a real three-turn fight");g.state.enemy.hp=1;g.state.hand={"punch"};g.state.energy=1;g.Play(0);Settle(g);Check(g.Map()&&g.run.gold==gold+100,"circus pays a hundred gold only on winning");
        EventAt(g,"circus");g.ChooseEvent(0);for(int i=0;i<3;++i){g.EndTurn();Settle(g);}Check(g.Map()&&g.run.gold==50,"circus timeout does not award the prize");
        EventAt(g,"ambush");g.ChooseEvent(0);g.state.player.hp=30;g.state.enemy.hp=1;g.state.hand={"punch"};g.state.energy=1;g.Play(0);Settle(g);Check(g.CanAct()&&g.state.player.hp==30&&g.run.sequence.empty(),"ambush immediately starts second fight with carried HP");g.state.enemy.hp=1;g.state.hand={"punch"};g.state.energy=1;g.Play(0);Settle(g);Check(g.Reward()&&g.run.relics.size()==1,"ambush grants a relic after its second fight");
        EventAt(g,"bridge");g.ChooseEvent(1);Check(g.CanAct()&&g.state.Role()=="combat","bridge detour really adds a combat");
        bool elite_bridge=false;
        for(int seed=0;seed<30&&!elite_bridge;++seed){EventAt(g,"bridge");g.run.seed=seed;g.ChooseEvent(0);if(g.state.Role()!="elite")continue;elite_bridge=true;const int gold=g.run.gold;g.state.enemy.hp=1;g.state.hand={"punch"};g.state.energy=1;g.Play(0);Settle(g);Check(g.run.relics.size()==2&&g.run.gold>=gold+60&&g.run.gold<=gold+80,"bridge elite doubles gold and awards two distinct relics");}
        Check(elite_bridge,"bridge fifty-percent branch can actually reach elite");
    }
    {
        Profile p;p.feathers=1000;Check(!UnlockSkill(p,config,"chick_spear_3"),"skill prerequisites block skipping tiers");
        for(int tier=1;tier<=5;++tier)Check(UnlockSkill(p,config,"chick_spear_"+std::to_string(tier)),"sequential skill tier unlocks");
        Check(!UnlockSkill(p,config,"chick_spear_5")&&SkillLevel(p,config,"chick","spear")==5,"skill cannot be bought twice");
        Check(std::find(p.achievements.begin(),p.achievements.end(),"skill")!=p.achievements.end(),"buying first skill immediately awards its achievement");
        RunState run;Combat c(config);StartRun(run,c,7,"chick",0,p.skills);Check(RunBonus(run,"spear_bonus")==1&&RunBonus(run,"third_attack")==5,"unlocked branch bonuses enter the run");
        const auto profile_file=std::filesystem::path("artifacts/world-profile.json");std::string error;Check(SaveProfile(profile_file,p,error),"profile atomic save");Profile loaded;Check(LoadProfile(profile_file,loaded,config,error)&&loaded.skills==p.skills&&loaded.feathers==p.feathers,"profile currencies and skills round trip");
        run.run_id="world-once";run.victory=true;run.defeated_bosses={"frog","hawk","fox","undergod"};run.metrics["damage"]=1200;const int before=p.feathers;Check(SettleProfile(p,run,config)&&p.feathers>before&&p.highest_challenge==0&&!p.achievements.empty(),"campaign settlement unlocks feathers achievements and challenge one");const int paid=p.feathers;run.meta_claimed=false;Check(!SettleProfile(p,run,config)&&p.feathers==paid,"reload cannot duplicate result currency");
        for(int level=1;level<=10;++level){RunState harder;StartRun(harder,c,7,"chick",level);harder.active_encounter_id="undergod";const auto m=ModsFor(harder,config);Check(m.enemy_hp_percent>=110&&m.enemy_damage>=(level>1?1:0),"challenge tiers have cumulative gameplay penalties");}
        RunState highest;StartRun(highest,c,8,"chick",10);highest.active_encounter_id="undergod";const auto highest_mods=ModsFor(highest,config);highest.hp=10;Check(highest.max_hp==45&&highest.gold==25&&highest.potion_slots==2&&highest_mods.enemy_hp_percent==135&&highest_mods.enemy_damage==4&&RestHealAmount(highest,config)==11,"challenge ten combines HP gold slots recovery and boss penalties");
        bool ended=true;for(const auto& character:config.classes)for(int level=1;level<=10;++level){Game hard(config,timeline);hard.NewRun(1000+level,character.id,level);for(int n=0;n<4000&&!hard.Result();++n){const auto step=hard.Suggest();if(step.kind==AutoKind::None)break;hard.Act(step);Settle(hard);}ended&=hard.Result();}Check(ended,"all three classes reach a result at every challenge tier");
        std::error_code ignored;std::filesystem::remove(profile_file,ignored);
    }
    {
        // Save at each settled decision, especially act transitions and new event states.
        Game g(config,timeline),h(config,timeline);g.NewRun(1337);g.SetSaveFile("artifacts/world-run.json");h.SetSaveFile("artifacts/world-run.json");
        bool okay=true;int decisions=0,actmask=0,places=0;
        for(int n=0;n<2000&&!g.Result();++n){
            actmask|=1<<g.run.act;places|=1<<int(g.run.place);const auto action=g.Suggest();if(action.kind==AutoKind::None){okay=false;break;}
            g.Act(action);Settle(g);++decisions;
            if(!g.Save()||!h.Load()){std::cout<<"CHECKPOINT failure at act="<<g.run.act<<" place="<<int(g.run.place)<<" toast="<<g.toast<<" / "<<h.toast<<"\n";okay=false;break;}
            if(Json(g.run)!=Json(h.run)||Json(g.state.Snapshot())!=Json(h.state.Snapshot())){okay=false;std::cout<<"CHECKPOINT state mismatch "<<Json::diff(Json(g.run),Json(h.run)).dump()<<" combat "<<Json::diff(Json(g.state.Snapshot()),Json(h.state.Snapshot())).dump()<<"\n";break;}
        }
        Check(okay&&g.run.victory&&actmask==7&&decisions>100,"every decision saves and reloads through all three acts to victory");
        Check((places&(1<<int(Place::RelicReward)))!=0,"campaign passed through boss relic reward screen");
        std::error_code ignored;std::filesystem::remove("artifacts/world-run.json",ignored);
    }
}
