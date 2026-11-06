#include "ui.hpp"
#include "audio.hpp"
#include <DxLib.h>
#include <filesystem>
#include <fstream>
#include <cmath>

namespace undergod {
int AnimationSmoke(Game& game,Ui& ui,Audio& audio){
    const std::filesystem::path folder="artifacts/skill-animations";std::filesystem::create_directories(folder);
    std::ofstream report(folder/"smoke.txt");bool okay=true;
    auto check=[&](bool value,const std::string& name){report<<name<<"="<<value<<"\n";report.flush();okay&=value;};
    auto step=[&](float seconds){while(seconds>.00001f){const float dt=std::min(.01f,seconds);game.Update(dt/game.PlaybackRate());audio.Update(game);seconds-=dt;}};
    auto settle=[&](){for(int i=0;i<2000&&game.Busy();++i)step(.01f);check(!game.Busy(),"animation_settled");};
    auto capture=[&](const std::string& name){ClearDrawScreen();ui.Draw(game,-1,-1);check(SaveDrawScreenToPNG(0,0,Ui::Width,Ui::Height,(folder/name).string().c_str())==0,name);ScreenFlip();ProcessMessage();};
    Intent idle;idle.kind=IntentKind::Defend;
    auto fixture=[&](const std::string& card,const std::string& cls="chick",const std::string& boss="tiger"){
        game.StartBossPractice(boss,1337,cls);settle();game.state.player.hp=game.state.player.max_hp=500;game.state.enemy.hp=game.state.enemy.max_hp=500;
        game.state.hand={card};game.state.live.locked=-1;game.state.player.block=game.state.enemy.block=0;
        game.state.stars.enemy={{0,idle,true},{0,idle,true},{0,idle,true}};
    };
    auto launch=[&](){const auto r=Ui::StarHandRect(1,0);ui.Click(game,r.x+30,r.y+30);game.QueueWait();game.QueueWait();const auto attack=Ui::EndButton();ui.Click(game,attack.x+20,attack.y+20);};
    auto enemy=[&](const Intent& action){game.state.stars.enemy[0]={0,action,false};for(int i=0;i<3;++i)game.QueueWait();game.EndTurn();for(int n=0;n<100&&game.ActiveActor()<0;++n)step(.01f);};
    check(ui.loaded==379&&ui.loaded==ui.expected,"all_379_runtime_images_loaded");
    fixture("punch");const int spear_hp=game.state.enemy.hp;launch();step(.15f);capture("01-spear-prepare.png");step(.15f);capture("02-spear-thrust.png");
    check(game.state.enemy.hp==spear_hp&&game.SkillVisual().pose=="thrust","spear_windup_before_damage");step(.04f);capture("03-spear-contact.png");check(game.state.enemy.hp<spear_hp,"spear_contact_damage");settle();
    fixture("spellbolt","mage");const int bolt_hp=game.state.enemy.hp;launch();step(.20f);capture("04-mage-release.png");const auto first=game.Projectiles();step(.20f);capture("05-mage-bolt-midair.png");
    check(!first.empty()&&!game.Projectiles().empty()&&game.Projectiles()[0].x>first[0].x+100&&game.state.enemy.hp==bolt_hp&&std::abs(game.ActorOffset(false))<.01f,"mage_projectile_moves_before_hit_without_lunge");step(.20f);capture("06-mage-impact.png");settle();
    fixture("shield");launch();step(.20f);capture("07-player-guard-prepare.png");step(.20f);capture("08-player-shield-cast.png");step(.6f);capture("09-player-shield-persists.png");check(game.UnitBlock(-1)>0&&!game.SkillActor(false),"player_ward_survives_cast");
    game.state.player.block=0;step(.09f);capture("10-player-shield-break.png");check(game.Reaction(-1).break_age<.11f,"player_ward_breaks_when_armor_gone");settle();
    fixture("cookie","mage","fox");Intent fire;fire.attack=true;fire.kind=IntentKind::Attack;fire.value=8;fire.deck_card_id="boss_fox_fire";const int fire_hp=game.state.player.hp;enemy(fire);step(.52f);capture("11-fox-release.png");step(.18f);capture("12-fox-fireball-midair.png");
    check(!game.Projectiles().empty()&&game.state.player.hp==fire_hp&&game.SkillVisual().pose=="cast","boss_fireball_visible_before_contact");step(.20f);capture("13-fox-fireball-impact.png");check(game.state.player.hp==fire_hp-8,"boss_fireball_hits_player");settle();
    fixture("punch");Intent guard;guard.kind=IntentKind::Defend;guard.value=18;guard.deck_card_id="boss_tiger_guard";enemy(guard);step(.50f);capture("14-boss-guard-cast.png");step(.70f);capture("15-boss-shield-persists.png");check(game.UnitBlock(0)==18,"boss_ward_survives_cast");settle();
    fixture("comet_lance");launch();step(.38f);capture("16-flaming-spear-flight.png");step(.35f);capture("17-flaming-spear-hit.png");settle();
    fixture("meteor_rain","mage","fox");launch();step(.40f);capture("18-meteor-rain.png");check(game.Projectiles().size()==6,"six_random_meteors_visible");step(.40f);capture("19-meteor-impact.png");settle();
    fixture("dagger","ninja");launch();step(.30f);capture("20-ninja-slash.png");settle();fixture("blade_rain","ninja");launch();step(.40f);capture("21-ninja-blades.png");settle();
    fixture("punch","chick","frog");Intent tongue=fire;tongue.deck_card_id="boss_frog_tongue";enemy(tongue);step(.61f);capture("22-frog-tongue-out.png");step(.24f);capture("23-frog-tongue-return.png");settle();
    fixture("cookie");Intent pounce=fire;pounce.kind=IntentKind::Charge;pounce.deck_card_id="boss_tiger_pounce";game.state.live.charge_left=1;enemy(pounce);step(.60f);capture("24-tiger-pounce.png");settle();
    // Draw every playable card and every boss action through the actual renderer.
    for(const auto& card:game.state.config.cards)if(!card.upgraded&&!card.unplayable){
        const auto id=card.id,cls=card.class_id=="any"||card.class_id.empty()?"chick":card.class_id;
        fixture(id,cls);launch();step(game.SkillVisual().animation.launch+.10f);ClearDrawScreen();ui.Draw(game,-1,-1);ScreenFlip();ProcessMessage();
        check(game.SkillVisual().active&&game.SkillVisual().card_id==id,"player_binding_"+id);settle();
    }
    const auto decks=game.state.config.boss_cards;
    for(const auto& card:decks){
        fixture("cookie","chick",card.boss_id);Intent action;action.kind=IntentKind::Attack;action.attack=true;action.value=1;action.deck_card_id=card.id;enemy(action);step(.50f);
        ClearDrawScreen();ui.Draw(game,-1,-1);ScreenFlip();ProcessMessage();check(game.SkillVisual().active&&game.SkillVisual().card_id==card.id,"boss_binding_"+card.id);settle();
    }
    report<<"PASS="<<okay<<"\n";return okay?0:1;
}
}
