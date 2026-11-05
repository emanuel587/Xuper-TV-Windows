#include "platform.hpp"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
int main(){
    const char* dir="xs_platform_test_dir";
    std::remove(dir);
#ifdef _WIN32
    _rmdir(dir);
#else
    rmdir(dir);
#endif
    assert(xs::make_dir(dir));                 // creates
    assert(xs::make_dir(dir));                 // already existing counts as success
    assert(!xs::make_dir("xs_platform_missing/child"));   // no recursive creation
    const std::string file=std::string(dir)+"/probe.txt";
    {std::ofstream f(file);f<<"x";}
    assert(xs::file_exists(file));
    assert(!xs::file_exists(std::string(dir)+"/nothing.txt"));
    assert(!xs::make_dir(file.c_str()));       // a file is not a directory
    std::remove(file.c_str());
#ifdef _WIN32
    _rmdir(dir);
#else
    rmdir(dir);
#endif
    for(size_t bytes:{size_t(1),size_t(63),size_t(64),size_t(640*4*360)}){
        void* p=xs::aligned_alloc64(bytes);
        assert(p&&reinterpret_cast<std::uintptr_t>(p)%64==0);
        static_cast<unsigned char*>(p)[bytes-1]=1;  // whole block is writable
        xs::aligned_free64(p);
    }
    assert(!xs::find_font().empty());
    assert(xs::change_directory("."));
    std::puts("platform: directories, aligned buffers, font lookup and chdir passed");
    return 0;
}
