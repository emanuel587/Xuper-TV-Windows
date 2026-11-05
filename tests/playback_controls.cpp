#include "player.hpp"
#include "view.hpp"
#include "trace.hpp"
#include <thread>
#include <chrono>
#include <cassert>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <vector>
static void key(SDL_Keycode key){SDL_Event event{};event.type=SDL_KEYDOWN;event.key.keysym.sym=key;assert(SDL_PushEvent(&event)==1);std::this_thread::sleep_for(std::chrono::milliseconds(150));}
static void wait(int n){std::this_thread::sleep_for(std::chrono::milliseconds(n));}
int main(int argc,char** argv){
    if(argc!=2)return 2;
    xs::trace_reset();xs::View view;std::string error;assert(view.open(error));
    xs::PlaybackOptions options;options.font=view.font;options.title="Prueba: pelicula con dos idiomas";
    options.variants={"720p / Espanol","1080p / Ingles"};options.items={"Episodio 1","Episodio 2"};options.start=2;
    options.subtitles={"Espanol externo"};options.subtitle_loader=[](size_t,std::string& data,std::string&){data="1\n00:00:00,000 --> 00:00:25,000\nSubtitulo externo verificado\n";return true;};
    auto* renderer=view.renderer;
    for(int scenario=0;scenario<8;++scenario){
        xs::PlaybackOutcome result;
        std::thread input([scenario]{wait(600);
            if(scenario==0){key(SDLK_SPACE);wait(200);key(SDLK_SPACE);wait(200);key(SDLK_y);wait(150);key(SDLK_s);wait(150);key(SDLK_RIGHTBRACKET);wait(150);key(SDLK_f);wait(150);key(SDLK_RIGHT);wait(200);key(SDLK_LEFT);wait(200);key(SDLK_x);wait(300);key(SDLK_ESCAPE);wait(100);key(SDLK_ESCAPE);}
            if(scenario==1||scenario==2){key(SDLK_x);wait(100);for(int i=0;i<(scenario==1?3:4);++i){key(SDLK_DOWN);wait(50);}key(SDLK_RETURN);wait(100);key(SDLK_DOWN);wait(50);key(SDLK_RETURN);}
            if(scenario==3){key(SDLK_x);wait(100);key(SDLK_DOWN);wait(50);key(SDLK_DOWN);wait(50);key(SDLK_RETURN);wait(100);for(int i=0;i<2;++i){key(SDLK_DOWN);wait(50);}key(SDLK_RETURN);wait(300);key(SDLK_ESCAPE);wait(100);key(SDLK_ESCAPE);}
            if(scenario==4){key(SDLK_SPACE);wait(200);key(SDLK_ESCAPE);}
            if(scenario==5){key(SDLK_x);wait(300);SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);}
            if(scenario==6){key(SDLK_x);wait(100);for(int i=0;i<17;++i){key(SDLK_DOWN);wait(25);}key(SDLK_RETURN);wait(100);key(SDLK_ESCAPE);wait(50);key(SDLK_ESCAPE);}
            if(scenario==7){key(SDLK_x);wait(100);for(int i=0;i<7;++i){key(SDLK_DOWN);wait(25);}key(SDLK_LEFT);wait(50);key(SDLK_DOWN);wait(25);key(SDLK_DOWN);wait(25);key(SDLK_RIGHT);wait(25);key(SDLK_DOWN);wait(25);key(SDLK_RIGHT);wait(25);key(SDLK_DOWN);wait(25);key(SDLK_RIGHT);wait(100);key(SDLK_ESCAPE);wait(50);key(SDLK_ESCAPE);}
        });
        error=xs::play(argv[1],renderer,{},options,&result);input.join();
        if(!error.empty())std::cerr<<error<<'\n';
        assert(error.empty());assert(view.renderer==renderer);
        if(scenario==0){assert(std::abs(result.aspect-16.0/9)<0.001);assert(!result.paused);assert(result.audio==2);assert(result.subtitle==1);assert(std::abs(result.speed-1.25)<0.001);assert(result.position>=1);}
        if(scenario==1)assert(result.next_variant==1&&result.position>=2);
        if(scenario==2)assert(result.next_item==1);
        if(scenario==3)assert(result.subtitle==2);
        if(scenario==4)assert(result.paused);
        if(scenario==5){SDL_Surface* image=SDL_CreateRGBSurfaceWithFormat(0,1280,720,32,SDL_PIXELFORMAT_ABGR8888);assert(image);
            assert(SDL_RenderReadPixels(renderer,nullptr,SDL_PIXELFORMAT_ABGR8888,image->pixels,image->pitch)==0);const char* capture=std::getenv("XUPERSWITCH_CAPTURE");assert(SDL_SaveBMP(image,capture?capture:"player_menu.bmp")==0);SDL_FreeSurface(image);}
        if(scenario==6)assert(!result.auto_next);
        if(scenario==7){assert(result.volume==95);assert(std::abs(result.audio_delay-0.1)<0.001);assert(std::abs(result.sub_delay-0.25)<0.001);assert(std::abs(result.sub_scale-1.1)<0.001);}
        std::cout<<"controls scenario "<<scenario<<" passed; aid="<<result.audio<<" sid="<<result.subtitle<<" speed="<<result.speed<<" position="<<result.position<<std::endl;
    }
}
