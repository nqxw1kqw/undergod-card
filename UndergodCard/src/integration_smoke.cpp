#include "ui.hpp"
#include "audio.hpp"
#include <DxLib.h>
#include <filesystem>
#include <fstream>
#include <set>
#include <chrono>
#include <numeric>

namespace undergod {
int IntegrationSmoke(Game& game,Ui& ui,Audio& audio){
    const std::filesystem::path folder="artifacts/integration";std::filesystem::create_directories(folder);
    std::ofstream report(folder/"smoke.txt");bool okay=true;game.profile.muted=true;
    auto check=[&](bool v,const std::string& name){report<<name<<"="<<v<<"\n";report.flush();okay&=v;};
    auto step=[&](float seconds){while(seconds>.00001f){const float dt=std::min(.01f,seconds);game.Update(dt/game.PlaybackRate());audio.Update(game);seconds-=dt;}};
    auto settle=[&](){for(int i=0;i<3000&&game.Busy();++i)step(.01f);check(!game.Busy(),"settled");};
    auto click=[&](Rect r){ui.Click(game,r.x+r.w/2,r.y+r.h/2);};
    auto capture=[&](const std::string& name,int mx=-1,int my=-1){ClearDrawScreen();ui.Draw(game,mx,my);check(SaveDrawScreenToPNG(0,0,Ui::Width,Ui::Height,(folder/name).string().c_str())==0,name);ScreenFlip();ProcessMessage();};
    auto fixture=[&](const std::vector<std::string>& hand){game.StartBossPractice("tiger",1337);settle();game.state.hand=hand;game.state.live.locked=-1;game.state.enemy.hp=game.state.enemy.max_hp=500;game.state.player.hp=game.state.player.max_hp=500;Intent wait;wait.kind=IntentKind::Defend;game.state.stars.enemy={{0,wait,true},{0,wait,true},{0,wait,true}};step(.3f);};
    auto visual_hand=[&](int index){for(const auto& c:game.cards)if(!c.leaving&&c.zone==CardZone::Hand&&c.hand_index==index){const auto b=c.Bounds();return Rect{int(b.x),int(b.y),int(b.w),int(b.h)};}return Rect{0,0,1,1};};
    check(ui.loaded==379&&Ui::Width==1600&&Ui::Height==900,"draft_assets_and_resolution_preserved");
    check(ui.grayscale>=9,"dxlib_monochrome_filter_prepared_for_heroes_and_bosses");
    game.NewRun(1337);game.OpenClasses();capture("00-integrated-menu.png");game.BackMenu();game.ChooseNode(0);step(3.25f);capture("01-staggered-deal.png");settle();
    fixture({"punch","punch","shield","cookie"});std::set<unsigned> ids;for(const auto& c:game.cards)if(!c.leaving)ids.insert(c.id);
    click(visual_hand(0));step(.10f);capture("02-card-queue-flight.png");step(.15f);click(Ui::StarSlotRect(0));step(.1f);capture("03-card-return-reflow.png");step(.2f);
    std::set<unsigned> after;for(const auto& c:game.cards)if(!c.leaving)after.insert(c.id);check(ids==after,"duplicate_card_identity_survives_queue_and_return");
    game.state.enemy.block=4;click(visual_hand(0));step(.25f);game.QueueWait();game.QueueWait();click(Ui::EndButton());step(.15f);capture("04-active-card-flight.png");step(.19f);capture("05-partial-block-impact.png");
    bool blocked=false,hp=false;for(const auto& p:game.popups){blocked|=p.enemy&&p.blocked&&p.amount==4;hp|=p.enemy&&!p.blocked&&p.type==EventType::Damage&&p.amount==2;}
    check(blocked&&hp,"separate_absorbed_and_hp_popups");settle();
    fixture({"heavy+","cookie+","shield"});game.state.player.hp=498;game.FocusCard(1);capture("06-resolver-details.png");game.FocusCard(1);capture("07-capped-heal-details.png");check(game.Preview(1).heal==2,"details_use_effective_heal");
    game.state.draw.clear();for(const auto& c:game.state.config.cards)if(!c.upgraded)game.state.draw.push_back(c.id);const auto rng=game.state.Snapshot().shuffle_rng;
    click(Ui::DrawPileButton());capture("08-unordered-pile.png");click(Ui::PileNextButton());capture("09-pile-second-page.png");check(game.pile_page==1&&game.state.Snapshot().shuffle_rng==rng,"pile_pages_do_not_reveal_or_consume_shuffle_rng");click(Ui::PileCloseButton());
    game.state.rage=100;game.Ultimate();step(1.4f);capture("10-full-feedback-camera.png");const auto feedback=Ui::FeedbackButton();click(feedback);capture("11-reduced-feedback-camera.png");check(game.ReducedFeedback(),"feedback_button_applies_during_cinematic");game.SkipCinematic();settle();
    fixture({"punch"});click(Ui::FeedbackButton());game.state.enemy.hp=1;click(visual_hand(0));game.QueueWait();game.QueueWait();click(Ui::EndButton());step(.34f);capture("12-fatal-contact.png");step(.35f);capture("13-defeated-actor.png");step(.85f);capture("14-result-fade-recap.png");check(game.terminal.active&&game.ResultAlpha()>0&&!game.run.victory,"result_fades_before_reward_settlement");settle();capture("15-practice-result.png");check(game.summary.cards==1&&game.summary.damage==1,"recap_uses_actual_hp_loss");
    const auto result_seed=game.Seed();click(Ui::RetrySeedButton());settle();check(game.Seed()==result_seed&&!game.Result(),"result_same_seed_button_restarts_practice");
    game.StartBossPractice("tiger",1337);settle();click(Ui::DifficultyButton());settle();capture("16-challenge-practice.png");check(game.PracticeLevel()==PracticeDifficulty::Challenge,"practice_difficulty_control");click(Ui::DifficultyButton());settle();capture("17-gentle-practice.png");check(game.PracticeLevel()==PracticeDifficulty::Gentle,"gentle_profile_keeps_draft_boss");
    game.SetSessionSeed(77);game.NewMatch();settle();const auto seed=game.Seed();game.Retry();settle();check(game.Seed()==seed,"same_seed_retry");game.NewMatch();settle();check(game.Seed()!=seed,"new_seed_session");
    fixture({"shield","punch","cookie"});game.state.player.hp=480;
    Intent hit;hit.kind=IntentKind::Attack;hit.attack=true;hit.value=8;game.state.stars.enemy[0]={0,hit,false};hit.value=6;game.state.stars.enemy[1]={0,hit,false};
    game.Play(0);game.Play(1);game.Play(2);step(.25f);click(Ui::ForecastButton());capture("18-six-step-forecast.png");
    check(game.forecast_view&&game.Forecast().steps[1].hp==477&&game.Forecast().steps[4].hp==479,"six_step_panel_tracks_armor_damage_and_heal");
    click(Ui::ForecastCloseButton());click(Ui::EndButton());step(.34f);capture("19-first-action-recovery.png");check(game.ActiveStep()==0&&game.state.stars.cursor==1,"active_indicator_stays_with_recovery");
    click(Ui::ForecastButton());capture("19b-live-six-actions.png");check(game.forecast_view&&game.Forecast().steps[0].resolved,"live_panel_distinguishes_actual_contact_from_projection");click(Ui::ForecastCloseButton());settle();
    fixture({"heavy"});game.Play(0);game.QueueWait();game.QueueWait();game.EndTurn();step(.30f);capture("20-heavy-windup.png");check(game.state.enemy.hp==500,"heavy_does_not_hit_during_windup");step(.15f);capture("21-heavy-contact.png");check(game.state.enemy.hp==488,"heavy_hits_at_its_own_contact");settle();
    // CPU submission timings only; ScreenFlip/GPU waiting is measured separately.
    std::ofstream perf(folder/"performance.json");perf<<"{\"frames_per_case\":120,\"unit\":\"microseconds\",\"cases\":[";
    for(int scenario=0;scenario<3;++scenario){
        fixture({"shield","punch","cookie"});if(scenario==1)game.FocusCard(1);if(scenario==2)game.ToggleForecast();
        ClearDrawScreen();ui.Draw(game,-1,-1);ScreenFlip();const auto builds=game.ForecastStats().round_builds;
        std::vector<double> update,draw,present;
        for(int n=0;n<120;++n){
            const auto a=std::chrono::steady_clock::now();game.Update(.016f/game.PlaybackRate());audio.Update(game);const auto b=std::chrono::steady_clock::now();
            ClearDrawScreen();ui.Draw(game,-1,-1);const auto c=std::chrono::steady_clock::now();ScreenFlip();ProcessMessage();const auto d=std::chrono::steady_clock::now();
            auto us=[](auto x,auto y){return std::chrono::duration<double,std::micro>(y-x).count();};update.push_back(us(a,b));draw.push_back(us(b,c));present.push_back(us(c,d));
        }
        check(game.ForecastStats().round_builds==builds,"native_idle_frames_reuse_forecast_"+std::to_string(scenario));
        auto mean=[](const auto& v){return std::accumulate(v.begin(),v.end(),0.)/v.size();};std::sort(draw.begin(),draw.end());
        if(scenario)perf<<",";perf<<"{\"scenario\":"<<scenario<<",\"update_mean_us\":"<<mean(update)<<",\"draw_mean_us\":"<<mean(draw)<<",\"draw_p95_us\":"<<draw[114]<<",\"present_mean_us\":"<<mean(present)<<"}";
    }
    perf<<"],\"scope\":\"current native CPU update/draw submission; present includes GPU/display wait; not a before-after FPS claim\"}\n";
    report<<"PASS="<<okay<<"\n";return okay?0:5;
}
}
