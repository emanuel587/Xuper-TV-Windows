#include "player.hpp"
#include "view.hpp"
#include "trace.hpp"
#include "ranger_codec.hpp"
#include <chrono>
#include <cassert>
#include <iostream>
#include <vector>
#include <set>
int main(int argc,char** argv){
    if(argc<2||argc>3)return 2;
    std::vector<std::pair<std::string,std::string>> headers;
    if(argc==3)headers={{"Content-Auth","synthetic-auth"},{"Content-License","synthetic-license"}};
    const std::string mode=argc==3?argv[2]:"";
    std::string playback_source=argv[1];
    if(mode=="denied"){
        const std::string url=argv[1];const auto start=url.find('/',url.find("://")+3);const auto query=url.find('?',start);
        playback_source=url.substr(0,start)+xs::ranger_slb_live_path("source-live","ts")+(query==std::string::npos?"":url.substr(query));
    }
    if(mode=="live"||mode=="live_mp4"||mode=="route404"){
        const std::string url=argv[1];const auto start=url.find('/',url.find("://")+3);const std::string resource=url.substr(start);
        const auto moment=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        if(mode!="route404"){
            const auto suffix=url.find('?',start);
            playback_source=url.substr(0,start)+xs::ranger_slb_live_path("source-live",mode=="live_mp4"?"mp4":"ts")+(suffix==std::string::npos?"":url.substr(suffix));
        }
        headers={{"Content-Auth",xs::ranger_live_content_auth(resource,0,moment)},{"Content-License","synthetic-license"},
            {"App","test.app"},{"App-Version","43404"},{"Ranger-Id",xs::ranger_request_id(123456,0,static_cast<uint32_t>(moment/1000),0x12345678)},{"Pragma","akamai-x-cache-on"}};
    }
    xs::trace_reset();xs::View view;std::string error;
    assert(view.open(error));
    auto* window=view.window;auto* renderer=view.renderer;
    xs::Catalog catalog;catalog.entries.push_back({"Test",argv[1]});
    for(int i=0;i<2;++i){
        view.draw(catalog,0,"Before playback",true);
        xs::PlaybackOptions options;options.font=view.font;options.title="Prueba de video HTTP";
        error=xs::play(playback_source,renderer,headers,options);
        if(mode=="denied"||mode=="route404"){assert(error.find(mode=="route404"?"HTTP 404":"HTTP 403")!=std::string::npos);view.close();std::cout<<"HTTP error classified without leaking request data\n";return 0;}
        if(!error.empty())std::cerr<<error<<'\n';
        assert(error.empty());
        assert(view.window==window&&view.renderer==renderer);
        assert(SDL_WasInit(SDL_INIT_VIDEO));
        int width=0,height=0;assert(SDL_GetRendererOutputSize(renderer,&width,&height)==0);
        std::vector<unsigned> pixels(static_cast<size_t>(width)*height);
        assert(SDL_RenderReadPixels(renderer,nullptr,SDL_PIXELFORMAT_ABGR8888,pixels.data(),width*4)==0);
        std::set<unsigned> colors;
        for(size_t p=0;p<pixels.size();p+=127)colors.insert(pixels[p]&0xffffffu);
        assert(colors.size()>16); // The decoded test pattern must not be a blank frame.
        view.draw(catalog,0,"Returned to menu",true);
    }
    view.close();std::cout<<"Two complete playbacks returned to persistent menu\n";
}
