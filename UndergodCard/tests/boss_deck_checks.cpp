#include "game.hpp"
#include "quest_json.hpp"
#include <set>
using namespace undergod;
extern void Check(bool,const char*);
namespace {
void Advance(Game& game,float logical){game.Update(logical/game.PlaybackRate());}
void Settle(Game& game){for(int i=0;i<1200&&game.Busy();++i)Advance(game,.05f);}
const BossCardDef* Owned(const Config& config,const std::string& owner,const std::string& id){for(const auto& card:config.boss_cards)if(card.boss_id==owner&&card.id==id)return &card;return nullptr;}
void Waits(Combat& combat){for(int i=0;i<3;++i)combat.QueueWait();Check(combat.BeginStarAttack(),"boss card fixture starts an ordered round");}
void Finish(Combat& combat){for(int i=0;i<6&&combat.stars.resolving&&combat.phase==Phase::Player;++i)combat.ResolveStarAction();combat.FinishStarRound();}
}
void BossDeckChecks(const Config& config,const Timeline& timeline){
    {
        Game game(config,timeline);game.NewRun(1337);const int destination=game.run.options.front();game.ChooseNode(0);
        Advance(game,.84f);Check(game.Traveling()&&game.MapAvatarTo()==destination&&game.MapAvatarProgress()>.98f,"map avatar approaches its selected destination");
        Advance(game,.02f);Check(game.ArrivalStage()==Arrival::FadeOut&&game.run.node==-1&&game.MapAvatarTo()==destination&&game.MapAvatarProgress()==1,"arrival holds the avatar at its destination before committing the node");
        Advance(game,.24f);Check(game.MapAvatarTo()==destination&&game.MapAvatarProgress()==1&&game.FadeOpacity()>.5f,"avatar never resets to entrance during fade out");
        Advance(game,.24f);Check(game.run.node==destination&&game.MapAvatarFrom()==destination&&game.MapAvatarTo()==destination,"black transition hands off to committed node at the same location");
        Settle(game);int attack=-1;for(int i=0;i<int(game.state.hand.size());++i)if(game.state.Definition(game.state.hand[i]).damage>0){attack=i;break;}
        Check(attack>=0,"map return fixture has an attack in its actual hand");
        if(attack>=0){game.state.enemy.hp=1;game.Play(attack);game.QueueWait();game.QueueWait();game.EndTurn();Settle(game);}
        Check(game.Reward(),"map return fixture wins first stage");game.ChooseReward(0);
        Check(game.Map()&&game.run.node==destination&&game.MapAvatarTo()==destination,"returning from a stage preserves the visited map position");
        game.SetSaveFile("artifacts/map-avatar-check.json");Check(game.Save(false),"map position saves after a stage");
        Game loaded(config,timeline);loaded.SetSaveFile("artifacts/map-avatar-check.json");Check(loaded.Load()&&loaded.MapAvatarFrom()==destination&&loaded.MapAvatarTo()==destination,"loading a map preserves the last stage position");
        loaded.ChooseNode(0);Check(loaded.MapAvatarFrom()==destination&&loaded.MapAvatarProgress()==0,"next journey begins at the previous stage rather than entrance");
    }
    std::set<std::string> ids;bool separate=true;
    for(const auto& card:config.boss_cards){separate&=ids.insert(card.id).second;for(const auto& player:config.cards)separate&=player.id!=card.id;}
    Check(config.boss_cards.size()==36&&separate,"36 exclusive boss cards never enter the player reward or shop card pool");
    for(const auto& boss:config.bestiary)if(boss.role=="boss"){
        int count=0;for(const auto& card:config.boss_cards)count+=card.boss_id==boss.id;
        Check(count==6,"each boss owns exactly six named cards");
        for(int phase=0;phase<=int(boss.phases.size());++phase){
            bool valid=true,unique=true,stable=true;std::set<std::string> seen;
            for(int seed=0;seed<24;++seed){
                Combat combat(config);combat.BeginEncounter(boss,seed,5000,5000,0,StartingDeck(config));
                auto snapshot=combat.Snapshot();snapshot.phase_index=phase;snapshot.phase_opening=true;combat.Restore(snapshot,{});combat.EnableTurnStars();
                std::set<std::string> drawn;
                valid&=combat.stars.enemy.size()==3;
                for(int slot=0;slot<int(combat.stars.enemy.size());++slot){
                    const auto& action=combat.stars.enemy[slot].action;const auto* card=Owned(config,boss.id,action.deck_card_id);
                    valid&=card&&phase>=card->min_phase&&phase<=card->max_phase;
                    const auto view=combat.EnemyStarView(slot);valid&=card&&view.id==card->id&&view.name==card->name&&view.rarity=="boss";
                    if(action.kind!=IntentKind::Defend||combat.intent.kind!=IntentKind::Charge)unique&=drawn.insert(action.deck_card_id).second;
                    seen.insert(action.deck_card_id);
                }
                const auto plan=nlohmann::json(combat.stars.enemy).dump();const auto rng=combat.Snapshot().intent_rng;
                for(int i=0;i<3;++i)combat.EnemyStarView(i);
                if(!combat.hand.empty()&&combat.CanQueueStar(0)){combat.QueueStar(0);combat.UnqueueStar(0);}
                stable&=rng==combat.Snapshot().intent_rng&&plan==nlohmann::json(combat.stars.enemy).dump();
                Combat restored(config);restored.Restore(combat.Snapshot(),{});stable&=nlohmann::json(restored.Snapshot()).dump()==nlohmann::json(combat.Snapshot()).dump();
            }
            Check(valid,"every phase draws actual actions and art exclusively from that boss deck");
            Check(unique,"ordinary boss plans draw distinct cards; charge support guards may repeat");
            Check(stable,"boss previews queue editing and save restore never reroll their cards");
            Check(seen.size()>=3||boss.id=="tiger"&&phase==1,"boss plans sample multiple owned cards in each phase");
        }
    }
    for(const auto& card:config.boss_cards){
        Combat combat(config);combat.BeginEncounter(FindEnemy(config,card.boss_id),1337,500,1000,0,StartingDeck(config));combat.EnableTurnStars();
        Intent action;action.deck_card_id=card.id;action.kind=card.action.kind;action.hits=card.action.hits;action.status=card.action.status;
        action.status_amount=(card.action.status_amount+1)/2;action.summon_id=card.action.summon_id;
        action.attack=action.kind==IntentKind::Attack||action.kind==IntentKind::MultiAttack||action.kind==IntentKind::Drain||action.kind==IntentKind::Mimic;
        action.value=action.attack?(card.action.min_damage*35+99)/100:card.action.block;
        if(action.kind==IntentKind::Charge){action.value=card.action.min_damage;combat.live.charge_left=card.action.charge_turns;}
        if(action.kind==IntentKind::Mimic)action.card_id="shield";
        combat.rage=100;combat.intent=action;combat.stars.enemy={{0,action,false},{0,{},true},{0,{},true}};
        const auto view=combat.EnemyStarView(0);Check(view.id==card.id&&view.name==card.name,"each boss card keeps its own illustration and name during its effect");
        Waits(combat);combat.ResolveStarAction();const int hp=combat.player.hp,block=combat.enemy.block,strength=combat.enemy.status.strength,rage=combat.rage;
        combat.ResolveStarAction();
        if(action.kind==IntentKind::Attack||action.kind==IntentKind::MultiAttack||action.kind==IntentKind::Drain)Check(hp-combat.player.hp==view.damage,"owned attack and multi-hit card damage matches its tooltip");
        if(action.kind==IntentKind::Drain)Check(combat.rage==std::max(0,rage-action.status_amount),"owned drain card removes the announced rage");
        if(action.kind==IntentKind::Defend)Check(combat.enemy.block==action.value,"owned guard card grants its announced armor");
        if(action.kind==IntentKind::Buff)Check(combat.enemy.status.strength==strength+action.status_amount&&combat.enemy.block==block+action.value,"owned buff card grants its announced strength and armor");
        if(action.kind==IntentKind::Debuff)Check(StatusOf(combat.player,action.status)==action.status_amount,"owned debuff card applies its announced status");
        if(action.kind==IntentKind::Summon)Check(combat.adds.size()==size_t(action.hits),"owned summon card creates the announced reinforcements");
        if(action.kind==IntentKind::Mimic)Check(combat.enemy.block==view.block&&view.block==5&&view.description.find("Sao chép")!=std::string::npos,"Undergod copies effects while keeping its own card art");
        if(action.kind==IntentKind::Charge){
            Check(combat.player.hp==hp&&combat.live.charge_left==1,"owned charge card telegraphs without early damage");Finish(combat);
            Check(combat.stars.enemy[0].action.deck_card_id==card.id,"charging boss retains the same owned card next round");
            const int damage=combat.EnemyStarView(0).damage;Waits(combat);combat.ResolveStarAction();const int before=combat.player.hp;combat.ResolveStarAction();
            Check(before-combat.player.hp==damage,"owned charged strike hits on the following round with announced damage");
        }else if(action.kind==IntentKind::Curse){Finish(combat);Check(std::find(combat.hand.begin(),combat.hand.end(),"mud")!=combat.hand.end(),"owned fox curse becomes mud in the next hand");}
    }
    {
        Combat combat(config);combat.BeginEncounter(FindEnemy(config,"tiger"),1337,500,500,0,StartingDeck(config));combat.EnableTurnStars();
        auto snapshot=combat.Snapshot();const auto before=nlohmann::json(snapshot).dump();bool rejected=false;
        snapshot.stars.enemy[0].action.deck_card_id="boss_fox_veil";
        try{combat.Restore(snapshot,{});}catch(...){rejected=true;}
        Check(rejected&&nlohmann::json(combat.Snapshot()).dump()==before,"foreign boss card IDs are rejected without changing combat");
        snapshot=combat.Snapshot();snapshot.stars.enemy[0].action.deck_card_id="boss_tiger_missing";rejected=false;
        try{combat.Restore(snapshot,{});}catch(...){rejected=true;}
        Check(rejected,"unknown boss cards cannot enter saved plans");
        snapshot=combat.Snapshot();nlohmann::json old=snapshot;old["intent"].erase("deck_card_id");for(auto& q:old["stars"]["enemy"])q["action"].erase("deck_card_id");
        Combat migrated(config);migrated.Restore(old.get<CombatSnapshot>(),{});
        bool same=migrated.Snapshot().intent_rng==snapshot.intent_rng;
        for(int i=0;i<3;++i){same&=migrated.stars.enemy[i].action.value==snapshot.stars.enemy[i].action.value;same&=Owned(config,"tiger",migrated.EnemyStarView(i).id)!=nullptr;}
        Check(same,"old v1.1 plans retain their announced effects and RNG while receiving owned art");
    }
}
