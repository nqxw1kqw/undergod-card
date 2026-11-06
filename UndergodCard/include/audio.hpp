#pragma once
#include "game.hpp"
#include <map>
namespace undergod {
class Audio {
    std::map<std::string,int> handles_;
    int music_=-1;
    bool muted_=false;
public:
    int loaded=0,expected=0,played=0;
    bool Init();
    void Shutdown();
    void Update(Game& game);
    bool Play(const std::string& id);
    bool Playing(const std::string& id)const;
};
}
