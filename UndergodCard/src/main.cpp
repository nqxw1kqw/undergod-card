#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <DxLib.h>
#include "ui.hpp"
#include "audio.hpp"
#include <filesystem>
#include <fstream>
#include <cmath>
#include <sstream>
namespace undergod { int StarSmoke(Game&,Ui&,Audio&); }

namespace {
std::filesystem::path ExeDirectory(){wchar_t path[32768]{};GetModuleFileNameW(nullptr,path,32768);return std::filesystem::path(path).parent_path();}
void Tick(undergod::Game& game,float logical_seconds){game.Update(logical_seconds/game.PlaybackRate());}
void Drain(undergod::Game& game){for(int i=0;i<100&&game.Busy();++i)game.Update(.1f);}
bool ReachSmoke(undergod::Game& game,undergod::NodeKind kind){
    using namespace undergod;
    for(unsigned seed=0;seed<30;++seed){
        game.NewRun(seed);int target=-1;const auto& map=game.run.map;
        for(int i=0;i<int(map.size());++i)if(map[i].kind==kind){target=i;break;}
        if(target<0)continue;
        std::vector<int> parents(map.size(),-2);for(int i=0;i<int(map.size());++i)if(map[i].floor==0)parents[i]=-1;
        for(int i=0;i<int(map.size());++i)if(parents[i]!=-2)for(int next:map[i].next)if(parents[next]==-2)parents[next]=i;
        std::vector<int> path;for(int at=target;at>=0;at=parents[at])path.push_back(at);std::reverse(path.begin(),path.end());
        for(int id:path){
            if(!game.Map())break;
            const auto option=std::find(game.run.options.begin(),game.run.options.end(),id);if(option==game.run.options.end())break;
            game.ChooseNode(int(option-game.run.options.begin()));if(id==target)return true;
            for(int n=0;n<400&&!game.Map()&&!game.Result();++n){const auto step=game.Suggest();if(step.kind==AutoKind::None)break;game.Act(step);Drain(game);}
        }
    }
    return false;
}
int Smoke(undergod::Game& game,undergod::Ui& ui,undergod::Audio& audio){
    using namespace undergod;std::filesystem::create_directories("artifacts");std::ofstream report("artifacts/smoke.txt");bool okay=true;
    auto check=[&](bool result,const char* label){report<<label<<"="<<result<<"\n";okay&=result;};
    auto capture=[&](const char* name){ClearDrawScreen();ui.Draw(game,-1,-1);const auto file=std::string("artifacts/")+name;check(SaveDrawScreenToPNG(0,0,Ui::Width,Ui::Height,file.c_str())==0,name);ScreenFlip();ProcessMessage();};
    auto click=[&](Rect r){ui.Click(game,r.x+r.w/2,r.y+r.h/2);};
    report<<"assets="<<ui.loaded<<"/"<<ui.expected<<"\n";check(ui.loaded==ui.expected,"all_assets");
    report<<"audio="<<audio.loaded<<"/"<<audio.expected<<"\n";check(audio.loaded==audio.expected&&audio.expected==12,"all_audio");
    audio.Update(game);check(audio.Play("hit")&&audio.Playing("hit"),"audio_playback");
    game.ToggleAudio();audio.Update(game);check(!audio.Playing("music_0")&&!audio.Playing("hit")&&!audio.Play("hit"),"audio_mute");game.ToggleAudio();audio.Update(game);check(audio.Playing("music_0"),"audio_unmute");
    game.NewRun();capture("00-map.png");check(game.Map()&&game.run.options.size()>=2,"map_opening");click(Ui::MapUpButton());click(Ui::MapUpButton());capture("00-map-boss.png");ui.Page(game,-100);click(Ui::MapChoiceRect(0,int(game.run.options.size())));
    const int opening_hp=game.state.enemy.hp;capture("01-battle.png");check(game.CanAct()&&game.state.hand.size()==5&&game.state.hand[0]=="punch"&&game.state.player.hp==50&&game.state.enemy.hp==opening_hp,"opening");
    check(game.Speed()==1,"default_speed_x1");click(Ui::SpeedButton(2));check(game.Speed()==2,"speed_x2_button");click(Ui::SpeedButton(3));check(game.Speed()==3,"speed_x3_button");click(Ui::SpeedButton(1));
    game.state.energy=0;click(Ui::CardRect(5,0));check(!game.toast.empty()&&game.state.enemy.hp==opening_hp,"unaffordable_toast");game.state.energy=3;
    click(Ui::CardRect(5,0));check(game.Busy(),"attack_locks_input");Tick(game,.24f);capture("02-lunge.png");check(game.state.enemy.hp==opening_hp,"no_damage_before_contact");
    Tick(game,.11f);capture("03-hit.png");Drain(game);check(game.state.enemy.hp==opening_hp-6&&game.state.energy==2,"damage_at_contact");
    const int incoming=game.state.intent.value,player_hp=game.state.player.hp;
    click(Ui::EndButton());const auto hp=game.state.enemy.hp;game.Play(0);check(game.Busy()&&game.state.enemy.hp==hp,"enemy_input_lock");
    Tick(game,.69f);capture("04-boss-lunge.png");check(game.state.player.hp==player_hp&&game.ActorOffset(true)<-600,"boss_windup_and_contact_range");
    Tick(game,.09f);capture("04-boss-impact.png");check(game.state.player.hp==player_hp-incoming,"boss_deals_damage_at_contact");
    Drain(game);check(game.state.turn==2&&std::abs(game.ActorOffset(true))<.01f,"next_turn_and_boss_returns");capture("04-boss-recovery.png");
    for(int mode=0;mode<3;++mode){
        click(Ui::SpeedButton(mode+1));game.state.rage=100;game.state.enemy.hp=game.state.enemy.max_hp=120;game.state.enemy.block=6;click(Ui::UltimateButton());check(game.Cinematic(),"ultimate_started");
        if(mode==0){
            Tick(game,.7f);capture("05-spear-windup.png");click(Ui::SpeedButton(2));check(game.Speed()==2&&game.Cinematic(),"speed_button_during_cinematic");
            Tick(game,.25f);capture("05-spear-rise.png");check(game.visual.spear.visible&&!game.visual.spear.falling,"spear_launches_into_sky");
            click(Ui::SpeedButton(3));Tick(game,1.6f);capture("05-spear-fall.png");check(game.state.enemy.hp==120&&game.visual.spear.falling,"fall_waits_for_contact");
            Tick(game,.06f);capture("06-impact.png");Drain(game);
        }
        else if(mode==1)click(Ui::SkipButton());else{Tick(game,2.85f);game.SkipCinematic();}
        game.SkipCinematic();check(game.state.enemy.hp==86&&game.CanAct()&&game.visual.zoom==1&&!game.visual.spear.visible,"ultimate_once_and_cleanup");
    }
    click(Ui::RestartButton());check(game.MenuPage()==1,"restart_class_menu");click(Ui::StartButton());check(game.Map(),"restart_map");click(Ui::MapChoiceRect(0,int(game.run.options.size())));check(game.state.turn==1&&game.state.enemy.hp==opening_hp&&game.state.hand.size()==5,"restart");
    for(int i=0;i<100&&!game.Result();++i){click(Ui::EndButton());Drain(game);}
    check(game.state.phase==Phase::Lost&&game.Result(),"normal_defeat");capture("07-defeat.png");click(Ui::ResultButton());check(game.MenuPage()==1,"result_class_menu");click(Ui::StartButton());check(game.Map()&&game.run.hp==50,"result_restart");click(Ui::MapChoiceRect(0,int(game.run.options.size())));check(game.state.player.hp==50&&game.CanAct(),"result_fight");
    game.state.enemy.hp=1;game.state.hand={"punch"};game.state.energy=1;click(Ui::CardRect(1,0));Drain(game);check(game.Reward()&&!game.Result()&&game.state.phase==Phase::Won,"victory_fixture");capture("08-reward.png");
    const int kept_hp=game.state.player.hp,kept_deck=int(game.run.deck.size());click(Ui::RewardRect(0));check(game.Map()&&!game.Reward()&&game.state.player.hp==kept_hp&&int(game.run.deck.size())==kept_deck+1,"reward_persists_hp");
    game.NewRun();game.ChooseNode(0);game.state.player.hp=40;game.state.hand={"cookie","shield","heavy"};capture("09-all-card-art.png");
    // Full battle through gameplay APIs, with no HP/hand/rage fixtures.
    game.NewRun(1337);int rewards=0;bool persisted=true,phase_shot=false;
    for(int turn=0;turn<2000&&!game.Result();++turn){
        if(game.Busy()){Drain(game);continue;}
        if(game.Reward())persisted&=true;
        const auto step=game.Suggest();
        if(step.kind==AutoKind::Reward){const int hp=game.state.player.hp;game.Act(step);persisted&=game.state.player.hp==hp;++rewards;continue;}
        if(step.kind==AutoKind::None)break;
        if(!phase_shot&&game.run.place==Place::Fight&&game.state.PhaseIndex()>=1){capture("11-boss-phase.png");phase_shot=true;}
        const bool animated=step.kind==AutoKind::Play||step.kind==AutoKind::EndTurn||step.kind==AutoKind::Ultimate;
        game.Act(step);if(animated)Drain(game);ProcessMessage();
    }
    check(game.run.victory&&game.state.PhaseIndex()==2&&game.run.act==2&&persisted,"playthrough_victory");
    report<<"run_victory="<<(game.run.victory?1:0)<<"\n";report<<"boss_phase="<<(game.state.PhaseIndex()+1)<<"\n";report<<"rewards="<<rewards<<"\n";
    report<<"playthrough_hp="<<game.state.player.hp<<" deck="<<game.run.deck.size()<<"\n";capture("10-playthrough-result.png");
    check(ReachSmoke(game,NodeKind::Event),"event_reachable");capture("12-event.png");click(Ui::EventChoiceRect(0));check(!game.Event(),"event_choice_button");
    check(ReachSmoke(game,NodeKind::Shop),"shop_reachable");game.run.gold=400;for(int i=0;i<9;++i)game.run.deck.push_back("peck");capture("13-shop.png");
    click(Ui::ShopRect(0));check(game.run.shop[0].sold,"shop_buy_button");click(Ui::DeckNextButton());click(Ui::DeckNextButton());capture("14-shop-last-page.png");
    const int deck_before=int(game.run.deck.size()),gold_before=game.run.gold;click(Ui::DeckRect(1));check(int(game.run.deck.size())==deck_before-1&&game.run.gold==gold_before-game.state.config.remove_price,"shop_remove_last_page");
    click(Ui::LeaveButton());check(game.Map(),"shop_leave_button");
    check(ReachSmoke(game,NodeKind::Rest),"rest_reachable");game.run.relics={"water_jar"};game.run.hp=20;for(int i=0;i<9;++i)game.run.deck.push_back("peck");capture("15-rest.png");
    click(Ui::DeckNextButton());click(Ui::DeckNextButton());capture("16-rest-last-page.png");const auto upgraded=game.state.Definition(game.run.deck[17]).upgrade_id;click(Ui::UpgradeRect(1));check(game.Map()&&game.run.deck[17]==upgraded,"rest_upgrade_last_page");
    check(ReachSmoke(game,NodeKind::Rest),"rest_heal_reachable");game.run.relics={"water_jar"};game.run.hp=20;const int healing=RestHealAmount(game.run,game.state.config);capture("17-rest-heal.png");click(Ui::HealButton());check(game.Map()&&game.run.hp==20+healing,"rest_heal_matches_preview");
    game.NewRun();game.ChooseNode(0);game.run.potion_slots=4;game.run.potions={"serum","smoke_shield","strength_tonic","weak_brew"};capture("18-potions.png");
    const auto before_potion=game.state.hand.size();click(Ui::PotionRect(3));check(game.state.hand.size()==before_potion+3&&game.run.potions.size()==3,"fourth_potion_button");
    game.NewRun(42);game.SetSaveFile("artifacts/ui-save.json");click(Ui::SaveButton());game.ChooseNode(0);click(Ui::LoadButton());check(game.Map()&&game.run.seed==42,"save_load_buttons");game.SetSaveFile({});capture("19-resumed-map.png");
    game.OpenClasses();capture("20-classes.png");click(Ui::ClassRect(1));check(game.profile.selected_class=="mage","mage_class_button");
    click(Ui::SkillsButton());game.profile.feathers=100;capture("21-skills.png");click(Ui::SkillRect(0,0));check(!game.profile.skills.empty(),"skill_purchase_button");click(Ui::BackButton());
    click(Ui::AchievementsButton());capture("22-achievements.png");check(game.MenuPage()==3,"achievements_button");click(Ui::BackButton());click(Ui::ClassRect(2));check(game.profile.selected_class=="ninja","ninja_class_button");click(Ui::StartButton());check(game.Map()&&game.run.class_id=="ninja"&&game.run.hp==45,"class_start_button");
    game.profile.skills.clear();
    for(const std::string character:{"mage","ninja"}){
        game.NewRun(42,character);game.ChooseNode(0);game.state.enemy.hp=game.state.enemy.max_hp=1000;
        if(character=="ninja"){game.state.hand={"dagger","shield","dagger"};game.state.energy=3;for(int i=0;i<3;++i){game.Play(0);Drain(game);}}
        const int hp=game.state.enemy.hp;game.state.rage=100;game.Ultimate();Tick(game,1.6f);capture(character=="mage"?"23-mage-ultimate.png":"24-ninja-ultimate.png");
        check(game.Cinematic()&&!game.visual.spear.visible,"class_ultimate_has_own_visual");game.SkipCinematic();check(game.CanAct()&&game.state.enemy.hp==hp-(character=="mage"?48:6),"class_ultimate_resolves_once");
    }
    // Renderer fixtures for boss mechanics; logic assertions live in WorldChecks.
    for(const std::string boss:{"frog","snake","hawk","fox","undergod","tiger"}){
        game.NewRun(1337);game.ChooseNode(0);game.run.active_encounter_id=boss;
        game.state.BeginEncounter(FindEnemy(game.state.config,boss),42,50,50,0,game.run.deck,ModsFor(game.run,game.state.config));
        if(boss=="frog"||boss=="fox"||boss=="undergod"){game.state.enemy.hp=game.state.enemy.max_hp/2+1;game.state.hand={"punch","shield"};game.state.energy=3;game.Play(0);Drain(game);}
        capture(("25-boss-"+boss+".png").c_str());
        if(game.state.Targets().size()>1){const int target=game.state.Targets().back();ui.Click(game,int(game.TargetX(target)),420);check(game.state.live.target==target,"target_mouse_button");}
        game.EndTurn();Tick(game,.69f);capture(("26-attack-"+boss+".png").c_str());Drain(game);
    }
    game.OpenClasses();click(Ui::PracticeButton());check(game.Practice()&&game.state.EnemyId()=="tiger","tiger_practice_button");
    game.StartBossPractice("tiger",1337);capture("27-tiger-practice.png");check(game.Practice()&&game.state.enemy.hp==200,"tiger_practice_opens");
    game.state.hand={"punch"};game.state.energy=1;game.state.enemy.hp=101;game.Play(0);Drain(game);capture("28-tiger-enraged.png");check(game.state.PhaseIndex()==1&&game.state.live.charge_left==2,"tiger_phase_charge");
    const int charge_hp=game.state.player.hp;game.EndTurn();Tick(game,.72f);capture("29-tiger-charge.png");check(game.poses[1].name=="ult_charge"&&game.state.player.hp==charge_hp,"tiger_charge_has_pose_and_no_early_hit");Drain(game);
    game.EndTurn();Tick(game,.69f);capture("30-tiger-pounce.png");Drain(game);check(game.state.player.hp<charge_hp,"tiger_pounce_deals_damage");
    report<<"PASS="<<okay<<"\n";return okay?0:3;
}
}
int WINAPI WinMain(HINSTANCE,HINSTANCE,LPSTR args,int){
    using namespace undergod;std::filesystem::current_path(ExeDirectory());const bool smoke=std::string(args?args:"").find("--smoke")!=std::string::npos;
    const bool star_smoke=std::string(args?args:"").find("--star-smoke")!=std::string::npos;
    ChangeWindowMode(TRUE);SetGraphMode(Ui::Width,Ui::Height,32);SetWindowSize(1152,768);SetMainWindowText("Undergod Card Quest - C++ / DxLib");
    SetAlwaysRunFlag(TRUE);SetDoubleStartValidFlag(TRUE);SetOutApplicationLogValidFlag(FALSE);SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8);
    if(DxLib_Init()<0)return 1;SetDrawScreen(DX_SCREEN_BACK);Ui ui;Audio audio;
    try{
        const auto config=LoadConfig();Game game(config,LoadTimeline(),!smoke);
        const std::string arguments=args?args:"";const auto seed_at=arguments.find("--seed ");
        const auto boss_at=arguments.find("--boss ");
        const unsigned seed=seed_at==std::string::npos?unsigned(GetNowHiPerformanceCount()):unsigned(std::stoul(arguments.substr(seed_at+7)));
        if(smoke||star_smoke)game.NewRun();
        else if(boss_at!=std::string::npos){
            std::string boss_id;std::istringstream(arguments.substr(boss_at+7))>>boss_id;
            game.SetSaveFile("save.json");game.SetProfileFile("profile.json");game.StartBossPractice(boss_id,seed,game.profile.selected_class);
        }else{
            game.SetSaveFile("save.json");
            if(!game.SetProfileFile("profile.json"))std::filesystem::copy_file("profile.json","profile.json.invalid."+std::to_string(GetNowHiPerformanceCount()));
            if(seed_at!=std::string::npos||!std::filesystem::exists("save.json")){game.NewRun(seed,game.profile.selected_class);if(seed_at==std::string::npos)game.OpenClasses();}
            else if(!game.Load()){
                std::filesystem::copy_file("save.json","save.json.invalid."+std::to_string(GetNowHiPerformanceCount()));
                game.NewRun(seed,game.profile.selected_class);game.OpenClasses();game.Toast("Bản lưu lỗi đã được giữ riêng. Bắt đầu hành trình mới.");
            }
        }
        if(!ui.Init(config))throw std::runtime_error("Cannot load complete image manifest, DxLib fonts or render target");
        if(!audio.Init())throw std::runtime_error("Cannot load audio manifest or sound files");
        if(smoke){const int result=Smoke(game,ui,audio);audio.Shutdown();ui.Shutdown();DxLib_End();return result;}
        if(star_smoke){const int result=StarSmoke(game,ui,audio);audio.Shutdown();ui.Shutdown();DxLib_End();return result;}
        char previous[256]{};int old_mouse=0;auto last=GetNowHiPerformanceCount();
        while(ProcessMessage()==0){
            const auto start=GetNowHiPerformanceCount();const float dt=std::min(.1f,float(start-last)/1000000.f);last=start;
            char keys[256]{};GetHitKeyStateAll(keys);auto pressed=[&](int key){return keys[key]&&!previous[key];};
            POINT mouse{};RECT client{};GetCursorPos(&mouse);ScreenToClient(GetMainWindowHandle(),&mouse);GetClientRect(GetMainWindowHandle(),&client);
            const int mx=int(static_cast<long long>(mouse.x)*Ui::Width/std::max(1L,client.right)),my=int(static_cast<long long>(mouse.y)*Ui::Height/std::max(1L,client.bottom));
            if(pressed(KEY_INPUT_F1))game.SetSpeed(1);if(pressed(KEY_INPUT_F2))game.SetSpeed(2);if(pressed(KEY_INPUT_F3))game.SetSpeed(3);
            if(pressed(KEY_INPUT_F10))game.ToggleAudio();
            if(pressed(KEY_INPUT_F6))game.Save();if(pressed(KEY_INPUT_F9)&&game.Load())ui.ResetView();
            if(pressed(KEY_INPUT_PGUP))ui.Page(game,game.Map()?1:-1);if(pressed(KEY_INPUT_PGDN))ui.Page(game,game.Map()?-1:1);
            const int wheel=GetMouseWheelRotVol();if(wheel)ui.Page(game,game.Map()?(wheel>0?1:-1):(wheel>0?-1:1));
            const int numbers[]={KEY_INPUT_1,KEY_INPUT_2,KEY_INPUT_3,KEY_INPUT_4,KEY_INPUT_5,KEY_INPUT_6,KEY_INPUT_7,KEY_INPUT_8,KEY_INPUT_9};
            if(game.Cinematic()){if(pressed(KEY_INPUT_SPACE)||pressed(KEY_INPUT_ESCAPE))game.SkipCinematic();}
            else if(game.Menu()){
                if(pressed(KEY_INPUT_ESCAPE))game.BackMenu();
                else if(game.MenuPage()==1){for(int i=0;i<3;++i)if(pressed(numbers[i]))game.SelectClass(i);if(pressed(KEY_INPUT_RETURN)){game.BeginSelected(unsigned(GetNowHiPerformanceCount()));ui.ResetView();}}
            }
            else if((pressed(KEY_INPUT_R)||pressed(KEY_INPUT_RETURN))&&!game.Busy()&&!game.Shop()&&!game.Rest()){game.OpenClasses();ui.ResetView();}
            else if(game.RelicReward()){for(int i=0;i<3;++i)if(pressed(numbers[i]))game.ChooseRelic(i);}
            else if(game.CardSelection()||game.RelicSelection()){for(int i=0;i<8;++i)if(pressed(numbers[i])){if(game.CardSelection())ui.ChooseSelection(game,i);else ui.ChooseTrade(game,i);}if(pressed(KEY_INPUT_ESCAPE))game.CancelSelection();}
            else if(game.Reward()){for(int i=0;i<3;++i)if(pressed(numbers[i]))game.ChooseReward(i);}
            else if(game.Map()){for(int i=0;i<9;++i)if(pressed(numbers[i]))game.ChooseNode(i);}
            else if(game.Event()){for(int i=0;i<std::min(9,int(game.run.event_choices.size()));++i)if(pressed(numbers[i]))game.ChooseEvent(i);}
            else if(game.Shop()){for(int i=0;i<9;++i)if(pressed(numbers[i]))game.Buy(i);if(pressed(KEY_INPUT_ESCAPE)||pressed(KEY_INPUT_E))game.LeaveShop();}
            else if(game.Rest()){if(pressed(KEY_INPUT_H))game.RestHeal();for(int i=0;i<8;++i)if(pressed(numbers[i]))ui.ChooseUpgrade(game,i);}
            else{
                for(int i=0;i<9;++i)if(pressed(numbers[i]))ui.ChooseHand(game,i);
                if(pressed(KEY_INPUT_E))game.EndTurn();if(pressed(KEY_INPUT_U))game.Ultimate();
                if(pressed(KEY_INPUT_Z))game.DrinkPotion(0);if(pressed(KEY_INPUT_X))game.DrinkPotion(1);
                if(pressed(KEY_INPUT_C))game.DrinkPotion(2);if(pressed(KEY_INPUT_V))game.DrinkPotion(3);
                if(pressed(KEY_INPUT_TAB)){const auto targets=game.state.Targets();const auto current=std::find(targets.begin(),targets.end(),game.state.live.target);if(!targets.empty())game.SelectTarget(targets[(current==targets.end()?0:int(current-targets.begin()+1))%targets.size()]);}
            }
            const int buttons=GetMouseInput();if((buttons&MOUSE_INPUT_LEFT)&&!(old_mouse&MOUSE_INPUT_LEFT))ui.Click(game,mx,my);old_mouse=buttons;
            std::copy(keys,keys+256,previous);game.Update(dt);audio.Update(game);ClearDrawScreen();ui.Draw(game,mx,my);ScreenFlip();
            const int wait=16-int((GetNowHiPerformanceCount()-start)/1000);if(wait>0)WaitTimer(wait);
        }
    }catch(const std::exception& error){MessageBoxA(nullptr,error.what(),"Undergod Card Quest",MB_OK|MB_ICONERROR);audio.Shutdown();ui.Shutdown();DxLib_End();return 2;}
    audio.Shutdown();ui.Shutdown();DxLib_End();return 0;
}
