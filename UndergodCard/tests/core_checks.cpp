#include "game.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <set>
using namespace undergod;
int checks=0,failures=0;
void Check(bool value,const char* name){++checks;if(!value){++failures;std::cout<<"FAIL "<<name<<"\n";}}
void Tick(Game& game,float logical_seconds){game.Update(logical_seconds/game.PlaybackRate());}
void Open(Game& game,unsigned seed=1337){game.NewRun(seed);game.ChooseNode(0);}
void Drain(Game& game){for(int i=0;i<100&&game.Busy();++i)game.Update(.1f);Check(!game.Busy(),"controller settles");}
void Settle(Game& game){for(int i=0;i<120&&game.Busy();++i)game.Update(.1f);}
void RunChecks(const Config&,const Timeline&);
void WorldChecks(const Config&,const Timeline&);
void TigerChecks(const Config&,const Timeline&);
void StarChecks(const Config&,const Timeline&);
void AnimationChecks(const Config&,const Timeline&);
void IntegrationChecks(const Config&,const Timeline&);
void ForecastChecks(const Config&,const Timeline&);
void ForecastBenchmark(const Config&,const Timeline&,int);
void IntegratedBalanceChecks(const Config&,const Timeline&,int);
void BossDeckChecks(const Config&,const Timeline&);
void SimulateStarRuns(const Config&,const Timeline&,int);
void SimulateRuns(const Config&,const Timeline&,int);
int main(int argc,char** argv){
    std::cout<<std::unitbuf;
    std::filesystem::current_path(std::filesystem::absolute(argv[0]).parent_path());
    const auto config=LoadConfig();const auto timeline=LoadTimeline();
    {
        Combat c(config);Check(c.player.hp==50&&c.enemy.hp==config.enemy.hp&&config.enemy.hp>0,"requested HP");Check(c.energy==3&&c.hand.size()==5&&c.draw.size()==5,"opening resources and piles");
        Check(c.hand==std::vector<std::string>({"punch","cookie","shield","punch","punch"}),"opening hand matches TypeScript seed 1337");
        int base_cards=0;for(const auto& card:config.cards)if(!card.upgraded)++base_cards;
        Check(base_cards==50&&config.cards.size()==98&&StartingDeck(config).size()==10&&config.events.size()>=10&&config.relics.size()>=10&&config.potions.size()>=5&&c.config.balance.ultimate_damage==40,"JSON definitions");
        Rng r;r.Seed(42);const double expected[]={.6011037519201636,.44829055899754167,.8524657934904099,.6697340414393693};for(double value:expected)Check(std::abs(r.Next()-value)<1e-12,"RNG matches TypeScript golden values");
        auto legacy=config;legacy.enemy=EnemyDef{};legacy.enemy.opening_attack=false;legacy.enemy.max_guards=100;Combat original(legacy);
        const bool attacks[]={false,false,false,false,false,true,false,false};for(bool attack:attacks){Check(original.intent.attack==attack,"legacy intent RNG matches TypeScript");if(attack)Check(original.intent.value==8,"legacy intent damage golden value");original.EndPlayer();original.ActEnemy();}
    }
    {
        Fighter f{10,10,5,{}};auto hit=ResolveDamage(f,8);Check(hit.lost==3&&hit.blocked==5&&f.hp==7&&f.block==0,"damage through armor");hit=ResolveDamage(f,99);Check(hit.lost==7&&f.hp==0,"lethal clamps actual HP loss");
        f={10,10,5,{}};hit=ResolveDamage(f,-2);Check(hit.lost==0&&f.block==5&&f.hp==10,"negative damage cannot heal or grant block");
    }
    {
        Combat c(config);c.hand={"cookie"};c.player.hp=48;c.Play(0);Check(c.player.hp==50&&c.energy==1,"heal caps and pays cost");
        c.Reset();c.hand={"heavy"};c.energy=1;const int untouched=c.enemy.hp;Check(!c.Play(0)&&c.hand.size()==1&&c.enemy.hp==untouched,"insufficient energy does nothing");Check(!c.Play(-1)&&!c.Play(99),"invalid card indices");
        c.Reset();c.hand={"punch","shield"};c.rage=98;c.Play(0);Check(c.rage==100,"attack rage cap");c.Play(0);Check(c.rage==100&&c.player.block==5,"defense rage cap");
        c.Reset();c.hand={"punch","shield"};c.Play(0);c.Play(0);Check(c.rage==13,"attack and defense rage gains");
    }
    {
        Combat c(config);c.player.block=20;c.intent={true,8};c.EndPlayer();Check(!c.Play(0)&&!c.EndPlayer(),"enemy phase rejects player actions");c.ActEnemy();Check(c.player.hp==50&&c.player.block==0&&c.energy==3&&c.turn==2,"block survives enemy hit then clears on player start");
        c.Reset();c.intent={false,6};c.EndPlayer();c.ActEnemy();Check(c.enemy.block==6,"enemy block survives into player phase");c.intent={true,8};c.EndPlayer();c.ActEnemy();Check(c.enemy.block==0,"enemy block clears on own action");
        c.Reset();c.hand={"punch"};c.enemy.hp=1;c.Play(0);Check(c.phase==Phase::Won&&!c.EndPlayer(),"victory and terminal lock");
        c.Reset();c.player.hp=1;c.intent={true,8};c.EndPlayer();c.ActEnemy();Check(c.phase==Phase::Lost&&c.player.hp==0,"defeat");
    }
    {
        Combat c(config);c.hand.clear();c.draw={"punch"};c.discard={"shield","cookie"};c.Draw(5);std::multiset<std::string> got(c.hand.begin(),c.hand.end());Check(got==std::multiset<std::string>({"punch","shield","cookie"})&&c.discard.empty()&&c.draw.empty(),"draw refills from discard without duplicating cards");
        c.Reset();c.player.hp=c.player.max_hp=500;for(int n=0;n<8;++n){if(c.CanPlay(0))c.Play(0);c.EndPlayer();c.ActEnemy();Check(c.hand.size()+c.draw.size()+c.discard.size()==10,"deck conserves cards across repeated reshuffles");}
        Combat a(config),b(config);a.Draw(20);a.EndPlayer();a.ActEnemy();b.EndPlayer();b.ActEnemy();Check(a.intent.attack==b.intent.attack&&a.intent.value==b.intent.value,"shuffle stream cannot perturb intent RNG");
    }
    {
        Combat c(config);Check(!c.StartUltimate(),"ultimate requires full rage");c.rage=100;c.enemy.hp=120;c.enemy.block=6;Check(c.StartUltimate()&&c.rage==0&&!c.Play(0),"ultimate consumes rage and locks cards");Check(c.enemy.hp==120,"no ultimate damage at start");c.UltimateImpact();Check(!c.UltimateImpact()&&c.enemy.hp==86,"ultimate impact resolves once through armor");c.FinishUltimate();Check(c.phase==Phase::Player,"ultimate releases input after end");
        c.Reset();c.rage=100;c.enemy.hp=120;c.StartUltimate();c.FinishUltimate();c.FinishUltimate();Check(c.enemy.hp==80,"skip before impact resolves once");
    }
    {
        TimelinePlayer player;int hits=0,ends=0,cues=0;player.Start(timeline,[&](const Step&){++cues;},[&]{++hits;},[&]{++ends;});player.Advance(2599);Check(hits==0,"timeline waits for 2600ms impact");player.Advance(2600);Check(hits==1,"timeline impact exact boundary");player.Advance(5000);player.Skip();Check(hits==1&&ends==1&&cues==int(std::count_if(timeline.steps.begin(),timeline.steps.end(),[](const Step& s){return s.cue!=Cue::Impact&&s.cue!=Cue::End;})),"timeline ends once after skipped frames");
        for(float t:{0.f,900.f,2600.f,3400.f}){hits=ends=0;player.Start(timeline,[](const Step&){},[&]{++hits;},[&]{++ends;});player.Advance(t);player.Skip();player.Advance(9999);Check(hits==1&&ends==1,"skip before or after impact is idempotent");}
    }
    {
        Game g(config,timeline,false);Open(g);const int opening_hp=g.state.enemy.hp;g.Play(0);Check(g.Busy()&&g.state.enemy.hp==opening_hp,"lunge defers resolution");Tick(g,.24f);Check(g.state.enemy.hp==opening_hp,"windup has no damage");Tick(g,.11f);Check(g.state.enemy.hp==opening_hp-6&&g.state.energy==2,"contact applies card");g.Play(0);Check(g.state.enemy.hp==opening_hp-6,"animation locks input");Drain(g);
        g.EndTurn();Tick(g,.8f);Check(g.Busy()&&g.state.turn==2,"controller holds lock through enemy recovery");Drain(g);
        Open(g);g.state.rage=100;g.state.enemy.hp=120;g.state.enemy.block=6;g.Ultimate();Tick(g,2.59f);Check(g.state.enemy.hp==120&&g.Cinematic(),"cinematic delay");Tick(g,.02f);Check(g.state.enemy.hp==86,"cinematic callback applies damage");g.SkipCinematic();Check(g.CanAct()&&g.visual.zoom==1&&g.visual.dim==0&&g.state.enemy.hp==86,"skip cleans camera and unlocks once");
        Open(g);g.state.rage=100;g.state.enemy.hp=10;g.Ultimate();g.SkipCinematic();Check(g.Reward()&&!g.Result()&&g.state.phase==Phase::Won,"skipped lethal ultimate opens a card reward");
        Open(g);g.state.rage=100;g.Ultimate();Open(g);Tick(g,5);Check(g.state.enemy.hp==g.state.enemy.max_hp&&g.CanAct()&&g.state.rage==0,"reset cancels pending cinematic safely");
        Game a(config,timeline,false),b(config,timeline,false);a.Play(0);b.Play(0);Tick(a,10);for(int i=0;i<100;++i)Tick(b,.1f);Check(a.state.enemy.hp==b.state.enemy.hp&&a.state.energy==b.state.energy,"frame cadence does not alter card outcome");
    }
    {
        Game g(config,timeline,false);g.state.rage=100;g.Ultimate();Tick(g,1.5f);
        Check(g.visual.spear.visible&&g.visual.spear.falling&&std::abs(g.visual.spear.age-.05f)<.001f&&std::abs(g.poses[0].age-1.25f)<.001f,"skipped frames sample spear trajectory and throw pose at absolute time");
        g.SkipCinematic();Open(g);const int blocked_hp=g.state.enemy.hp;g.state.enemy.block=10;g.Play(0);Tick(g,.4f);
        Check(g.state.enemy.hp==blocked_hp&&!g.popups.empty()&&g.popups.back().blocked&&g.popups.back().amount==6,"fully blocked attack shows absorbed damage instead of HP loss");
    }
    {
        bool opening=true,streak_ok=true,range_ok=true;
        Config legacy_policy=config;legacy_policy.enemy=EnemyDef{};legacy_policy.enemy.opening_attack=true;legacy_policy.enemy.max_guards=1;
        for(unsigned seed=0;seed<100;++seed){Combat c(legacy_policy);c.player.hp=c.player.max_hp=10000;opening&=c.intent.attack;int guards=0;
            for(int turn=0;turn<20;++turn){guards=c.intent.attack?0:guards+1;streak_ok&=guards<=1;if(c.intent.attack)range_ok&=c.intent.value>=8&&c.intent.value<=13;c.EndPlayer();c.ActEnemy();}}
        Check(opening,"legacy boss opens with attack across seeds");Check(streak_ok,"legacy boss never spends consecutive turns guarding");Check(range_ok,"legacy boss attack damage stays within configured range");
        Game g(config,timeline,false);const int incoming=g.state.intent.value;g.EndTurn();Tick(g,.69f);
        Check(g.poses[1].name=="attack"&&Game::EnemyHome+g.ActorOffset(true)-Game::PlayerHome<=175,"boss moves into contact range");
        Check(g.state.player.hp==50,"boss windup does not apply damage early");Tick(g,.09f);
        Check(g.state.player.hp==50-incoming&&g.poses[0].name=="hit","boss impact damages player and starts hit reaction");Tick(g,.6f);
        Check(g.CanAct()&&std::abs(g.ActorOffset(true))<.01f,"boss returns home and releases input");
        Open(g);g.Play(0);Tick(g,.24f);Check(Game::EnemyHome-(Game::PlayerHome+g.ActorOffset(false))<=165,"player spear lunge reaches opponent");
        Check(g.state.config.player_id=="chick","player asset identity comes from JSON");
    }
    {
        float durations[3]{};bool same_turn=true;Combat turn_reference(config);
        for(int speed=1;speed<=3;++speed){Game g(config,timeline,false);Open(g);g.SetSpeed(speed);g.Play(0);
            for(int i=0;i<300&&g.Busy();++i){g.Update(.01f);durations[speed-1]+=.01f;}
            Check(!g.Busy()&&g.state.enemy.hp==g.state.enemy.max_hp-6&&g.state.energy==2,"speed changes duration without altering card result");
            g.EndTurn();for(int i=0;i<500&&g.Busy();++i)g.Update(.01f);
            if(speed==1)turn_reference=g.state;
            same_turn&=!g.Busy()&&g.state.player.hp==turn_reference.player.hp&&g.state.enemy.hp==turn_reference.enemy.hp&&g.state.energy==turn_reference.energy&&g.state.hand==turn_reference.hand&&g.state.intent.attack==turn_reference.intent.attack&&g.state.intent.value==turn_reference.intent.value;
        }
        Check(same_turn,"all speeds preserve boss damage, next hand, energy and RNG intent");
        Check(durations[0]>.95f&&durations[0]<1.05f,"default x1 has a slower readable action");
        Check(std::abs(durations[0]-durations[1]*2)<.035f&&std::abs(durations[0]-durations[2]*3)<.04f,"x2 and x3 scale the whole action clock");
        Game g(config,timeline,false);Check(g.Speed()==1&&!g.SetSpeed(0)&&!g.SetSpeed(4)&&g.Speed()==1,"speed defaults to x1 and rejects invalid values");
        g.SetSpeed(3);Open(g);Check(g.Speed()==1,"new run starts at slow x1");
    }
    {
        bool flight=true,impact=true,cleanup=true,skips=true;
        for(int speed=1;speed<=3;++speed){Game g(config,timeline,false);g.SetSpeed(speed);g.state.rage=100;g.state.enemy.hp=120;g.state.enemy.block=6;g.Ultimate();
            Tick(g,.7f);flight&=!g.visual.spear.visible&&g.poses[0].name=="ult_throw";
            Tick(g,.1f);flight&=g.visual.spear.visible&&!g.visual.spear.falling&&g.SpearTip().second<110;
            Tick(g,1.f);flight&=g.visual.spear.falling&&std::abs(g.SpearTip().first-Game::EnemyHome)<.01f;
            Tick(g,.79f);impact&=g.state.enemy.hp==120&&g.SpearTip().second<135;
            Tick(g,.02f);impact&=g.state.enemy.hp==86&&std::abs(g.SpearTip().second-135)<.01f;
            Tick(g,1.1f);cleanup&=g.CanAct()&&!g.visual.spear.visible&&g.visual.zoom==1;
            for(float t:{0.f,.95f,2.55f,2.8f}){Open(g);g.SetSpeed(speed);g.state.rage=100;g.state.enemy.hp=120;g.Ultimate();Tick(g,t);g.SkipCinematic();g.SkipCinematic();skips&=g.state.enemy.hp==80&&g.CanAct()&&!g.visual.spear.visible;}
        }
        Check(flight,"throw releases spear, rises into sky then falls over boss at every speed");
        Check(impact,"damage occurs exactly at spear contact at every speed");
        Check(cleanup&&skips,"normal end and all skip phases clean spear and resolve once");
        Game g(config,timeline,false);g.state.rage=100;g.state.enemy.hp=120;g.Ultimate();Tick(g,2.f);const auto before=g.SpearTip();g.SetSpeed(3);
        Check(g.SpearTip()==before&&g.state.enemy.hp==120,"changing speed mid-flight causes no teleport or damage");
        Tick(g,.61f);Check(g.state.enemy.hp==80,"changed speed still reaches the single impact");
        Open(g);Check(!g.visual.spear.visible&&g.Speed()==1&&g.CanAct(),"reset cancels projectile and restores default speed");
    }
    {
        Fighter poisoned{20,20,9,{}};poisoned.status.poison=4;auto hit=ResolveDamage(poisoned,4,true);Check(hit.lost==4&&hit.blocked==0&&poisoned.block==9&&poisoned.hp==16,"poison damage ignores block");
        Check(ScaleDamage(6,StatusSet{},StatusSet{})==6,"unmodified damage stays exact");
        StatusSet strong;strong.strength=2;Check(ScaleDamage(6,strong,StatusSet{})==8,"strength adds before the hit");
        StatusSet weak;weak.weak=1;Check(ScaleDamage(6,weak,StatusSet{})==4,"weak reduces damage to three quarters");
        StatusSet vulnerable;vulnerable.vulnerable=1;Check(ScaleDamage(6,StatusSet{},vulnerable)==9,"vulnerable increases damage by half");
        Combat c(config);c.player.status.weak=1;c.hand={"punch"};c.energy=1;const int max_hp=c.enemy.max_hp;c.Play(0);Check(c.enemy.hp==max_hp-4&&c.player.status.weak==1,"weak card damage and the stack stays through the player turn");
        c.Reset();c.enemy.status.vulnerable=1;c.hand={"punch"};c.energy=1;c.Play(0);Check(c.enemy.hp==c.enemy.max_hp-9,"vulnerable boosts a later hit in the same turn");
        c.Reset();c.player.status.strength=2;c.hand={"punch"};c.energy=1;c.Play(0);Check(c.enemy.hp==c.enemy.max_hp-8,"strength boosts the starter attack");
        c.Reset();c.enemy.block=5;c.hand={"double_peck"};c.energy=1;c.Play(0);Check(c.enemy.hp==c.enemy.max_hp-4&&c.enemy.block==0,"multi-hit spends block one strike at a time");
        c.Reset();c.hand={"venom_beak"};c.energy=1;c.Play(0);Check(c.enemy.status.poison==3&&c.enemy.hp==c.enemy.max_hp-5,"poison card deals damage and a stack");
        c.enemy.hp=20;c.enemy.block=8;c.EndPlayer();c.ActEnemy();Check(c.enemy.status.poison==2&&c.enemy.hp==17,"poison ticks at the start of the enemy turn and then decays");
        c.Reset();c.player.hp=40;c.enemy.status.weak=1;c.intent={true,8};c.EndPlayer();c.ActEnemy();Check(c.player.hp==34&&c.enemy.status.weak==0,"enemy weak reduces the shown attack and then expires");
        c.Reset();c.player.status.vulnerable=1;c.intent={true,8};c.EndPlayer();c.ActEnemy();Check(c.player.hp==50-12&&c.player.status.vulnerable==0,"vulnerable already on the player boosts the enemy hit");
        c.Reset();c.intent=Intent{};c.intent.attack=true;c.intent.kind=IntentKind::Attack;c.intent.value=5;c.intent.status=StatusId::Vulnerable;c.intent.status_amount=1;c.player.hp=40;c.EndPlayer();c.ActEnemy();Check(c.player.hp==35&&c.player.status.vulnerable==1,"a new vulnerable stack survives the action that applied it");
        c.Reset();const int draw_before=int(c.draw.size());c.hand={"feather"};c.energy=2;c.Play(0);Check(c.player.block==4&&c.hand.size()==1&&int(c.draw.size())==draw_before-1,"block and draw resolve from the effect list");
        CardDef surge;surge.id="surge";surge.cost=0;surge.effects={{EffectOp::Energy,EffectTarget::Self,StatusId::Strength,1,1},{EffectOp::Draw,EffectTarget::Self,StatusId::Strength,1,1}};c.config.cards.push_back(surge);c.Reset();c.hand={"surge"};const int energy_before=c.energy;c.Play(0);Check(c.energy==energy_before+1&&c.hand.size()==1,"energy and draw effects are data driven");
        StatusSet decay;decay.weak=2;decay.vulnerable=1;decay.strength=3;decay.DecayDebuffs();Check(decay.weak==1&&decay.vulnerable==0&&decay.strength==3,"only weak and vulnerable decay");
    }
    {
        EnemyDef old_boss=FindEnemy(config,"frog");old_boss.phases[0].threshold=0;old_boss.phases[0].summon_count=0;old_boss.phases[0].lock_card=false;const EnemyDef* boss=&old_boss;
        Check(boss&&boss->phases.size()==1,"boss definition carries a second phase");
        Combat c(config);c.BeginEncounter(*boss,7,50,50,0,StartingDeck(config));
        Check(c.PhaseCount()==2&&c.PhaseIndex()==0&&c.PhaseName()=="Ếch Vương","boss starts in phase one");
        Check(c.intent.attack&&c.IntentText().find("Sắp tấn công")!=std::string::npos,"boss intent is visible text");
        const int phase_two_hp=boss->phases[0].hp,enrage=boss->phases[0].strength;
        c.enemy.status.poison=4;c.enemy.hp=1;c.hand={"punch"};c.energy=3;c.Play(0);
        Check(c.phase==Phase::Player&&c.PhaseIndex()==1&&c.enemy.hp==phase_two_hp&&c.enemy.status.strength==enrage,"crossing phase one refills the next phase");
        Check(c.PhaseName()=="Ếch Vương Cuồng Nộ"&&c.enemy.status.poison==0,"phase two clears leftover poison and announces its name");
        c.enemy.hp=1;c.hand={"punch"};c.energy=3;c.Play(0);Check(c.phase==Phase::Won&&c.PhaseIndex()==1,"phase two can actually end the fight");
        bool bounded=true;
        for(unsigned seed=0;seed<20;++seed){Combat fight(config);fight.BeginEncounter(*boss,seed,80,80,0,StartingDeck(config));
            for(int turn=0;turn<8&&fight.phase==Phase::Player;++turn){
                const auto& patterns=fight.PhaseIndex()==0?boss->patterns:boss->phases[0].patterns;
                if(fight.intent.attack){int lo=99,hi=0;for(const auto& pattern:patterns)if(pattern.kind==IntentKind::Attack){lo=std::min(lo,pattern.min_damage);hi=std::max(hi,pattern.max_damage);}bounded&=fight.intent.value>=lo&&fight.intent.value<=hi;}
                fight.player.hp=80;
                if(turn==3&&fight.PhaseIndex()==0){fight.enemy.hp=1;fight.hand={"punch"};fight.energy=1;fight.Play(0);}
                if(fight.phase!=Phase::Player)break;
                fight.EndPlayer();fight.ActEnemy();
            }}
        Check(bounded,"boss intents stay inside the active phase table");
        Combat spill(config);spill.BeginEncounter(*boss,3,50,50,0,StartingDeck(config));spill.enemy.hp=1;spill.hand={"double_peck"};spill.energy=1;spill.Play(0);
        Check(spill.PhaseIndex()==1&&spill.phase==Phase::Player&&spill.enemy.hp==boss->phases[0].hp-6,"the hit after a phase change strikes the new phase");
    }
    {
        auto corridor=config;corridor.map_weights={100,0,0,0,0};
        Game g(corridor,timeline,false);Open(g,1337);g.state.player.hp=37;g.state.player.block=9;g.state.rage=40;g.state.enemy.hp=1;g.state.hand={"punch"};g.state.energy=1;g.Play(0);Drain(g);
        Check(g.Reward()&&g.run.rewards.size()==3&&g.run.hp==37&&g.run.rage==48,"winning a fight offers three cards and banks hp");
        std::set<std::string> offer(g.run.rewards.begin(),g.run.rewards.end());Check(offer.size()==3,"reward choices are distinct");
        bool reward_cards=true;for(const auto& id:g.run.rewards){const auto& card=g.state.Definition(id);reward_cards&=card.rarity!="starter"&&!card.upgraded;}Check(reward_cards,"starter cards stay out of the reward pool");
        const int deck=int(g.run.deck.size()),gold=g.run.gold;g.ChooseReward(0);
        Check(g.Map()&&g.state.player.hp==37&&g.run.rage==48&&int(g.run.deck.size())==deck+1&&g.run.gold==gold,"reward returns to the map with hp, rage and gold intact");
        int fight=-1;for(int i=0;i<int(g.run.options.size());++i)if(IsFightNode(g.run.map[g.run.options[i]].kind))fight=i;
        Check(fight>=0,"a fight remains among the next paths");
        g.ChooseNode(fight);Check(g.CanAct()&&g.state.player.hp==37&&g.state.player.block==0&&g.state.rage==48,"the next fight keeps hp and rage but clears block");
    }
    {
        const auto map=GenerateMap(config,1337);
        bool branched=false,boss=false,kinds[6]={};int reachable=0;
        std::vector<char> seen(map.size());
        std::vector<int> stack;for(int i=0;i<int(map.size());++i)if(map[i].floor==0)stack.push_back(i);
        while(!stack.empty()){const int id=stack.back();stack.pop_back();if(seen[id])continue;seen[id]=1;++reachable;if(map[id].next.size()>1)branched=true;if(map[id].kind==NodeKind::Boss)boss=true;kinds[int(map[id].kind)]=true;for(int next:map[id].next)stack.push_back(next);}
        Check(map.size()>6&&branched&&boss&&reachable==int(map.size()),"every node is reachable and the map branches");
        Check(kinds[int(NodeKind::Combat)]&&kinds[int(NodeKind::Elite)]&&kinds[int(NodeKind::Event)]&&kinds[int(NodeKind::Shop)]&&kinds[int(NodeKind::Rest)]&&kinds[int(NodeKind::Boss)],"the map contains every node type");
        Check(map.front().kind==NodeKind::Combat&&map.front().encounter_id=="froglet","the first path starts with Ếch Con");
    }
    {
        Game g(config,timeline,false);g.NewRun(1);g.run.hp=20;g.run.gold=30;g.run.place=Place::Event;g.run.event_choices=config.events[0].choices;
        g.ChooseEvent(1);Check(g.Map()&&g.run.hp==35,"the well heals when you drink");
        g.NewRun(1);g.run.place=Place::Shop;g.run.gold=200;g.run.shop.push_back({Ware::Card,"peck",50,false});
        Check(g.Buy(0)&&g.run.gold==150&&g.run.deck.back()=="peck"&&g.run.shop[0].sold,"the shop sells a card");
        const int before=int(g.run.deck.size());g.run.gold=config.remove_price;Check(g.RemoveCard(0)&&int(g.run.deck.size())==before-1,"the shop can remove a card");
        g.run.place=Place::Rest;g.run.rest_used=false;g.run.hp=10;g.run.max_hp=50;const int healed=g.run.hp;g.RestHeal();Check(g.Map()&&g.run.hp>healed,"rest heals and returns to the map");
        g.NewRun(1);g.run.place=Place::Rest;g.run.rest_used=false;const auto upgraded=FindCard(config,"punch").upgrade_id;Check(g.UpgradeCard(0)&&g.run.deck[0]==upgraded,"rest upgrades a card");
        Combat mods(config);CombatMods bonus;bonus.bonus_damage=2;mods.BeginEncounter(config.enemy,3,40,40,0,StartingDeck(config),bonus);mods.hand={"punch"};mods.energy=1;const int hp=mods.enemy.hp;mods.Play(0);Check(mods.enemy.hp==hp-8,"relic damage is added inside the effect path");
        mods.player.hp=20;mods.Cast(FindPotion(config,"serum").effects);Check(mods.player.hp==35,"a potion heal uses the effect list");
        Check(FindCard(config,"punch+").damage==9&&FindCard(config,"shield+").block==8,"upgrades raise the printed numbers");
    }
    {
        Game g(config,timeline,false);g.NewRun(1337);int rewards=0;std::vector<int> carried;
        for(int step=0;step<900&&!g.Result();++step){
            if(g.Busy()){Settle(g);continue;}
            if(g.Reward())carried.push_back(g.state.player.hp);
            const auto step_choice=g.Suggest();
            if(step_choice.kind==AutoKind::None)break;
            if(step_choice.kind==AutoKind::Reward)++rewards;
            const bool animated=step_choice.kind==AutoKind::Play||step_choice.kind==AutoKind::EndTurn||step_choice.kind==AutoKind::Ultimate||step_choice.kind==AutoKind::Potion;
            g.Act(step_choice);
            if(animated)Settle(g);
        }
        std::cout<<"RUN victory="<<g.run.victory<<" hp="<<g.run.hp<<" rewards="<<rewards<<" deck="<<g.run.deck.size()<<" gold="<<g.run.gold<<" relics="<<g.run.relics.size()<<" phase="<<(g.state.PhaseIndex()+1)<<" node="<<g.run.node<<"\n";
        Check(g.run.victory&&g.run.hp>0&&g.run.hp<=g.run.max_hp,"seed 1337 clears all three acts with surviving HP");
        Check(g.state.PhaseCount()==3&&g.state.PhaseIndex()==2&&g.run.act==2,"the final blow lands on Undergod phase three in act three");
        Check(rewards>=4&&g.run.deck.size()>=StartingDeck(config).size()&&g.run.relics.size()>=1,"rewards, deck growth and a relic happened");
        Check(!carried.empty(),"hp is banked on the reward screen");
    }
    RunChecks(config,timeline);
    WorldChecks(config,timeline);
    TigerChecks(config,timeline);
    try{StarChecks(config,timeline);}catch(const std::exception& error){std::cout<<"TURN_STAR_EXCEPTION "<<error.what()<<"\n";Check(false,"Turn Star tests must not throw");}
    try{BossDeckChecks(config,timeline);}catch(const std::exception& error){std::cout<<"BOSS_DECK_EXCEPTION "<<error.what()<<"\n";Check(false,"Boss deck tests must not throw");}
    try{AnimationChecks(config,timeline);}catch(const std::exception& error){std::cout<<"ANIMATION_EXCEPTION "<<error.what()<<"\n";Check(false,"Animation checks must not throw");}
    try{IntegrationChecks(config,timeline);}catch(const std::exception& error){std::cout<<"INTEGRATION_EXCEPTION "<<error.what()<<"\n";Check(false,"Integration checks must not throw");}
    try{ForecastChecks(config,timeline);}catch(const std::exception& e){std::cout<<"FORECAST_EXCEPTION "<<e.what()<<"\n";Check(false,"forecast checks must not throw");}
    if(argc==3&&std::string(argv[1])=="--forecast-benchmark")ForecastBenchmark(config,timeline,std::clamp(std::stoi(argv[2]),10,10000));
    if(argc==3&&std::string(argv[1])=="--simulate")SimulateRuns(config,timeline,std::clamp(std::stoi(argv[2]),1,10000));
    if(argc==3&&std::string(argv[1])=="--simulate-stars")try{SimulateStarRuns(config,timeline,std::clamp(std::stoi(argv[2]),1,10000));}catch(const std::exception& error){std::cout<<"STAR_CAMPAIGN_EXCEPTION "<<error.what()<<"\n";Check(false,"Turn Star campaign must not throw");}
    if(argc==3&&std::string(argv[1])=="--integrate-balance")IntegratedBalanceChecks(config,timeline,std::clamp(std::stoi(argv[2]),1,1000));
    std::cout<<"RESULT "<<checks<<" checks, "<<failures<<" failures\n";return failures?1:0;
}
