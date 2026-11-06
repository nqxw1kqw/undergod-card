#include "audio.hpp"
#include <DxLib.h>
#include "../third_party/nlohmann/json.hpp"
#include <fstream>
namespace undergod {
bool Audio::Init(){
    std::ifstream file("data/audio.json");if(!file)return false;nlohmann::json defs;file>>defs;expected=int(defs.size());
    for(const auto& def:defs){const std::string id=def.at("id"),path=def.at("file");const int handle=LoadSoundMem(path.c_str());handles_[id]=handle;if(handle>=0){ChangeVolumeSoundMem(def.value("volume",150),handle);++loaded;}}
    return loaded==expected;
}
void Audio::Shutdown(){for(const auto& p:handles_)if(p.second>=0)DeleteSoundMem(p.second);handles_.clear();}
bool Audio::Play(const std::string& id){const auto it=handles_.find(id);if(muted_||it==handles_.end()||it->second<0)return false;const bool success=PlaySoundMem(it->second,DX_PLAYTYPE_BACK,TRUE)==0;if(success)++played;return success;}
bool Audio::Playing(const std::string& id)const{const auto it=handles_.find(id);return it!=handles_.end()&&it->second>=0&&CheckSoundMem(it->second)==1;}
void Audio::Update(Game& game){
    const bool changed=muted_!=game.profile.muted;muted_=game.profile.muted;
    if(changed&&muted_)for(const auto& p:handles_)if(p.second>=0)StopSoundMem(p.second);
    const auto it=handles_.find("music_"+std::to_string(game.run.act));const int wanted=it==handles_.end()?-1:it->second;
    if(muted_){if(music_>=0)StopSoundMem(music_);music_=-1;}
    else if(wanted>=0&&(wanted!=music_||changed)){if(music_>=0)StopSoundMem(music_);music_=wanted;PlaySoundMem(music_,DX_PLAYTYPE_LOOP,TRUE);}
    for(const auto& id:game.sound_events)Play(id);game.sound_events.clear();
}
}
