#include "view.hpp"
#include "trace.hpp"
#include "donation_qr.hpp"
#include "platform.hpp"
#include <algorithm>
#include <cmath>

namespace xs {
static constexpr SDL_Color white{247,245,255,255},muted{164,158,185,255};
static constexpr SDL_Color accent{164,91,255,255},accent2{111,64,210,255};
static constexpr SDL_Color panel{27,24,42,255},panel2{37,31,56,255};
static constexpr SDL_Color line{54,45,75,255},gold{255,207,113,255};
static const char* nav[]={"Inicio","TV en vivo","Peliculas","Series","Favoritos","Apoyar"};

static const char* label(EntryKind kind,const std::string& type){
    switch(kind){
        case EntryKind::Channel:return "EN VIVO";
        case EntryKind::Asset:return type=="0"?"SERIE":"PELICULA";
        case EntryKind::Episode:return "REPRODUCIR";
        case EntryKind::VodPage:case EntryKind::SearchPage:return "SIGUIENTE PAGINA";
        case EntryKind::Category:case EntryKind::VodCategory:return "CATEGORIA";
        default:return "EXPLORAR";
    }
}
static void rounded(SDL_Renderer* renderer,SDL_Rect r,SDL_Color color,int radius=12){
    radius=std::max(0,std::min(radius,std::min(r.w,r.h)/2));
    SDL_SetRenderDrawColor(renderer,color.r,color.g,color.b,color.a);
    for(int y=0;y<r.h;++y){
        int inset=0;
        if(y<radius){const double dy=radius-y-0.5;inset=radius-static_cast<int>(std::sqrt(std::max(0.0,double(radius*radius)-dy*dy)));}
        else if(y>=r.h-radius){const double dy=y-(r.h-radius)+0.5;inset=radius-static_cast<int>(std::sqrt(std::max(0.0,double(radius*radius)-dy*dy)));}
        SDL_RenderDrawLine(renderer,r.x+inset,y+r.y,r.x+r.w-inset-1,y+r.y);
    }
}

bool View::open(std::string& error){
    trace("view:init");
#ifndef __SWITCH__
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"0");
    const Uint32 sdl_flags=SDL_INIT_VIDEO|SDL_INIT_EVENTS|SDL_INIT_GAMECONTROLLER;
#else
    const Uint32 sdl_flags=SDL_INIT_VIDEO|SDL_INIT_EVENTS;
#endif
    if(SDL_Init(sdl_flags)!=0){error=SDL_GetError();return false;}
    if(TTF_Init()!=0){error=TTF_GetError();return false;}
    window=SDL_CreateWindow("Xuper TV para Windows",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1280,720,SDL_WINDOW_RESIZABLE);
    if(!window){error=SDL_GetError();return false;}
    renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if(!renderer)renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
    if(!renderer){error=SDL_GetError();return false;}
    SDL_RenderSetLogicalSize(renderer,1280,720);SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"1");
#ifdef __SWITCH__
    const char* path="sdmc:/switch/xuperswitch/font.ttf";
#else
    const std::string font_path=find_font();const char* path=font_path.c_str();
#endif
    font=TTF_OpenFont(path,22);small=TTF_OpenFont(path,16);large=TTF_OpenFont(path,46);heading=TTF_OpenFont(path,30);
    if(!font||!small||!large||!heading){error=TTF_GetError();return false;}
    images=std::make_unique<Artwork>(renderer);trace("view:ready");return true;
}
void View::rect(SDL_Rect r,SDL_Color c){SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,c.a);SDL_RenderFillRect(renderer,&r);}
void View::text(int x,int y,const std::string& value,SDL_Color color,int width,TTF_Font* type,int max_height){
    if(value.empty()||width<=0)return;
    auto* s=TTF_RenderUTF8_Blended_Wrapped(type?type:font,value.c_str(),color,width);if(!s)return;
    auto* t=SDL_CreateTextureFromSurface(renderer,s);SDL_Rect dst{x,y,s->w,s->h};
    if(t){if(max_height>0){SDL_Rect clip{x,y,width,max_height};SDL_RenderSetClipRect(renderer,&clip);}SDL_RenderCopy(renderer,t,nullptr,&dst);SDL_RenderSetClipRect(renderer,nullptr);SDL_DestroyTexture(t);}SDL_FreeSurface(s);
}
void View::background(){
    for(int y=0;y<720;++y){
        const auto r=static_cast<Uint8>(8+y*5/720),g=static_cast<Uint8>(7+y*4/720),b=static_cast<Uint8>(17+y*13/720);
        SDL_SetRenderDrawColor(renderer,r,g,b,255);SDL_RenderDrawLine(renderer,0,y,1280,y);
    }
    rounded(renderer,{1000,-150,390,360},{25,14,48,100},180);
}
void View::brand(int x,int y,bool big){
    const int n=big?56:38;
    rounded(renderer,{x,y,n,n},accent,big?15:12);
    text(x+(big?10:8),y+(big?-2:0),"X",{255,255,255,255},n,big?large:heading);
    text(x+n+12,y-2,"Xuper TV",white,330,big?large:heading);
    text(x+n+14,y+(big?54:30),"ENTRETENIMIENTO",{187,145,255,255},260,small);
}
void View::splash(const std::string& status){
    background();brand(405,218,true);
    text(405,330,"Todo tu entretenimiento, en un solo lugar.",muted,650,font);
    rounded(renderer,{405,398,470,5},{48,38,66,255},3);
    const int p=static_cast<int>((SDL_GetTicks()/5)%350);rounded(renderer,{405+p,398,120,5},accent,3);
    text(405,425,status,white,650,font,80);
    text(405,560,"Preparando tu experiencia...",muted,680,small);
    SDL_RenderPresent(renderer);
}
int View::columns(const Catalog& c)const{
    if(!c.entries.empty()&&c.entries[0].kind==EntryKind::Asset)return 5;
    if(!c.entries.empty()&&c.entries[0].kind==EntryKind::Channel)return 4;
    return 3;
}
int View::page_size(const Catalog& c)const{const int col=columns(c);return col*(col==4?3:2);}
static bool detail(const Catalog& c){return !c.entries.empty()&&c.entries[0].kind==EntryKind::Episode;}
bool View::image(const std::string& url,const std::string& key,SDL_Rect bounds){
    auto* t=images?images->get(url,key):nullptr;if(!t)return false;
    int w=0,h=0;SDL_QueryTexture(t,nullptr,nullptr,&w,&h);if(!w||!h)return false;
    const double scale=std::min(static_cast<double>(bounds.w)/w,static_cast<double>(bounds.h)/h);
    SDL_Rect dst{bounds.x+(bounds.w-int(w*scale))/2,bounds.y+(bounds.h-int(h*scale))/2,int(w*scale),int(h*scale)};
    SDL_RenderCopy(renderer,t,nullptr,&dst);return true;
}
void View::draw(const Catalog& c,size_t selected,const std::string& status,bool full_memory){
    poll_images();background();
    rect({0,0,232,720},{14,11,25,255});rect({231,0,1,720},line);
    brand(24,28);
    text(27,112,"MENU PRINCIPAL",{126,113,151,255},180,small);
    for(int i=0;i<6;++i){
        const int y=145+i*61;const bool focus=nav_focus==i,active=active_nav==i;
        if(focus||active)rounded(renderer,{13,y,205,48},focus?SDL_Color{91,54,139,255}:SDL_Color{40,27,60,255},12);
        if(active&&!focus)rounded(renderer,{13,y+9,3,30},accent,2);
        const char* glyph[]={"⌂","▣","▤","▧","♡","＋"};
        text(28,y+10,glyph[i],focus?white:active?accent:muted,24,heading,30);
        text(65,y+13,nav[i],focus?white:active?white:muted,145,font,30);
    }
    rounded(renderer,{17,555,198,1},line,1);
    rounded(renderer,{25,583,34,34},panel2,17);text(34,586,"JR",accent,30,small);
    text(69,582,"Xuper TV",white,140,small);text(69,603,"Windows",muted,140,small);
    text(27,668,"v0.14.1",{119,109,139,255},170,small);

    const bool home=c.title=="Inicio";
    const bool episode=detail(c);
    const bool live=!c.entries.empty()&&c.entries[0].kind==EntryKind::Channel;
    const bool movies=!c.entries.empty()&&c.entries[0].kind==EntryKind::Asset&&c.entries[0].program_type=="1";
    const bool series=!c.entries.empty()&&c.entries[0].kind==EntryKind::Asset&&c.entries[0].program_type=="0";
    const bool live=!c.entries.empty()&&c.entries[0].kind==EntryKind::Channel;
    const bool movies=!c.entries.empty()&&c.entries[0].kind==EntryKind::Asset&&c.entries[0].program_type=="1";
    const bool series=!c.entries.empty()&&c.entries[0].kind==EntryKind::Asset&&c.entries[0].program_type=="0";
    text(258,25,c.title.empty()?"Explorar":c.title,white,720,heading,42);
    text(260,68,home?"DESCUBRE ALGO NUEVO":live?"CANALES DISPONIBLES":movies?"PELICULAS Y ESTRENOS":series?"SERIES PARA MARATONEAR":full_memory?"TU CATALOGO":"MODO ALBUM",muted,650,small,24);
    text(1080,31,"⌕  X  BUSCAR",muted,170,small,28);

    int top=145;
    if(home){
        rounded(renderer,{254,105,1002,205},{25,17,42,255},18);
        rounded(renderer,{254,105,1002,205},{38,22,65,255},18);
        // Decorative light panels keep the banner cinematic without fabricating catalog artwork.
        rounded(renderer,{1050,105,206,205},{64,35,100,110},18);
        rounded(renderer,{1120,85,190,245},{85,48,128,70},90);
        if(!c.entries.empty()&&!c.entries[0].artwork.empty())image(c.entries[0].artwork,c.entries[0].artwork_cache,{1000,108,250,198});
        text(286,137,"BIENVENIDO A XUPER TV",{197,160,255,255},580,small,24);
        text(282,170,"Tu mundo de",white,600,large,56);
        text(282,218,"series, peliculas y TV",{218,190,255,255},660,large,58);
        text(286,276,"Elige una categoria y empieza a disfrutar.",muted,650,small,24);
        text(1000,263,"CANALES  •  PELICULAS  •  SERIES",white,245,small,24);
        text(260,324,"Explora el catalogo",white,720,heading,34);
        top=365;
    }
    if(episode){
        rounded(renderer,{258,105,104,144},panel2,12);image(c.artwork,c.artwork_cache,{264,111,92,132});
        text(382,111,"LISTO PARA VER",accent,800,small);
        text(382,145,c.description.empty()?"Elige una pelicula o un episodio para comenzar.":c.description,muted,820,font,90);
        top=270;
    }
    const int cols=columns(c),count=page_size(c),gap=home?12:14;
    const int content_w=970,w=(content_w-gap*(cols-1))/cols;
    const int h=home?112:episode?154:live?132:cols==5?210:cols==4?140:190;
    const size_t start=(selected/count)*count;
    for(size_t i=start;i<c.entries.size()&&i<start+count;++i){
        const auto& e=c.entries[i];const int k=static_cast<int>(i-start);
        const int x=258+(k%cols)*(w+gap),y=top+(k/cols)*(h+14);
        const bool chosen=i==selected&&nav_focus<0;
        rounded(renderer,{x-2,y-2,w+4,h+4},chosen?accent:SDL_Color{34,28,48,255},13);
        rounded(renderer,{x,y,w,h},chosen?SDL_Color{40,30,58,255}:panel,11);
        const bool poster=e.kind==EntryKind::Asset,channel=e.kind==EntryKind::Channel;
        const int art_h=home?65:poster?h-54:channel?76:episode?65:112;
        rounded(renderer,{x+5,y+5,w-10,art_h},channel?SDL_Color{24,19,38,255}:SDL_Color{19,15,29,255},8);
        if(!e.artwork_cache.empty())image(e.artwork,e.artwork_cache,{x+8,y+7,w-16,art_h-4});
        else{
            const char* icon=poster?(e.program_type=="0"?"SERIES":"CINE"):channel?"TV":e.kind==EntryKind::Donation?"+":e.kind==EntryKind::Favorites?"♡":e.kind==EntryKind::Resume?"▶":e.kind==EntryKind::SearchAction?"⌕":e.kind==EntryKind::Section?(e.program_type=="0"?"SERIES":"CINE"):e.kind==EntryKind::LiveSection?"EN VIVO":e.kind==EntryKind::Episode?"▶":"+";
            text(x+15,y+std::max(10,(art_h-34)/2),icon,chosen?white:accent,w-28,heading,42);
        }
        if(poster||channel){
            rounded(renderer,{x+10,y+9,poster?82:86,21},channel?SDL_Color{105,34,54,255}:SDL_Color{17,13,27,230},7);
            text(x+14,y+9,label(e.kind,e.program_type),channel?SDL_Color{255,151,169,255}:SDL_Color{205,174,255,255},poster?76:80,small,22);
        }
        text(x+10,y+art_h+8,e.title,white,w-20,(poster||channel)?small:font,home?34:poster?42:channel?39:54);
        if(!home&&!poster&&!channel)text(x+11,y+h-22,label(e.kind,e.program_type),muted,w-22,small,20);
        if(chosen&&favorite&&channel){rounded(renderer,{x+w-39,y+8,28,26},accent,8);text(x+w-33,y+7,"♡",white,22,font);}
    }
    if(c.entries.empty())text(280,200,c.title=="Favoritos"?"Tus canales favoritos apareceran aqui.":"No hay contenido para mostrar.",muted,850,font,150);
    rounded(renderer,{254,638,1002,1},line,1);
    text(260,648,status,muted,720,small,28);
    text(260,682,"ENTER Abrir    ESC Volver    X Buscar    F Favorito    PgUp/PgDn Pagina",white,960,small,24);
    if(!c.entries.empty()&&selected<c.entries.size()&&c.entries[selected].kind==EntryKind::Channel){
        rounded(renderer,{1080,104,148,28},panel2,9);text(1091,108,favorite?"Quitar favorito":"＋ Favorito",accent,140,small,22);
    }else if(!c.entries.empty())text(1130,108,std::to_string(selected+1)+" / "+std::to_string(c.entries.size()),muted,120,small,24);
    SDL_RenderPresent(renderer);
}
int View::hit(const Catalog& c,size_t selected,int x,int y)const{
    if(!c.entries.empty()&&selected<c.entries.size()&&c.entries[selected].kind==EntryKind::Channel&&x>=1080&&x<=1228&&y>=104&&y<=132)return -8;
    if(x>=13&&x<=218&&y>=145&&y<511){const int row=(y-145)/61;if((y-145)%61<48)return -2-row;}
    const bool home=c.title=="Inicio",episode=detail(c);
    const int cols=columns(c),count=page_size(c),gap=home?12:14;
    const int content_w=970,w=(content_w-gap*(cols-1))/cols;
    const int top=home?365:episode?270:145,h=home?112:episode?154:(!c.entries.empty()&&c.entries[0].kind==EntryKind::Channel)?132:cols==5?210:cols==4?140:190;
    if(x<258||y<top||y>=638)return -1;
    const int col=(x-258)/(w+gap),row=(y-top)/(h+14);
    if(col>=cols||(x-258)%(w+gap)>=w||(y-top)%(h+14)>=h)return -1;
    const size_t i=(selected/count)*count+row*cols+col;
    return i<c.entries.size()&&i<(selected/count)*count+count?static_cast<int>(i):-1;
}
void View::donation(){
    background();brand(76,52);
    text(76,170,"Apoya el desarrollo",white,720,large);
    text(76,252,"Tu aportacion ayuda a seguir mejorando\nXuper TV para Windows.",muted,680,heading,100);
    text(76,382,"Escanea el QR con tu celular y revisa\nel destinatario y el importe en Mercado Pago.",white,680,font,80);
    text(76,492,"Aportacion voluntaria. Puedes usar la app sin donar.",accent,680,font,70);
    text(76,590,"link.mercadopago.com.mx/\nxuperbyjonaruacho",muted,680,font,70);
    rounded(renderer,{842,206,340,340},{255,255,255,255},12);
    for(int y=0;y<33;++y)for(int x=0;x<33;++x)if(donation_qr[y][x]=='1')rect({848+(x+4)*8,212+(y+4)*8,8,8},{0,0,0,255});
    text(900,565,"MERCADO PAGO",accent,300,small);
    text(76,680,"B / A / Y  Continuar a la app",white,900,small);SDL_RenderPresent(renderer);
}
void View::close(){
    images.reset();for(auto** f:{&font,&small,&large,&heading})if(*f){TTF_CloseFont(*f);*f=nullptr;}
    if(renderer){SDL_DestroyRenderer(renderer);renderer=nullptr;}
    if(window){SDL_DestroyWindow(window);window=nullptr;}
    if(TTF_WasInit())TTF_Quit();if(SDL_WasInit(SDL_INIT_VIDEO))SDL_Quit();
}
}