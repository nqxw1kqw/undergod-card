#include "game.hpp"
#include <cmath>

namespace undergod {
void Game::NewRun(unsigned seed,std::string class_id,int challenge){if(practice_)profile=practice_profile_;practice_=false;cinematic_.Cancel();StartRun(run,state,seed,class_id,challenge,profile.skills);run.run_id=std::to_string(seed)+":"+std::to_string(profile.next_run++);profile.selected_class=class_id;profile_dirty_=true;menu_=0;motion_=Motion::None;pending_card_=-1;motion_age_=cinematic_age_=0;motion_impact_=false;speed_=1;poses={};popups.clear();visual={};time=toast_age=0;toast.clear();sound_events.clear();save_dirty_=true;arrival_=Arrival::None;arrival_age_=0;star_sequence_=false;active_card_={};active_actor_=-1;travel_from_=travel_to_=pending_option_=-1;ResetSkillVisuals();}
void Game::StartBossPractice(std::string enemy_id,unsigned seed,std::string class_id){
    const auto& boss=FindEnemy(state.config,enemy_id);if(boss.role!="boss")throw std::runtime_error("Practice needs a boss");
    const auto kept_profile=profile;const auto baseline=practice_?practice_profile_:profile;NewRun(seed,class_id);profile=kept_profile;practice_profile_=baseline;profile_dirty_=false;
    practice_=true;save_dirty_=false;run.act=1;run.map=GenerateMap(state.config,seed,run.act);run.node=run.index=int(run.map.size())-1;
    run.map.back().encounter_id=enemy_id;run.map.back().visited=true;run.options.clear();run.place=Place::Fight;run.active_encounter_id=enemy_id;
    for(auto& id:run.deck){const auto upgrade=FindCard(state.config,id).upgrade_id;if(!upgrade.empty())id=upgrade;}
    run.potions={"serum","smoke_shield"};
    state.BeginEncounter(boss,seed,run.hp,run.max_hp,0,run.deck,ModsFor(run,state.config));Toast("Đấu thử · Bộ bài nâng cấp và hai thuốc · F9 trở về bản lưu.");
    if(turn_stars_){state.EnableTurnStars();ConsumeEvents();BeginArrival();}
}
bool Game::SetProfileFile(std::filesystem::path file){profile_file_=std::move(file);if(!std::filesystem::exists(profile_file_))return true;std::string error;const bool loaded=LoadProfile(profile_file_,profile,state.config,error);if(!loaded)Toast("Không đọc được hồ sơ tiến trình.");return loaded;}
void Game::SelectClass(int index){if(index<0||index>=int(state.config.classes.size()))return;profile.selected_class=state.config.classes[index].id;profile_dirty_=true;}
bool Game::BuySkill(const std::string& id){
    if(practice_){Toast("Đấu thử không mua kỹ năng hoặc thay đổi tiến trình.");return false;}
    const auto old=profile;if(!UnlockSkill(profile,state.config,id)){Toast("Chưa đủ Lông Vũ hoặc chưa mở nút trước.");return false;}
    std::string error;if(!profile_file_.empty()&&!SaveProfile(profile_file_,profile,error)){profile=old;Toast("Không ghi được hồ sơ; Lông Vũ được giữ nguyên.");return false;}
    profile_dirty_=true;sound_events.push_back("reward");Toast("Đã mở kỹ năng. Áp dụng từ hành trình mới.");return true;
}
bool Game::Save(bool notify){
    if(practice_){if(notify)Toast("Đấu thử không ghi đè hành trình hoặc tiến trình.");return false;}
    if(Busy()||Menu()){if(notify)Toast("Chờ hành động kết thúc và đóng menu để lưu nhé.");return false;}
    if(save_file_.empty())return false;
    std::string error;if(!profile_file_.empty()&&!SaveProfile(profile_file_,profile,error)){Toast("Không ghi được hồ sơ tiến trình.");return false;}profile_dirty_=false;
    const bool saved=SaveSession(save_file_,run,state,speed_,error);save_dirty_=false;
    if(!saved)Toast("Không lưu được hành trình. Kiểm tra thư mục lưu.");else if(notify)Toast("Đã lưu hành trình.");
    return saved;
}
bool Game::Load(){
    if(Busy()){Toast("Chờ hành động kết thúc để tải nhé.");return false;}
    std::string error;
    if(save_file_.empty()||!LoadSession(save_file_,run,state,speed_,error)){Toast("Không đọc được bản lưu. Hành trình hiện tại vẫn được giữ.");return false;}
    cinematic_.Cancel();motion_=Motion::None;pending_card_=-1;motion_age_=cinematic_age_=0;motion_impact_=false;
    poses={};popups.clear();visual={};time=0;menu_=0;arrival_=Arrival::None;star_sequence_=false;active_card_={};active_actor_=-1;if(turn_stars_&&run.place==Place::Fight)state.EnableTurnStars();if(practice_){profile=practice_profile_;profile_dirty_=false;}practice_=false;save_dirty_=false;sound_events.clear();ResetSkillVisuals();Toast("Đã tiếp tục hành trình.");return true;
}
void Game::ConsumeEvents(){
    for(const auto& event:state.events){
        ReactSkillEvent(event);
        if(event.type==EventType::Damage&&event.amount>0)sound_events.push_back("hit");
        else if(event.type==EventType::Block)sound_events.push_back("block");
        else if(event.type==EventType::Heal||event.type==EventType::Revive)sound_events.push_back("heal");
        else if(event.type==EventType::Phase||event.type==EventType::Summon)sound_events.push_back("boss");
        if(event.type==EventType::Damage){const bool blocked=event.amount==0&&event.blocked>0;poses[event.enemy?1:0]={blocked?"defend":"hit",0};popups.push_back({event.enemy,event.amount?event.amount:event.blocked,EventType::Damage,0,blocked,StatusId::Poison,event.target});}
        else if(event.type==EventType::Block){poses[event.enemy?1:0]={"defend",0};popups.push_back({event.enemy,event.amount,EventType::Block,0,false,StatusId::Poison,event.target});}
        else if(event.type==EventType::Heal)popups.push_back({event.enemy,event.amount,EventType::Heal,0,false,StatusId::Poison,event.target});
        else if(event.type==EventType::Status)popups.push_back({event.enemy,event.amount,EventType::Status,0,false,event.status,event.target});
        else if(event.type==EventType::Phase)Toast("Giai đoạn "+std::to_string(event.amount)+": "+state.PhaseName());
        else if(event.type==EventType::Revive){popups.push_back({false,event.amount,EventType::Heal,0});Toast("Phượng Hoàng hồi sinh!");}
        else if(event.type==EventType::Summon)Toast("Địch gọi thêm "+std::to_string(event.amount)+" viện binh.");
    }
    state.events.clear();
    SyncSkillShields();
    if(sound_events.size()>64)sound_events.erase(sound_events.begin(),sound_events.end()-64);
}
void Game::AfterResolve(){
    ConsumeEvents();
    if(turn_stars_&&((star_sequence_&&motion_!=Motion::None)||Cinematic()))return;
    if(practice_){run.hp=state.player.hp;run.rage=state.rage;run.victory=state.phase==Phase::Won;run.defeat=state.phase==Phase::Lost;if(run.victory||run.defeat)sound_events.push_back(run.victory?"victory":"defeat");return;}
    ObserveCombat(run,state);
    if(turn_stars_&&run.place==Place::Fight&&!state.stars.enabled){
        cinematic_.Cancel(false);state.EnableTurnStars();star_sequence_=false;motion_=Motion::None;motion_age_=0;pending_card_=-1;active_actor_=-1;poses={};visual={};BeginArrival();ConsumeEvents();
    }
    if(SettleProfile(profile,run,state.config)){profile_dirty_=true;sound_events.push_back(run.victory?"victory":"defeat");}save_dirty_=true;
}
void Game::Play(int index){
    if(!CanAct()||index<0||index>=int(state.hand.size()))return;
    if(turn_stars_){
        if(!state.QueueStar(index)){Toast(state.IsQueuedStar(index)?"Lá này đã nằm trong hàng chờ.":(state.stars.player.size()>=3?"Đã đủ ba sao. Bấm Attack hoặc gỡ một lá.":"Lá này bị khóa hoặc không thể chơi."));return;}
        save_dirty_=true;sound_events.push_back("select");return;
    }
    if(!state.CanPlay(index)){Toast(index==state.live.locked?"Lưỡi Quấn: lá bài này bị khóa trong lượt.":(state.Definition(state.hand[index]).unplayable?"Lời nguyền này không chơi được.":"Không đủ năng lượng cho lá bài này."));return;}
    pending_card_=index;const bool attack=state.Definition(state.hand[index]).damage>0;
    motion_=attack?Motion::PlayerAttack:Motion::PlayerOther;motion_age_=0;motion_impact_=false;
    if(attack)poses[0]={"attack",0};else ApplyMotion();
}
void Game::EndTurn(){
    if(turn_stars_){if(!CanAct())return;if(!state.BeginStarAttack()){Toast("Chọn đủ ba lá trước khi Attack. Có thể dùng Chờ nếu thiếu bài.");return;}star_sequence_=true;BeginStarMotion();return;}
    if(!CanAct()||!state.EndPlayer())return;motion_=state.intent.attack?Motion::EnemyAttack:Motion::EnemyOther;motion_age_=0;motion_impact_=false;ConsumeEvents();
}
void Game::ApplyMotion(){
    if(motion_impact_)return;motion_impact_=true;
    if(skill_visual_.active)skill_visual_.applied=true;
    if(star_sequence_)state.ResolveStarAction();
    else if(motion_==Motion::PlayerAttack||motion_==Motion::PlayerOther)state.Play(pending_card_);
    else state.ActEnemy();
    AfterResolve();
}
void Game::ChooseNode(int option){
    if(!Map())return;
    if(turn_stars_){
        if(option<0||option>=int(run.options.size()))return;
        pending_option_=option;travel_from_=run.node;travel_to_=run.options[option];arrival_=Arrival::Travel;arrival_age_=0;return;
    }
    if(!undergod::ChooseNode(run,state,option))return;
    save_dirty_=true;
    motion_=Motion::None;pending_card_=-1;motion_age_=0;motion_impact_=false;poses={};popups.clear();visual={};
    if(run.place==Place::Fight)Toast(state.PhaseName());
    else if(run.place==Place::Event)Toast(run.event_title);
}
void Game::ChooseReward(int index){
    if(!Reward())return;
    const int choice=index;
    if(choice<0||choice>=int(run.rewards.size()))return;
    const auto id=run.rewards[choice];
    if(!undergod::ChooseReward(run,state,choice))return;
    if(turn_stars_&&run.place==Place::Fight){state.EnableTurnStars();BeginArrival();}
    save_dirty_=true;
    motion_=Motion::None;pending_card_=-1;motion_age_=0;motion_impact_=false;poses={};popups.clear();visual={};
    Toast("Nhận: "+state.Definition(id).name);
    sound_events.push_back("reward");
}
bool Game::ChooseRelic(int index){const bool changed=RelicReward()&&ChooseRelicReward(run,state.config,index);save_dirty_|=changed;if(changed)sound_events.push_back("reward");return changed;}
bool Game::ChooseCard(int index){const bool changed=CardSelection()&&ChooseSelectedCard(run,state.config,index);save_dirty_|=changed;return changed;}
bool Game::TradeRelic(int index){const bool changed=RelicSelection()&&ChooseTradeRelic(run,state.config,index);save_dirty_|=changed;return changed;}
void Game::ChooseEvent(int index){
    if(!Event()||index<0||index>=int(run.event_choices.size()))return;
    const auto outcome=run.event_choices[index].outcome;
    if(undergod::ChooseEvent(run,state,index)){save_dirty_=true;Toast(run.last_outcome.empty()?outcome:run.last_outcome);if(turn_stars_&&run.place==Place::Fight){state.EnableTurnStars();BeginArrival();}}
    else Toast("Không đủ vàng cho lựa chọn này.");
}
bool Game::Buy(int index){const bool changed=Shop()&&undergod::BuyItem(run,state.config,index);save_dirty_|=changed;return changed;}
bool Game::RemoveCard(int index){const bool changed=Shop()&&undergod::RemoveCard(run,state.config,index);save_dirty_|=changed;return changed;}
void Game::LeaveShop(){if(Shop()){undergod::LeaveShop(run);save_dirty_=true;}}
bool Game::RestHeal(){const bool changed=Rest()&&undergod::RestHeal(run,state.config);save_dirty_|=changed;return changed;}
bool Game::UpgradeCard(int index){const bool changed=Rest()&&undergod::UpgradeCard(run,state.config,index);save_dirty_|=changed;return changed;}
bool Game::DrinkPotion(int index){if(!CanAct())return false;const bool drank=undergod::DrinkPotion(run,state,index);if(drank){AfterResolve();save_dirty_=true;}return drank;}
bool Game::Act(const AutoStep& step){
    switch(step.kind){
    case AutoKind::Node: ChooseNode(step.index); return true;
    case AutoKind::Reward: ChooseReward(step.index); return true;
    case AutoKind::Event: ChooseEvent(step.index); return true;
    case AutoKind::Buy: return Buy(step.index);
    case AutoKind::Remove: return RemoveCard(step.index);
    case AutoKind::Leave: LeaveShop(); return true;
    case AutoKind::Heal: return RestHeal();
    case AutoKind::Upgrade: return UpgradeCard(step.index);
    case AutoKind::Potion: return DrinkPotion(step.index);
    case AutoKind::Play: Play(step.index); return true;
    case AutoKind::EndTurn: EndTurn(); return true;
    case AutoKind::Ultimate: Ultimate(); return true;
    case AutoKind::Relic:return ChooseRelic(step.index);
    case AutoKind::SelectCard:if(step.index<0){CancelSelection();return true;}return ChooseCard(step.index);
    case AutoKind::TradeRelic:return TradeRelic(step.index);
    case AutoKind::Target:return SelectTarget(step.index);
    case AutoKind::Wait:return QueueWait();
    default: return false;
    }
}
void Game::CueStep(const Step& step){
    const float late=std::max(0.f,cinematic_age_-step.t/1000.f);
    switch(step.cue){
    case Cue::Dim:visual.dim=step.value;break;
    case Cue::Zoom:visual.zoom_target=step.value;visual.zoom_duration=step.duration/1000.f;visual.zoom_age=0;break;
    case Cue::Pose:poses[step.enemy?1:0]={step.pose,late};break;
    case Cue::Vfx:visual.vfx_age=late;visual.vfx_frames=int(step.value);visual.vfx_enemy=step.enemy;break;
    case Cue::Spear:if(state.Mods().ultimate=="spear")visual.spear={true,step.falling,late,step.duration/1000.f,step.from_x,step.from_y,step.to_x,step.to_y};break;
    case Cue::Shake:visual.shake=step.value;visual.shake_left=std::max(0.f,step.duration/1000.f-late);break;
    case Cue::Flash:visual.flash_duration=step.duration/1000.f;visual.flash_left=std::max(0.f,visual.flash_duration-late);break;
    default:break;
    }
}
void Game::Ultimate(){
    if(!CanAct())return;if(!state.StartUltimate()){Toast("Tuyệt kỹ cần "+std::to_string(state.UltimateCost())+" Nộ.");return;}
    sound_events.push_back("ultimate");
    ConsumeEvents();visual={};cinematic_age_=0;
    cinematic_.Start(timeline_,[this](const Step& s){CueStep(s);},[this]{state.UltimateImpact();AfterResolve();},[this]{state.FinishUltimate();visual={};poses[0]={};if(turn_stars_)AfterResolve();});
}
void Game::SkipCinematic(){cinematic_.Skip();}
void Game::Update(float real_dt){
    real_dt=std::max(0.f,real_dt);toast_age=std::max(0.f,toast_age-real_dt);
    const float dt=real_dt*PlaybackRate();time+=dt;
    TickSkillVisuals(dt);
    UpdateArrival(dt);
    for(auto& p:poses){p.age+=dt;const bool held=p.name=="ult_throw"&&Cinematic();if(!held&&p.name!="idle"&&p.age>(p.name=="ult_charge"?2.f:.65f))p={};}
    for(auto& p:popups)p.age+=dt;
    popups.erase(std::remove_if(popups.begin(),popups.end(),[](const auto& p){return p.age>.95f;}),popups.end());
    if(motion_!=Motion::None){
        const bool enemy=motion_==Motion::EnemyAttack||motion_==Motion::EnemyOther;
        const float before=motion_age_;motion_age_+=dt;
        if(skill_visual_.active)skill_visual_.age=motion_age_;
        if(enemy&&before<.45f&&motion_age_>=.45f)poses[1]={motion_==Motion::EnemyAttack?"attack":((star_sequence_?active_charge_:state.intent.kind==IntentKind::Charge)?"ult_charge":"defend"),motion_age_-.45f};
        const float impact=skill_visual_.active?skill_visual_.animation.contact:(enemy?(motion_==Motion::EnemyAttack?.77f:.45f):.32f);
        if(motion_age_>=impact)ApplyMotion();
        const float duration=skill_visual_.active?skill_visual_.animation.duration:(enemy?1.15f:.65f);
        if(motion_age_>=duration){motion_=Motion::None;pending_card_=-1;skill_visual_.active=false;if(star_sequence_)FinishStarMotion();}
    }
    SyncSkillShields();
    if(!Busy()&&!practice_){ObserveCombat(run,state);if(SettleProfile(profile,run,state.config)){profile_dirty_=true;save_dirty_=true;}}
    if(Cinematic()){
        cinematic_age_+=dt;
        visual.zoom_age+=dt;
        const float z=std::min(1.f,visual.zoom_duration>0?visual.zoom_age/visual.zoom_duration:1.f);
        visual.zoom=1+(visual.zoom_target-1)*(1-(1-z)*(1-z));
        if(visual.vfx_age>=0)visual.vfx_age+=dt;
        if(visual.spear.visible)visual.spear.age+=dt;
        visual.shake_left=std::max(0.f,visual.shake_left-dt);visual.flash_left=std::max(0.f,visual.flash_left-dt);
        cinematic_.Advance(cinematic_age_*1000.f);
    }else cinematic_age_=0;
    if(save_dirty_&&!save_file_.empty()&&!Busy()&&!Menu()&&!practice_)Save(false);
    if(profile_dirty_&&!profile_file_.empty()&&!practice_){std::string error;if(SaveProfile(profile_file_,profile,error))profile_dirty_=false;}
}
std::pair<float,float> Game::SpearTip()const{
    const auto& s=visual.spear;const float t=std::clamp(s.age/std::max(.001f,s.duration),0.f,1.f);
    const float progress=s.falling?t*t:1-(1-t)*(1-t);
    return {s.from_x+(s.to_x-s.from_x)*progress,s.from_y+(s.to_y-s.from_y)*progress};
}
float Game::TargetX(int target)const{
    const int count=int(state.adds.size())+1;if(count==1)return EnemyHome;
    int slot=target;for(int i=0;i<int(state.live.slots.size());++i)if(state.live.slots[i]==target){slot=i;break;}
    return 840.f+500.f*slot/std::max(1,count-1);
}
float Game::ActorOffset(bool enemy,int target)const{
    if(turn_stars_){
        const auto& r=Reaction(enemy?target:-1);const float recoil=std::sin(r.hit_age*60)*12*std::max(0.f,1-r.hit_age/.3f)*(enemy?1.f:-1.f);
        if(!SkillActor(enemy,target))return recoil;
        const auto& v=skill_visual_;const auto& a=v.animation;
        const bool melee=a.motion==SkillMotion::Thrust||a.motion==SkillMotion::Slash||a.motion==SkillMotion::Peck||a.motion==SkillMotion::Dash||a.motion==SkillMotion::Pounce||a.motion==SkillMotion::Swoop;
        if(!melee||a.self)return recoil;
        const float ramp=std::max(.06f,a.contact-a.launch-.10f),recover=std::max(.06f,a.duration-a.contact-.18f);float f=0;
        if(v.age>=a.launch&&v.age<a.launch+ramp){const float t=(v.age-a.launch)/ramp;f=t*t;}
        else if(v.age>=a.launch+ramp&&v.age<a.contact+.07f)f=1;
        else if(v.age>=a.contact+.07f){const float t=std::clamp((v.age-a.contact-.07f)/recover,0.f,1.f);f=1-t*t*(3-2*t);}
        const float travel=std::max(0.f,TargetX(enemy?target:state.live.target)-PlayerHome-(enemy?170.f:160.f));
        return (enemy?-1.f:1.f)*travel*a.lunge*f+recoil;
    }
    const auto& p=poses[enemy?1:0];float recoil=p.name=="hit"?std::sin(p.age*60.f)*12.f*std::max(0.f,1-p.age/.3f)*(enemy?1.f:-1.f):0;
    if((enemy&&motion_!=Motion::EnemyAttack)||(!enemy&&motion_!=Motion::PlayerAttack)||(enemy&&star_sequence_&&target!=active_actor_))return recoil;
    const float t=motion_age_-(enemy?.45f:0.f);float f=0;
    if(t>0&&t<.22f)f=(t/.22f)*(t/.22f);else if(t>=.22f&&t<.32f)f=1;else if(t>=.32f&&t<.54f)f=1-(t-.32f)/.22f;
    const float contact_travel=TargetX(enemy?target:state.live.target)-PlayerHome-(enemy?170.f:160.f);
    return (enemy?-1.f:1.f)*contact_travel*f+recoil;
}
}
