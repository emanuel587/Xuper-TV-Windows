#include "view.hpp"
#include "trace.hpp"
#include "donation_qr.hpp"
#include "platform.hpp"
#include <algorithm>
#include <cmath>

namespace xs {
static constexpr SDL_Color white{246,245,249,255},muted{160,160,172,255};
static constexpr SDL_Color accent{247,70,91,255};
static constexpr SDL_Color accent2{255,124,87,255};
static constexpr SDL_Color panel{25,25,32,255},panel2{35,35,44,255};
static constexpr SDL_Color line{52,50,61,255},gold{255,207,113,255};
static const char* nav[]={"Inicio","TV en vivo","Peliculas","Series","Favoritos","Apoyar"};

static float clamp01(float v){return std::max(0.0f,std::min(1.0f,v));}
static float ease_out(float v){v=clamp01(v);return 1.0f-(1.0f-v)*(1.0f-v)*(1.0f-v);}
static void rounded(SDL_Renderer* renderer,SDL_Rect r,SDL_Color color,int radius=12){
    if(r.w<=0||r.h<=0)return;
    radius=std::max(0,std::min(radius,std::min(r.w,r.h)/2));
    SDL_SetRenderDrawColor(renderer,color.r,color.g,color.b,color.a);
    for(int y=0;y<r.h;++y){
        int inset=0;
        if(y<radius){const double dy=radius-y-0.5;inset=radius-static_cast<int>(std::sqrt(std::max(0.0,double(radius*radius)-dy*dy)));}
        else if(y>=r.h-radius){const double dy=y-(r.h-radius)+0.5;inset=radius-static_cast<int>(std::sqrt(std::max(0.0,double(radius*radius)-dy*dy)));}
        SDL_RenderDrawLine(renderer,r.x+inset,r.y+y,r.x+r.w-inset-1,r.y+y);
    }
}
static const char* entry_glyph(const Entry& e){
    switch(e.kind){
        case EntryKind::Channel:case EntryKind::LiveSection:return "TV";
        case EntryKind::Asset:return e.program_type=="0"?"SERIE":"CINE";
        case EntryKind::Episode:return "PLAY";
        case EntryKind::Category:case EntryKind::VodCategory:return "▦";
        case EntryKind::Favorites:return "♡";
        case EntryKind::Resume:return "▶";
        case EntryKind::Donation:return "＋";
        case EntryKind::SearchAction:return "⌕";
        default:return "▶";
    }
}
static const char* entry_label(const Entry& e){
    switch(e.kind){
        case EntryKind::Channel:return "EN VIVO";
        case EntryKind::Asset:return e.program_type=="0"?"SERIE":"PELICULA";
        case EntryKind::Episode:return "EPISODIO";
        case EntryKind::Category:return "CATEGORIA";
        case EntryKind::VodCategory:return "EXPLORAR";
        case EntryKind::Section:return e.program_type=="0"?"SERIES":"PELICULAS";
        case EntryKind::LiveSection:return "TELEVISION";
        default:return "DESCUBRIR";
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
    window=SDL_CreateWindow("Xuper TV",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1280,720,SDL_WINDOW_RESIZABLE);
    if(!window){error=SDL_GetError();return false;}
    renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if(!renderer)renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
    if(!renderer){error=SDL_GetError();return false;}
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    SDL_RenderSetLogicalSize(renderer,1280,720);SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"1");
#ifdef __SWITCH__
    const char* path="sdmc:/switch/xuperswitch/font.ttf";
#else
    const std::string font_path=find_font();const char* path=font_path.c_str();
#endif
    font=TTF_OpenFont(path,22);small=TTF_OpenFont(path,16);large=TTF_OpenFont(path,48);heading=TTF_OpenFont(path,30);
    if(!font||!small||!large||!heading){error=TTF_GetError();return false;}
    images=std::make_unique<Artwork>(renderer);animation_started=SDL_GetTicks();trace("view:ready");return true;
}
void View::rect(SDL_Rect r,SDL_Color c){SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,c.a);SDL_RenderFillRect(renderer,&r);}
void View::text(int x,int y,const std::string& value,SDL_Color color,int width,TTF_Font* type,int max_height){
    if(value.empty()||width<=0)return;
    auto* s=TTF_RenderUTF8_Blended_Wrapped(type?type:font,value.c_str(),color,width);if(!s)return;
    auto* t=SDL_CreateTextureFromSurface(renderer,s);SDL_Rect dst{x,y,s->w,s->h};
    if(t){if(max_height>0){SDL_Rect clip{x,y,width,max_height};SDL_RenderSetClipRect(renderer,&clip);}SDL_RenderCopy(renderer,t,nullptr,&dst);SDL_RenderSetClipRect(renderer,nullptr);SDL_DestroyTexture(t);}SDL_FreeSurface(s);
}
void View::background(){
    const Uint32 now=SDL_GetTicks();
    rect({0,0,1280,720},{10,10,14,255});
    // Very subtle animated color bloom: it moves slowly without distracting from artwork.
    const double phase=double(now%9000)/9000.0*6.283185307;
    const int drift=static_cast<int>(std::sin(phase)*46.0);
    rounded(renderer,{940+drift,-205,510,390},{91,31,53,35},190);
    rounded(renderer,{370-drift/2,620,520,210},{45,38,76,20},150);
}
void View::brand(int x,int y,bool big){
    const int n=big?54:42;
    rounded(renderer,{x,y,n,n},accent,big?15:12);
    text(x+(big?9:8),y+(big?-1:0),"X",{255,255,255,255},n,big?large:heading,50);
    text(x+n+11,y-2,"Xuper TV",white,250,big?large:heading,48);
    text(x+n+13,y+(big?51:34),"ENTRETENIMIENTO",{255,157,151,255},230,small,22);
}
bool View::image(const std::string& url,const std::string& key,SDL_Rect bounds,bool cover){
    auto* t=images?images->get(url,key):nullptr;if(!t)return false;
    int w=0,h=0;SDL_QueryTexture(t,nullptr,nullptr,&w,&h);if(!w||!h)return false;
    const double sx=static_cast<double>(bounds.w)/w,sy=static_cast<double>(bounds.h)/h;
    const double scale=cover?std::max(sx,sy):std::min(sx,sy);
    SDL_Rect dst{bounds.x+(bounds.w-int(w*scale))/2,bounds.y+(bounds.h-int(h*scale))/2,int(w*scale),int(h*scale)};
    SDL_RenderSetClipRect(renderer,&bounds);SDL_RenderCopy(renderer,t,nullptr,&dst);SDL_RenderSetClipRect(renderer,nullptr);
    return true;
}
void View::splash(const std::string& status){
    background();brand(405,218,true);
    text(405,330,"Canales, peliculas y series en un solo lugar.",muted,650,font);
    rounded(renderer,{405,398,470,5},{58,34,49,255},3);
    const int p=static_cast<int>((SDL_GetTicks()/5)%350);rounded(renderer,{405+p,398,120,5},accent,3);
    text(405,425,status,white,650,font,80);
    text(405,560,"Preparando tu experiencia...",muted,680,small);
    SDL_RenderPresent(renderer);
}
int View::columns(const Catalog&)const{return 5;}
int View::page_size(const Catalog&)const{return 5;}
bool View::animating()const{return SDL_GetTicks()-animation_started<420;}

void View::draw(const Catalog& c,size_t selected,const std::string& status,bool full_memory){
    poll_images();
    const Uint32 now=SDL_GetTicks();
    if(selected!=last_selected||nav_focus!=last_nav_focus||active_nav!=last_active_nav||c.title!=last_catalog_title){
        animation_started=now;last_selected=selected;last_nav_focus=nav_focus;last_active_nav=active_nav;last_catalog_title=c.title;
    }
    const float transition=ease_out(float(now-animation_started)/340.0f);
    background();

    // Slim category rail inspired by Magis TV's landscape navigation.
    rect({0,0,190,720},{14,14,19,248});
    rect({189,0,1,720},{49,46,57,255});
    brand(22,25);
    text(25,112,"EXPLORAR",{121,119,132,255},150,small,22);
    const char* glyphs[]={"⌂","▣","▤","▧","♡","＋"};
    for(int i=0;i<6;++i){
        const int y=145+i*53;const bool focus=nav_focus==i,active=active_nav==i;
        if(focus||active){
            const Uint8 alpha=static_cast<Uint8>(active?220:150);
            rounded(renderer,{12,y,166,42},{focus?58:38,focus?34:31,focus?49:42,alpha},10);
            rounded(renderer,{12,y+8,3,26},accent,2);
        }
        text(27,y+7,glyphs[i],focus||active?white:muted,24,heading,30);
        text(61,y+10,nav[i],focus||active?white:muted,112,font,28);
    }
    rounded(renderer,{18,514,154,1},line,1);
    rounded(renderer,{22,548,35,35},panel2,10);text(31,551,"X",{255,148,151,255},28,small,28);
    text(66,546,"Xuper TV",white,105,small,21);
    text(66,566,"Windows",muted,100,small,19);
    text(23,674,"XUPER TV  •  0.14.1",{116,112,126,255},155,small,20);

    const bool home=c.title=="Inicio";
    const bool episode=!c.entries.empty()&&c.entries[0].kind==EntryKind::Episode;
    const bool live=!c.entries.empty()&&c.entries[0].kind==EntryKind::Channel;
    const bool movies=!c.entries.empty()&&c.entries[0].kind==EntryKind::Asset&&c.entries[0].program_type=="1";
    const bool series=!c.entries.empty()&&c.entries[0].kind==EntryKind::Asset&&c.entries[0].program_type=="0";
    const Entry* featured=(!c.entries.empty()&&selected<c.entries.size())?&c.entries[selected]:nullptr;

    text(216,23,c.title.empty()?"Descubrir":c.title,white,610,heading,38);
    text(218,61,home?"Tu entretenimiento empieza aqui":live?"Canales para ver ahora":movies?"Peliculas para descubrir":series?"Series para maratonear":episode?"Temporadas y episodios":"Explora el catalogo",muted,650,small,24);
    rounded(renderer,{1000,23,244,41},{29,28,36,255},12);
    rounded(renderer,{1000,23,244,1},line,1);
    text(1017,31,"⌕   Buscar contenido",muted,215,small,25);

    // Cinematic feature area. Real artwork is used whenever the service supplies it.
    rounded(renderer,{211,97,1041,250},{23,22,29,255},18);
    rounded(renderer,{212,98,1038,248},{32,25,34,255},17);
    rounded(renderer,{212,98,5,248},accent,3);
    const int glow=static_cast<int>((1.0f-transition)*12.0f);
    rounded(renderer,{750+glow,99,500,247},{91,30,52,90},17);
    rounded(renderer,{1045,80,280,275},{146,49,70,34},135);
    if(featured&&!featured->artwork.empty()){
        image(featured->artwork,featured->artwork_cache,{865,99,382,246},true);
        rounded(renderer,{842,98,110,248},{23,22,29,95},10);
    }else{
        rounded(renderer,{890,107,310,222},{57,27,43,190},16);
        rounded(renderer,{1015,94,220,249},{107,43,56,80},105);
        rounded(renderer,{1100,132,112,180},{177,69,77,35},56);
        rounded(renderer,{936,149,3,145},{247,83,94,125},2);
    }
    if(home){
        rounded(renderer,{246,123,146,25},{67,34,49,255},8);
        text(258,127,"XUPER TV ORIGINAL",{255,169,162,255},128,small,18);
        text(246,168,"Tu mundo de",white,570,large,56);
        text(246,220,"entretenimiento",{255,170,170,255},600,large,56);
        text(249,285,"Canales en vivo, peliculas y series en una sola experiencia.",muted,620,small,26);
        rounded(renderer,{246,315,126,3},accent,2);
    }else{
        const std::string title=featured?featured->title:c.title;
        const std::string description=featured&&!featured->description.empty()?featured->description:(live?"Explora las categorias y encuentra lo que quieres ver.":"Descubre contenido y continua donde lo dejaste.");
        rounded(renderer,{246,123,110,25},{67,34,49,255},8);
        text(257,127,live?"EN DIRECTO":movies?"PELICULAS":series?"SERIES":episode?"EPISODIOS":"DESTACADO",{255,169,162,255},95,small,18);
        text(246,168,title,white,590,large,62);
        text(249,237,description,muted,560,font,65);
        if(featured&&featured->kind==EntryKind::Channel){
            rounded(renderer,{246,310,112,27},{119,35,52,255},8);
            text(260,314,"● EN VIVO",{255,242,244,255},90,small,20);
        }
    }
    text(1154,322,c.entries.empty()?"":std::to_string(selected+1)+" / "+std::to_string(c.entries.size()),{231,213,218,255},75,small,20);

    const char* shelf=home?"Explora tus secciones":live?"Canales disponibles":movies?"Peliculas populares":series?"Series para ti":episode?"Episodios":"Para descubrir";
    text(216,367,shelf,white,700,heading,33);
    if(!c.entries.empty())text(1080,374,"VER TODO  →",{190,151,159,255},165,small,22);

    const int cols=columns(c),page=page_size(c),gap=14;
    const int left=212,content_w=1040,w=(content_w-gap*(cols-1))/cols;
    const int top=410,h=202;
    const size_t start=(selected/static_cast<size_t>(page))*static_cast<size_t>(page);
    for(size_t i=start;i<c.entries.size()&&i<start+static_cast<size_t>(page);++i){
        const Entry& e=c.entries[i];const int k=static_cast<int>(i-start);
        const int base_x=left+k*(w+gap);
        const bool chosen=i==selected&&nav_focus<0;
        const float card_anim=chosen?transition:1.0f;
        const int lift=chosen?static_cast<int>(8.0f*card_anim):0;
        const int grow=chosen?static_cast<int>(4.0f*card_anim):0;
        const SDL_Rect box{base_x-grow,top-lift,w+grow*2,h+grow};
        rounded(renderer,{box.x+3,box.y+7,box.w,box.h},{0,0,0,110},13);
        if(chosen)rounded(renderer,{box.x-3,box.y-3,box.w+6,box.h+6},{247,70,91,static_cast<Uint8>(125+80*std::sin(double(now%800)/800.0*3.14159))},14);
        rounded(renderer,box,chosen?SDL_Color{49,30,39,255}:SDL_Color{34,32,40,255},12);

        const bool poster=e.kind==EntryKind::Asset;
        const bool channel=e.kind==EntryKind::Channel;
        const int art_h=poster?145:channel?113:115;
        SDL_Rect art{box.x+5,box.y+5,box.w-10,art_h};
        rounded(renderer,art,poster?SDL_Color{23,22,28,255}:channel?SDL_Color{35,29,37,255}:SDL_Color{48,30,41,255},8);
        const bool loaded=!e.artwork_cache.empty()&&image(e.artwork,e.artwork_cache,{art.x+2,art.y+2,art.w-4,art.h-4},true);
        if(!loaded){
            const char* glyph=entry_glyph(e);
            text(art.x+15,art.y+(art.h-38)/2,glyph,chosen?white:SDL_Color{255,139,145,255},art.w-30,heading,44);
            rounded(renderer,{art.x+14,art.y+art.h-12,art.w-28,2},{247,70,91,100},1);
        }
        if(poster||channel){
            rounded(renderer,{box.x+13,box.y+12,poster?91:83,21},poster?SDL_Color{34,29,38,220}:SDL_Color{113,34,51,235},6);
            text(box.x+18,box.y+13,entry_label(e),poster?SDL_Color{255,197,198,255}:SDL_Color{255,232,234,255},poster?82:75,small,18);
        }
        if(channel){
            rounded(renderer,{box.x+box.w-24,box.y+14,7,7},{255,75,91,255},4);
        }
        text(box.x+12,box.y+art_h+13,e.title,chosen?white:SDL_Color{226,223,230,255},box.w-24,small,44);
        if(!home&&!poster&&!channel)text(box.x+12,box.y+h-22,entry_label(e),muted,box.w-24,small,18);
        if(chosen){
            rounded(renderer,{box.x+box.w-35,box.y+box.h-32,24,24},accent,8);
            text(box.x+box.w-29,box.y+box.h-30,"›",white,18,heading,25);
        }
    }
    if(c.entries.empty()){
        rounded(renderer,{214,412,1034,186},panel,14);
        text(248,463,c.title=="Favoritos"?"Tus favoritos apareceran aqui.":"No hay contenido para mostrar.",muted,850,font,90);
    }

    rounded(renderer,{212,638,1040,1},line,1);
    text(216,650,status,muted,780,small,25);
    text(216,683,"Mouse para explorar   •   Enter para abrir   •   Esc para volver   •   X para buscar   •   F para favorito",SDL_Color{120,117,130,255},1010,small,22);
    if(!c.entries.empty()&&selected<c.entries.size()&&c.entries[selected].kind==EntryKind::Channel){
        rounded(renderer,{1076,103,150,29},panel2,8);
        text(1088,107,favorite?"Quitar favorito":"＋ Favorito",accent,135,small,22);
    }
    SDL_RenderPresent(renderer);
}
int View::hit(const Catalog& c,size_t selected,int x,int y)const{
    if(!c.entries.empty()&&selected<c.entries.size()&&c.entries[selected].kind==EntryKind::Channel&&x>=1076&&x<=1226&&y>=103&&y<=132)return -8;
    if(x>=12&&x<=178&&y>=145&&y<145+6*53){const int row=(y-145)/53;if((y-145)%53<42)return -2-row;}
    const int cols=columns(c),page=page_size(c),gap=14,left=212,content_w=1040,w=(content_w-gap*(cols-1))/cols;
    const int top=410,h=202;
    if(x<left||y<top||y>=top+h)return -1;
    const int col=(x-left)/(w+gap);
    if(col>=cols||(x-left)%(w+gap)>=w)return -1;
    const size_t i=(selected/static_cast<size_t>(page))*static_cast<size_t>(page)+static_cast<size_t>(col);
    return i<c.entries.size()?static_cast<int>(i):-1;
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