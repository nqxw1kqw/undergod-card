#include "ui.hpp"
#include "audio.hpp"
#include <DxLib.h>
#include <filesystem>
#include <fstream>

namespace undergod {
int StarSmoke(Game& game,Ui& ui,Audio& audio){
    const std::filesystem::path folder="artifacts/turn-stars";std::filesystem::create_directories(folder);
    std::ofstream report(folder/"smoke.txt");bool okay=true;
    auto check=[&](bool value,const char* name){report<<name<<"="<<value<<"\n";okay&=value;};
    auto step=[&](float seconds){while(seconds>.0001f){const float dt=std::min(.02f,seconds);game.Update(dt/game.PlaybackRate());audio.Update(game);seconds-=dt;}};
    auto settle=[&](){for(int i=0;i<1000&&game.Busy();++i)step(.02f);check(!game.Busy(),"animation_settled");};
    auto capture=[&](const char* name,int mx=-1,int my=-1){ClearDrawScreen();ui.Draw(game,mx,my);const auto file=(folder/name).string();check(SaveDrawScreenToPNG(0,0,Ui::Width,Ui::Height,file.c_str())==0,name);ScreenFlip();ProcessMessage();};
    check(ui.loaded==ui.expected&&ui.expected==203,"all_203_images_loaded");
    game.NewRun(1337);game.OpenClasses();capture("00-classes.png");game.BackMenu();capture("00-map-player.png");game.ChooseNode(0);step(.4f);
    check(game.Traveling()&&game.run.node==-1&&game.TravelProgress()>0,"map_travel_precedes_entry");capture("01-map-moving.png");
    step(.70f);capture("02-fade-out.png");check(game.ArrivalStage()==Arrival::FadeOut&&game.FadeOpacity()>.4f,"fade_out_before_entry");
    step(.24f);capture("03-black-entry.png");check(game.run.node>=0&&game.FadeOpacity()>.9f,"node_changes_under_black");
    step(.68f);capture("04-fight-enter-right.png");step(.35f);capture("05-fight-center.png");
    check(game.ArrivalStage()==Arrival::Fight&&std::abs(game.FightBannerX()-720)<1&&game.DealtCards()==0,"fight_holds_center_before_dealing");
    step(.67f);capture("06-fight-exit-left.png");step(.3f);capture("07-dealing.png");
    check(game.ArrivalStage()==Arrival::Deal&&game.DealtCards()>0&&game.DealtCards()<5,"cards_deal_progressively");settle();capture("08-hand-ready.png");
    game.StartBossPractice("tiger",1337);settle();
    game.state.hand={"heavy+","snare","shield+","punch+","cookie+"};game.state.live.locked=-1;game.state.energy=0;
    Intent strike;strike.attack=true;strike.kind=IntentKind::Attack;strike.value=6;
    Intent guard;guard.kind=IntentKind::Defend;guard.value=5;
    Intent debuff;debuff.kind=IntentKind::Debuff;debuff.status=StatusId::Weak;debuff.status_amount=1;
    game.state.stars.enemy={{0,strike,false},{0,guard,false},{0,debuff,false}};capture("09-boss-plan.png");
    const int hp=game.state.enemy.hp;
    for(int i=0;i<3;++i){const auto r=Ui::StarHandRect(5-i,0);ui.Click(game,r.x+r.w/2,r.y+r.h/2);}
    check(game.state.stars.player.size()==3&&game.state.enemy.hp==hp&&game.CanAttack(),"three_clicks_queue_without_damage");capture("10-three-player-cards.png");
    const auto enemy=Ui::EnemyStarRect(0);const auto rng=game.state.Snapshot().intent_rng;
    capture("11-boss-card-tooltip.png",enemy.x+20,enemy.y+20);check(game.state.Snapshot().intent_rng==rng,"enemy_tooltip_does_not_reroll");
    const auto attack=Ui::EndButton();ui.Click(game,attack.x+20,attack.y+20);step(.30f);capture("12-player-windup.png");check(game.state.enemy.hp==hp,"no_early_queue_damage");
    step(.04f);check(game.state.enemy.hp==hp-15,"player_card_one_at_contact");capture("13-player-card-one.png");
    for(int i=0;i<100&&game.ActiveActor()<0;++i)step(.02f);
    const int player_hp=game.state.player.hp;step(.69f);capture("14-enemy-lunge.png");check(game.state.player.hp==player_hp,"enemy_windup_has_no_damage");
    step(.10f);check(game.state.player.hp==player_hp-6,"enemy_card_one_after_player_one");capture("15-enemy-card-one.png");
    for(int cursor=3;cursor<=6;++cursor){for(int i=0;i<300&&game.state.stars.cursor<cursor&&game.ResolvingStars();++i)step(.02f);capture(("16-order-"+std::to_string(cursor)+".png").c_str());}
    settle();capture("20-next-round-dealt.png");
    check(game.state.turn==2&&game.CanAct()&&game.state.player.status.weak==1,"six_actions_finish_into_next_round");
    game.StartBossPractice("tiger",1337);settle();game.state.enemy.hp=101;game.state.hand={"punch+","shield+","shield+"};game.state.live.locked=-1;
    game.state.stars.enemy={{0,guard,false},{0,guard,false},{0,guard,false}};
    game.Play(0);game.Play(1);game.Play(2);game.EndTurn();settle();capture("21-tiger-charge-plan.png");
    check(game.state.PhaseIndex()==1&&game.state.live.charge_left==2,"phase_two_has_announced_charge");
    const int charge_hp=game.state.player.hp;for(int i=0;i<3;++i)game.QueueWait();game.EndTurn();
    for(int i=0;i<100&&game.ActiveActor()<0;++i)step(.02f);step(.48f);capture("22-tiger-charge-pose.png");
    check(game.poses[1].name=="ult_charge"&&game.state.player.hp==charge_hp,"charge_card_has_pose_without_early_damage");settle();
    for(int i=0;i<3;++i)game.QueueWait();game.EndTurn();for(int i=0;i<100&&game.ActiveActor()<0;++i)step(.02f);
    step(.69f);capture("23-tiger-pounce.png");step(.10f);check(game.state.player.hp<charge_hp,"pounce_card_hits_after_waiting_round");settle();
    report<<"PASS="<<okay<<"\n";return okay?0:4;
}
}
