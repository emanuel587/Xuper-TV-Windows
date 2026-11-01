#include "playback_state.hpp"
#include <cassert>
#include <filesystem>
#include <chrono>
#include <iostream>
int main(){
    const auto directory=std::filesystem::temp_directory_path()/("xuperswitch-state-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directory(directory);
    const auto path=(directory/"playback.json").string();
    xs::PlaybackHistory state(path);assert(state.position("video")==0&&state.speed==1);
    assert(state.save("video",120.5,1.25,70,false,false));
    xs::PlaybackHistory restored(path);assert(restored.position("video")==120.5&&restored.speed==1.25&&restored.volume==70&&!restored.auto_next);
    assert(restored.save("video",0,1,100,true,false));assert(xs::PlaybackHistory(path).position("video")==0);
    assert(restored.save("video",180,1,100,true,false));assert(restored.save("video",180,1,100,true,true));assert(xs::PlaybackHistory(path).position("video")==0);
    assert(restored.save("negative",-5,100,-1,true,false));assert(restored.position("negative")==0&&restored.speed==2&&restored.volume==0);
    std::filesystem::rename(path,path+".bak");
    assert(xs::PlaybackHistory(path).speed==2);
    {std::ofstream file(path);file<<"{partial";}
    assert(xs::PlaybackHistory(path).speed==2);
    auto recovered=xs::PlaybackHistory(path);assert(recovered.save("recovered",90,1,80,true,false));
    assert(xs::PlaybackHistory(path).position("recovered")==90&&!std::filesystem::exists(path+".bak"));
    std::filesystem::create_directory(path+".tmp");
    assert(!recovered.save("lost",200,1,80,true,false));
    assert(std::string(recovered.last_error)=="write"&&xs::PlaybackHistory(path).position("lost")==0);
    assert(xs::PlaybackHistory(path).position("recovered")==90);
    std::filesystem::remove(path+".tmp");
    {std::ofstream file(path);file<<"{malformed";}
    xs::PlaybackHistory damaged(path);assert(damaged.positions.empty()&&damaged.speed==1);
    {std::ofstream file(path);file<<R"({"positions":{"bad":"text","future":999999999},"speed":-10,"volume":200})";}
    xs::PlaybackHistory bounds(path);assert(bounds.position("bad")==0&&bounds.position("future")==0&&bounds.speed==0.5&&bounds.volume==100);
    for(int i=0;i<510;++i)bounds.positions[std::to_string(i)]=20;
    assert(bounds.save("last",50,1,100,true,false)&&bounds.positions.size()<=500);
    std::filesystem::remove_all(directory);std::cout<<"playback state: resume, completion, preferences, malformed file and bounds passed\n";
}
