#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <DxLib.h>
#include "ui.hpp"
#include <filesystem>
#include <fstream>
#include <cmath>
#include <chrono>

namespace {
std::filesystem::path ExeDirectory(){wchar_t path[32768]{};GetModuleFileNameW(nullptr,path,32768);return std::filesystem::path(path).parent_path();}
void Tick(undergod::Game& game,float logical_seconds){game.Update(logical_seconds/game.PlaybackRate());}
void Drain(undergod::Game& game){for(int i=0;i<100&&game.Busy();++i)game.Update(.1f);}
int Smoke(undergod::Game& game,undergod::Ui& ui){
    using namespace undergod;std::filesystem::create_directories("artifacts");std::ofstream report("artifacts/smoke.txt");bool okay=true;
    auto check=[&](bool result,const char* label){report<<label<<"="<<result<<"\n";okay&=result;};
    auto capture=[&](const char* name){ClearDrawScreen();ui.Draw(game,-1,-1);const auto file=std::string("artifacts/")+name;check(SaveDrawScreenToPNG(0,0,Ui::Width,Ui::Height,file.c_str())==0,name);ScreenFlip();ProcessMessage();};
    auto click=[&](Rect r){ui.Click(game,r.x+r.w/2,r.y+r.h/2);};
    report<<"assets="<<ui.loaded<<"/"<<Ui::ExpectedAssets<<"\n";check(ui.loaded==Ui::ExpectedAssets,"all_assets");
    game.NewRun();capture("01-battle.png");check(game.state.hand.size()==5&&game.state.player.hp==50&&game.state.enemy.hp==120,"opening");
    check(game.Speed()==1,"default_speed_x1");click(Ui::SpeedButton(2));check(game.Speed()==2,"speed_x2_button");click(Ui::SpeedButton(3));check(game.Speed()==3,"speed_x3_button");click(Ui::SpeedButton(1));
    game.state.energy=0;click(Ui::CardRect(5,0));check(!game.toast.empty()&&game.state.enemy.hp==120,"unaffordable_toast");game.state.energy=3;
    click(Ui::CardRect(5,0));check(game.Busy(),"attack_locks_input");Tick(game,.24f);capture("02-lunge.png");check(game.state.enemy.hp==120,"no_damage_before_contact");
    Tick(game,.11f);capture("03-hit.png");Drain(game);check(game.state.enemy.hp==114&&game.state.energy==2,"damage_at_contact");
    const int incoming=game.state.intent.value,player_hp=game.state.player.hp;
    click(Ui::EndButton());const auto hp=game.state.enemy.hp;game.Play(0);check(game.Busy()&&game.state.enemy.hp==hp,"enemy_input_lock");
    Tick(game,.69f);capture("04-boss-lunge.png");check(game.state.player.hp==player_hp&&game.ActorOffset(true)<-600,"boss_windup_and_contact_range");
    Tick(game,.09f);capture("04-boss-impact.png");check(game.state.player.hp==player_hp-incoming,"boss_deals_damage_at_contact");
    Drain(game);check(game.state.turn==2&&std::abs(game.ActorOffset(true))<.01f,"next_turn_and_boss_returns");capture("04-boss-recovery.png");
    for(int mode=0;mode<3;++mode){
        click(Ui::SpeedButton(mode+1));game.state.rage=100;game.state.enemy.hp=120;game.state.enemy.block=6;click(Ui::UltimateButton());check(game.Cinematic(),"ultimate_started");
        if(mode==0){
            Tick(game,.7f);capture("05-spear-windup.png");click(Ui::SpeedButton(2));check(game.Speed()==2&&game.Cinematic(),"speed_button_during_cinematic");
            Tick(game,.25f);capture("05-spear-rise.png");check(game.visual.spear.visible&&!game.visual.spear.falling,"spear_launches_into_sky");
            click(Ui::SpeedButton(3));Tick(game,1.6f);capture("05-spear-fall.png");check(game.state.enemy.hp==120&&game.visual.spear.falling,"fall_waits_for_contact");
            Tick(game,.06f);capture("06-impact.png");Drain(game);
        }
        else if(mode==1)click(Ui::SkipButton());else{Tick(game,2.85f);game.SkipCinematic();}
        game.SkipCinematic();check(game.state.enemy.hp==86&&game.CanAct()&&game.visual.zoom==1&&!game.visual.spear.visible,"ultimate_once_and_cleanup");
    }
    click(Ui::RestartButton());check(game.state.turn==1&&game.state.enemy.hp==120&&game.state.hand.size()==5,"restart");
    for(int i=0;i<100&&!game.Result();++i){click(Ui::EndButton());Drain(game);}
    check(game.state.phase==Phase::Lost,"normal_defeat");capture("07-defeat.png");click(Ui::ResultButton());check(game.state.player.hp==50&&game.CanAct(),"result_restart");
    game.state.enemy.hp=1;game.state.hand={"punch"};click(Ui::CardRect(1,0));Drain(game);check(game.state.phase==Phase::Won&&game.Result(),"victory_fixture");capture("08-victory-fixture.png");
    game.NewRun();game.state.player.hp=40;game.state.hand={"cookie","shield","heavy"};capture("09-all-card-art.png");
    // Full battle through gameplay APIs, with no HP/hand/rage fixtures.
    game.NewRun(1337);
    for(int turn=0;turn<80&&!game.Result();++turn){
        for(int play=0;play<12&&game.CanAct();++play){
            if(game.state.rage>=game.state.config.balance.rage_max){game.Ultimate();Drain(game);continue;}
            int best=-1;double score=0;
            for(int i=0;i<int(game.state.hand.size());++i){
                if(!game.state.CanPlay(i))continue;const auto& card=game.state.Definition(game.state.hand[i]);
                double value=card.damage;
                if(card.damage>=game.state.enemy.hp+game.state.enemy.block)value+=100;
                if(card.block>0){const int incoming=game.state.intent.attack?std::max(0,game.state.intent.value-game.state.player.block):0;value+=std::min(card.block,incoming)*1.3+1;if(incoming>=game.state.player.hp&&card.block>0)value+=100;}
                if(card.heal>0)value+=std::min(card.heal,game.state.player.max_hp-game.state.player.hp)*(game.state.player.hp<25?2.:.8);
                value/=std::max(1,card.cost);if(value>score){score=value;best=i;}
            }
            if(best<0)break;game.Play(best);Drain(game);
        }
        if(game.CanAct()){game.EndTurn();Drain(game);}ProcessMessage();
    }
    check(game.state.phase==Phase::Won,"playthrough_victory");report<<"playthrough_hp="<<game.state.player.hp<<" turns="<<game.state.turn<<"\n";capture("10-playthrough-result.png");
    report<<"PASS="<<okay<<"\n";return okay?0:3;
}
}
int WINAPI WinMain(HINSTANCE,HINSTANCE,LPSTR args,int){
    using namespace undergod;std::filesystem::current_path(ExeDirectory());const bool smoke=std::string(args?args:"").find("--smoke")!=std::string::npos;
    ChangeWindowMode(TRUE);SetGraphMode(Ui::Width,Ui::Height,32);SetWindowSize(1152,768);SetMainWindowText("Undergod Card Quest - C++ / DxLib");
    SetAlwaysRunFlag(TRUE);SetDoubleStartValidFlag(TRUE);SetOutApplicationLogValidFlag(FALSE);SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8);
    if(DxLib_Init()<0)return 1;SetDrawScreen(DX_SCREEN_BACK);Ui ui;
    try{
        const auto config=LoadConfig();Game game(config,LoadTimeline());game.NewRun();
        if(!ui.Init(config))throw std::runtime_error("Cannot create DxLib fonts or render target");
        if(smoke){const int result=Smoke(game,ui);ui.Shutdown();DxLib_End();return result;}
        game.SetSessionSeed(unsigned(std::chrono::high_resolution_clock::now().time_since_epoch().count()));game.NewMatch();
        char previous[256]{};int old_mouse=0;auto last=GetNowHiPerformanceCount();
        while(ProcessMessage()==0){
            const auto start=GetNowHiPerformanceCount();const float dt=std::min(.1f,float(start-last)/1000000.f);last=start;
            char keys[256]{};GetHitKeyStateAll(keys);auto pressed=[&](int key){return keys[key]&&!previous[key];};
            POINT mouse{};RECT client{};GetCursorPos(&mouse);ScreenToClient(GetMainWindowHandle(),&mouse);GetClientRect(GetMainWindowHandle(),&client);
            const int mx=int(static_cast<long long>(mouse.x)*Ui::Width/std::max(1L,client.right)),my=int(static_cast<long long>(mouse.y)*Ui::Height/std::max(1L,client.bottom));
            if(pressed(KEY_INPUT_F1))game.SetSpeed(1);if(pressed(KEY_INPUT_F2))game.SetSpeed(2);if(pressed(KEY_INPUT_F3))game.SetSpeed(3);
            if(game.Cinematic()){if(pressed(KEY_INPUT_SPACE)||pressed(KEY_INPUT_ESCAPE))game.SkipCinematic();}
            else{
                const int numbers[]={KEY_INPUT_1,KEY_INPUT_2,KEY_INPUT_3,KEY_INPUT_4,KEY_INPUT_5};for(int i=0;i<5;++i)if(pressed(numbers[i]))game.Play(i);
                if(pressed(KEY_INPUT_E))game.EndTurn();if(pressed(KEY_INPUT_U))game.Ultimate();
                if(pressed(KEY_INPUT_R)&&!game.Busy())game.Retry();
                if(pressed(KEY_INPUT_RETURN)&&!game.Busy())game.NewMatch();
            }
            const int buttons=GetMouseInput();if((buttons&MOUSE_INPUT_LEFT)&&!(old_mouse&MOUSE_INPUT_LEFT))ui.Click(game,mx,my);old_mouse=buttons;
            std::copy(keys,keys+256,previous);game.Update(dt);ClearDrawScreen();ui.Draw(game,mx,my);ScreenFlip();
            const int wait=16-int((GetNowHiPerformanceCount()-start)/1000);if(wait>0)WaitTimer(wait);
        }
    }catch(const std::exception& error){MessageBoxA(nullptr,error.what(),"Undergod Card Quest",MB_OK|MB_ICONERROR);ui.Shutdown();DxLib_End();return 2;}
    ui.Shutdown();DxLib_End();return 0;
}
