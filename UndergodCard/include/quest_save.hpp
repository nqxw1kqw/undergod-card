#pragma once
#include "quest_run.hpp"
#include <filesystem>

namespace undergod {
bool SaveSession(const std::filesystem::path& file,const RunState& run,const Combat& combat,int speed,std::string& error);
bool LoadSession(const std::filesystem::path& file,RunState& run,Combat& combat,int& speed,std::string& error);
}
