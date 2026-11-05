#pragma once
// Small portability layer for desktop builds (Windows, Linux). Nothing here is
// used by the Switch build, which keeps its sdmc:/ paths and libnx services.
#include <string>
#include <fstream>
#include <cstdlib>
#include <cstddef>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#include <malloc.h>
#else
#include <unistd.h>
#endif
namespace xs {
// Creates a directory; an already existing one counts as success.
inline bool make_dir(const char* path){
#ifdef _WIN32
    const int rc=_mkdir(path);
#else
    const int rc=mkdir(path,0777);
#endif
    if(rc==0)return true;
    struct stat st{};return stat(path,&st)==0&&(st.st_mode&S_IFDIR)!=0;
}
inline bool change_directory(const std::string& path){
#ifdef _WIN32
    return _chdir(path.c_str())==0;
#else
    return chdir(path.c_str())==0;
#endif
}
inline bool file_exists(const std::string& path){std::ifstream f(path,std::ios::binary);return f.good();}
// std::aligned_alloc does not exist in the Windows C runtime, and memory from
// _aligned_malloc must be released with _aligned_free, never free().
inline void* aligned_alloc64(size_t bytes){
#ifdef _WIN32
    return _aligned_malloc(bytes,64);
#else
    return std::aligned_alloc(64,(bytes+63)/64*64);
#endif
}
inline void aligned_free64(void* pointer){
#ifdef _WIN32
    _aligned_free(pointer);
#else
    std::free(pointer);
#endif
}
// Font used by the UI. A font.ttf next to the program always wins, as on the
// Switch; otherwise common system fonts are tried so a fresh install still runs.
inline std::string find_font(){
    for(const char* candidate:{
        "font.ttf",
#ifdef _WIN32
        "C:/Windows/Fonts/segoeui.ttf","C:/Windows/Fonts/arial.ttf","C:/Windows/Fonts/tahoma.ttf",
#else
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf","/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/Library/Fonts/Arial.ttf","/System/Library/Fonts/Supplemental/Arial.ttf",
#endif
    })if(file_exists(candidate))return candidate;
    return "font.ttf";
}
}
