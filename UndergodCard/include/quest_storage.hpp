#pragma once
#include <filesystem>
#include <string>
namespace undergod { bool WriteAtomicText(const std::filesystem::path& file,const std::string& text,std::string& error); }
