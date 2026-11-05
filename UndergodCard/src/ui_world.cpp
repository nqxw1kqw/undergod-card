#include "ui.hpp"
#include <DxLib.h>
#include <set>
namespace undergod {
namespace {
unsigned Rgb(int r,int g,int b){return GetColor(r,g,b);}
const unsigned Ink=0x343948,Muted=0x7c887c;
bool Has(const std::vector<std::string>& ids,const std::string& id){return std::find(ids.begin(),ids.end(),id)!=ids.end();}
}
bool Ui::DrawExtensions(const Game& game,int mx,int my){
    if(!game.Menu()&&!game.RelicReward()&&!game.CardSelection()&&!game.RelicSelection())return false;
    DrawBox(0,0,Width,Height,Rgb(232,233,227),TRUE);Box({20,20,Width-40,Height-40},Rgb(251,249,243),22);
    Text(55,42,"✦ UNDERGOD",Rgb(75,93,82),brand_);
    Text(950,55,"Lông Vũ: "+std::to_string(game.profile.feathers),Rgb(146,112,48),font_);
    Button(AudioButton(),game.profile.muted?"Tắt ♪":"Bật ♪",true,mx,my);
    Button(BackButton(),game.Menu()?"← Quay lại":(game.RelicReward()?"Chọn lớp / hành trình":"Bỏ qua"),true,mx,my);
    const auto& config=game.state.config;
    if(game.MenuPage()==1){
        Center(720,113,"Chọn người lữ hành",Ink,brand_);
        for(int i=0;i<int(config.classes.size());++i){
            const auto& c=config.classes[i];const auto r=ClassRect(i);const bool selected=c.id==game.profile.selected_class;
            Box({r.x-3,r.y-3,r.w+6,r.h+6},selected?Rgb(121,149,118):Rgb(225,225,213),20);Box(r,Rgb(255,252,246),18);
            Center(r.x+r.w/2,r.y+22,c.name,Ink,heading_);
            const int graph=Image(c.sprite+"_idle_0");if(graph>=0)DrawRotaGraph3F(r.x+r.w/2.f,r.y+265.f,256,456,.43,.43,0,graph,TRUE);
            Center(r.x+r.w/2,r.y+295,"HP "+std::to_string(c.hp)+" · Năng lượng "+std::to_string(c.energy)+" · Rút "+std::to_string(c.draw),Muted,small_);
            Wrapped({r.x+22,r.y+332,r.w-44,76},c.description,Muted,font_);
        }
        Button(SkillsButton(),"Cây kỹ năng",true,mx,my);Button(AchievementsButton(),"Thành tựu",true,mx,my);
        Button(ChallengeMinus(),"−",game.SelectedChallenge()>0,mx,my);Button(ChallengePlus(),"+",game.SelectedChallenge()<std::clamp(game.profile.highest_challenge+1,0,10),mx,my);
        Center(1030,668,"Thử Thách "+std::to_string(game.SelectedChallenge()),Ink,font_);
        std::string description="Độ khó chuẩn. Thắng chiến dịch để mở Thử Thách tiếp theo.";
        if(game.SelectedChallenge()>0)description="Bất lợi cộng dồn: "+config.challenges[game.SelectedChallenge()-1].description;
        Wrapped({790,726,510,65},description,Muted);
        Button(StartButton(),"Bắt đầu hành trình →",true,mx,my);Text(120,772,"1–3 chọn lớp. Enter bắt đầu. Esc trở về.",Muted,small_);
    }else if(game.MenuPage()==2){
        Center(720,110,"Cây kỹ năng · "+game.SelectedClass().name,Ink,brand_);
        std::vector<std::string> branches;for(const auto& s:config.skills)if(s.class_id==game.profile.selected_class&&std::find(branches.begin(),branches.end(),s.branch)==branches.end())branches.push_back(s.branch);
        for(int branch=0;branch<int(branches.size());++branch)for(const auto& s:config.skills)if(s.class_id==game.profile.selected_class&&s.branch==branches[branch]){
            const bool owned=Has(game.profile.skills,s.id);const int level=SkillLevel(game.profile,config,s.class_id,s.branch);const bool can=!owned&&level==s.tier-1&&game.profile.feathers>=s.cost;const auto r=SkillRect(branch,s.tier-1);
            Button(r,s.name+(owned?" · Đã mở":" · "+std::to_string(s.cost)+" Lông Vũ"),owned||can,mx,my);
            if(r.Contains(mx,my))Wrapped({100,665,1230,100},s.name+": "+s.description,Ink,font_);
        }
        Text(100,785,"Mỗi nhánh mở tuần tự. Kỹ năng áp dụng từ hành trình mới; không thay đổi run đang chơi.",Muted,font_);
    }else if(game.MenuPage()==3){
        Center(720,110,"Dấu chân đã để lại",Ink,brand_);
        for(int i=0;i<int(config.achievements.size());++i){const auto& a=config.achievements[i];const Rect r={90+(i%2)*640,175+(i/2)*76,610,66};const bool unlocked=Has(game.profile.achievements,a.id);Box(r,unlocked?Rgb(221,233,210):Rgb(234,233,224));Text(r.x+18,r.y+10,(unlocked?"✓ ":"○ ")+a.name,unlocked?Rgb(71,104,76):Ink,font_);Wrapped({r.x+18,r.y+35,r.w-35,24},a.description,Muted);
            Text(r.x+470,r.y+12,"+"+std::to_string(a.feathers)+" Lông Vũ",Muted,small_);
            if(r.Contains(mx,my)){const auto it=game.profile.metrics.find(a.metric);const int progress=it==game.profile.metrics.end()?0:it->second;Wrapped({100,790,1210,42},a.description+" · Tiến độ "+std::to_string(std::min(a.threshold,progress))+"/"+std::to_string(a.threshold),Ink,font_);}
        }
    }else if(game.RelicReward()){
        Center(720,160,"Chiến lợi phẩm của trùm",Ink,brand_);Center(720,217,"Đã hồi 50% HP tối đa. Chọn một di vật để đi tiếp.",Muted,font_);
        for(int i=0;i<int(game.run.relic_rewards.size());++i){const auto& relic=FindRelic(config,game.run.relic_rewards[i]);auto r=RewardRect(i);r.h=290;Box(r,Rgb(241,235,216),18);const int graph=Image("relic_"+relic.id);if(graph>=0)DrawRotaGraph3F(r.x+r.w/2.f,r.y+85.f,128,128,.48,.48,0,graph,TRUE);Center(r.x+r.w/2,r.y+144,std::to_string(i+1)+" · "+relic.name,Ink,small_);Wrapped({r.x+20,r.y+182,r.w-40,90},relic.description,Muted,font_);}
        // Boss rewards cannot be skipped; the back button only opens the class menu.
    }else{
        const bool relics=game.RelicSelection();const int count=relics?int(game.run.relics.size()):int(game.run.deck.size());
        Center(720,130,relics?"Chọn di vật để đổi":(game.run.selection=="upgrade"?"Chọn một lá để nâng cấp":"Chọn một lá để gỡ"),Ink,brand_);
        for(int i=0;i<8&&deck_page_*8+i<count;++i){const int index=deck_page_*8+i;std::string label;bool enabled=true;
            if(relics)label=FindRelic(config,game.run.relics[index]).name;
            else{const auto& card=FindCard(config,game.run.deck[index]);label=card.name;enabled=game.run.selection=="remove"?count>5:!card.upgrade_id.empty();if(UpgradeRect(i).Contains(mx,my)&&!card.upgrade_id.empty())Wrapped({120,570,1120,100},FindCard(config,card.upgrade_id).description,Ink,font_);}
            Button(UpgradeRect(i),std::to_string(i+1)+" · "+label,enabled,mx,my);
        }
        Button(DeckPrevButton(),"← Trang trước",deck_page_>0,mx,my);Button(DeckNextButton(),"Trang sau →",(deck_page_+1)*8<count,mx,my);Text(470,863,"Trang "+std::to_string(deck_page_+1),Muted,font_);
    }
    if(game.toast_age>0){Box({420,900,850,34},Rgb(55,72,62));Wrapped({434,907,820,24},game.toast,Rgb(255,255,246));}
    return true;
}

void Ui::DrawTargets(const Game& game,int mx,int my)const{
    const auto& state=game.state;const auto targets=state.Targets();
    if(targets.size()>1)for(int index:targets){
        const auto& fighter=state.Target(index);const int x=int(game.TargetX(index));const Rect r={x-70,318,140,58};
        Box({r.x-2,r.y-2,r.w+4,r.h+4},state.live.target==index?Rgb(170,115,83):Rgb(217,222,205));Box(r,Rgb(251,248,235));
        const bool disguised=state.EnemyId()=="fox"&&targets.size()>1;
        Center(x,r.y+5,state.TargetName(index),Ink,small_);Center(x,r.y+25,disguised?"? / ? HP":std::to_string(fighter.hp)+"/"+std::to_string(fighter.max_hp)+" HP",Muted,small_);
        if(disguised)Center(x,r.y+42,"Chọn để thử phá ảo",Muted,small_);
        else if(index>0){const auto& intent=state.adds[index-1].intent;Center(x,r.y+42,intent.attack?"Đánh "+std::to_string(intent.value*intent.hits):"Phòng / Tăng sức",Muted,small_);}
    }
    if(state.live.airborne)Center(1160,294,"Đang bay · giảm 50% sát thương cận chiến",Rgb(105,113,158),small_);
    if(state.live.doom_left>0)Center(720,226,"TẬN THẾ SAU "+std::to_string(state.live.doom_left)+" LƯỢT",Rgb(181,77,91),font_);
    else if(state.live.constrict_left>0)Center(720,226,"Siết: còn "+std::to_string(state.live.constrict_left)+" lượt",Rgb(156,105,77),small_);
    const auto future=state.PreviewIntentText();if(!future.empty())Wrapped({760,568,620,40},future,Muted);
}
}
