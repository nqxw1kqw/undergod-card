#include "quest_storage.hpp"
#include <fstream>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif
namespace undergod {
bool WriteAtomicText(const std::filesystem::path& file,const std::string& text,std::string& error){
    auto temporary=file;temporary+=".tmp";
    try{
        if(file.empty())throw std::runtime_error("Empty storage path");
        if(file.has_parent_path())std::filesystem::create_directories(file.parent_path());
        std::ofstream out(temporary,std::ios::binary|std::ios::trunc);if(!out)throw std::runtime_error("Cannot open storage temp");out<<text;out.flush();if(!out)throw std::runtime_error("Cannot flush storage");out.close();if(!out)throw std::runtime_error("Cannot close storage");
#ifdef _WIN32
        if(!MoveFileExW(temporary.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot replace storage");
#else
        std::filesystem::rename(temporary,file);
#endif
        error.clear();return true;
    }catch(const std::exception& e){error=e.what();std::error_code ignored;std::filesystem::remove(temporary,ignored);return false;}
}
}
