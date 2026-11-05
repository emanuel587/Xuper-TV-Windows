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
int View::columns(const Catalog&)const{return 1;}
int View::page_size(const Catalog&)const{return 1;}
bool View::animating()const{return SDL_GetTicks()-animation_started<420;}

static void icon(SDL_Renderer* r,int kind,int cx,int cy,SDL_Color color,int scale=1){
    SDL_SetRenderDrawColor(r,color.r,color.g,color.b,color.a);
    auto line=[&](int x1,int y1,int x2,int y2){SDL_RenderDrawLine(r,cx+x1*scale,cy+y1*scale,cx+x2*scale,cy+y2*scale);};
    auto circle=[&](int x,int y,int rad){for(int a=0;a<360;a+=8){const double t=a*3.141592653589793/180.0;SDL_RenderDrawPoint(r,cx+(x+int(std::cos(t)*rad))*scale,cy+(y+int(std::sin(t)*rad))*scale);}};
    switch(kind){
        case 0: line(-9,0,0,-8);line(0,-8,9,0);line(-6,-2,-6,9);line(6,-2,6,9);line(-6,9,6,9);line(-2,9,-2,2);line(2,9,2,2);break;
        case 1: line(-9,-7,9,-7);line(-9,-7,-9,7);line(9,-7,9,7);line(-9,7,9,7);line(-5,-3,5,-3);line(-5,1,2,1);line(-5,5,5,5);break;
        case 2: circle(0,0,8);line(-2,-4,5,0);line(5,0,-2,4);line(-2,4,-2,-4);break;
        case 3: line(-8,-7,8,-7);line(-8,-7,-8,7);line(8,-7,8,7);line(-8,7,8,7);line(-4,-3,4,-3);line(-4,1,4,1);line(-4,5,2,5);break;
        case 4: line(0,-9,8,-5);line(8,-5,6,4);line(6,4,0,9);line(0,9,-6,4);line(-6,4,-8,-5);line(-8,-5,0,-9);line(-5,-2,5,-2);break;
        case 5: circle(0,-2,7);line(5,4,10,9);break;
        case 6: line(-8,-8,8,-8);line(-8,0,8,0);line(-8,8,8,8);circle(-10,-8,1);circle(-10,0,1);circle(-10,8,1);break;
        case 7: line(-9,-6,9,-6);line(-9,0,9,0);line(-9,6,9,6);break;
        case 8: circle(0,0,8);line(0,-5,0,0);line(0,0,4,3);break;
        case 9: circle(0,-4,4);line(-8,9,-6,3);line(-6,3,0,1);line(0,1,6,3);line(6,3,8,9);line(-8,9,8,9);break;
        case 10: line(-8,-6,8,-6);line(-8,-6,-8,5);line(-8,5,8,5);line(8,5,8,-6);line(-3,8,3,8);line(0,5,0,8);break;
        case 11: line(-9,-7,9,-7);line(-9,-7,-3,1);line(-3,1,-3,8);line(-3,8,3,8);line(3,8,3,1);line(3,1,9,-7);break;
        case 12: circle(0,0,7);line(5,5,10,10);break;
        case 13: circle(0,-1,7);line(-3,7,-3,10);line(3,7,3,10);line(-3,10,3,10);break;
        case 14: line(-8,-7,8,-7);line(-8,-7,-8,3);line(8,-7,8,3);line(-8,3,8,3);line(-4,3,0,8);line(0,8,4,3);break;
        default: circle(0,0,7);break;
    }
}
void View::draw(const Catalog& c,size_t selected,const std::string& status,bool full_memory){
    poll_images();
    const Uint32 now=SDL_GetTicks();
    if(selected!=last_selected||nav_focus!=last_nav_focus||active_nav!=last_active_nav||c.title!=last_catalog_title){
        animation_started=now;last_selected=selected;last_nav_focus=nav_focus;last_active_nav=active_nav;last_catalog_title=c.title;
    }
    const float transition=ease_out(float(now-animation_started)/260.0f);
    background();

    // Barra superior limpia, con iconografia vectorial consistente.
    rect({0,0,1280,68},{12,15,23,255});
    rect({0,67,1280,1},{43,48,59,255});
    brand(22,14);
    const SDL_Color topmuted{177,185,198,255};
    icon(renderer,12,952,34,topmuted,1);
    text(970,23,"Buscar",topmuted,68,small,24);
    icon(renderer,11,1061,34,topmuted,1);
    icon(renderer,8,1110,34,topmuted,1);
    icon(renderer,9,1157,34,topmuted,1);
    icon(renderer,10,1201,34,topmuted,1);
    text(1218,23,"16:50",white,55,small,24);

    // Navegacion lateral: opciones reales del port, sin inventar secciones no soportadas.
    rect({0,68,205,652},{13,17,25,255});
    rect({204,68,1,652},{40,47,59,255});
    const char* labels[]={"DESTACADOS","TV","PELICULA","SERIES","FAVORITOS","APOYAR"};
    const int icons[]={0,1,2,3,4,14};
    text(24,96,"MENU PRINCIPAL",{112,125,145,255},165,small,22);
    for(int i=0;i<6;++i){
        const int y=137+i*57;const bool focused=nav_focus==i,active=active_nav==i;
        if(focused||active){
            rounded(renderer,{10,y-5,183,46},{focused?42:30,focused?58:43,focused?78:57,255},10);
            rounded(renderer,{10,y+5,3,26},{67,176,255,255},2);
        }
        icon(renderer,icons[i],35,y+17,focused||active?SDL_Color{245,249,255,255}:SDL_Color{144,157,174,255});
        text(59,y+6,labels[i],focused||active?white:SDL_Color{171,181,195,255},125,small,28);
    }
    rounded(renderer,{23,510,158,1},{43,48,59,255},1);
    text(24,535,"TU BIBLIOTECA",{112,125,145,255},155,small,22);
    icon(renderer,7,36,580,{145,158,176,255});
    text(58,569,"Mi contenido",muted,120,small,25);
    text(24,674,"XUPER TV  •  WINDOWS",{99,111,129,255},170,small,20);

    const bool is_live=!c.entries.empty()&&c.entries[0].kind==EntryKind::Channel;
    const bool home=c.title=="Inicio";
    const bool is_movie=!c.entries.empty()&&c.entries[0].kind==EntryKind::Asset&&c.entries[0].program_type=="1";
    const bool is_series=!c.entries.empty()&&c.entries[0].kind==EntryKind::Asset&&c.entries[0].program_type=="0";
    const Entry* featured=(!c.entries.empty()&&selected<c.entries.size())?&c.entries[selected]:nullptr;
    const int main_x=225,main_w=1028;
    text(main_x,91,home?"Bienvenido a Xuper TV":c.title,white,690,heading,38);
    text(main_x,128,home?"Todo lo que te gusta, en un solo lugar":is_live?"Selecciona un canal para comenzar a ver":is_movie?"Peliculas para disfrutar":is_series?"Continua con tus series":"Explora el catalogo",muted,700,small,24);

    // Hero panoramico; artwork real del catalogo y degradado de lectura.
    const SDL_Rect hero{main_x,166,main_w,310};
    rounded(renderer,{hero.x,hero.y+7,hero.w,hero.h},{0,0,0,100},16);
    rounded(renderer,hero,{25,31,42,255},16);
    if(featured&&!featured->artwork.empty())image(featured->artwork,featured->artwork_cache,{hero.x+hero.w/2,hero.y,hero.w/2,hero.h},true);
    else if(!c.entries.empty()&&c.entries[0].kind==EntryKind::Channel&&!c.entries[0].artwork.empty())image(c.entries[0].artwork,c.entries[0].artwork_cache,{hero.x+hero.w/2,hero.y,hero.w/2,hero.h},true);
    rounded(renderer,{hero.x,hero.y,hero.w,hero.h},{8,13,22,115},16);
    rounded(renderer,{hero.x,hero.y,hero.w/2+180,hero.h},{10,15,24,210},16);
    rounded(renderer,{hero.x+18,hero.y+22,125,25},{29,88,133,235},8);
    text(hero.x+30,hero.y+26,is_live?"EN DIRECTO":home?"RECOMENDADO":is_movie?"PELICULAS":is_series?"SERIES":"DESTACADO",{225,241,255,255},112,small,18);
    const std::string title=featured?featured->title:(home?"Descubre tu proximo favorito":c.title);
    text(hero.x+24,hero.y+76,title,white,hero.w/2-50,large,100);
    const std::string desc=featured&&!featured->description.empty()?featured->description:(home?"Canales en vivo, peliculas y series listas para ti.":"Selecciona para ver los detalles y comenzar la reproduccion.");
    text(hero.x+27,hero.y+184,desc,{197,207,220,255},hero.w/2-70,font,65);
    rounded(renderer,{hero.x+26,hero.y+254,148,38},{40,145,226,255},10);
    icon(renderer,2,hero.x+47,hero.y+273,{255,255,255,255},1);
    text(hero.x+65,hero.y+261,is_live?"Ver canal":"Ver ahora",white,98,small,26);
    text(hero.x+hero.w-78,hero.y+276,c.entries.empty()?"":std::to_string(selected+1)+"/"+std::to_string(c.entries.size()),{220,228,238,255},65,small,22);

    // Lista vertical de canales a la derecha dentro de la misma escena, al estilo de la referencia.
    text(main_x,500,is_live?"CANALES DISPONIBLES":home?"ACCESO RAPIDO":is_movie?"PELICULAS":"CONTENIDO",white,500,heading,30);
    text(main_x+830,506,"VER TODO  →",{132,183,221,255},190,small,22);
    const size_t list_start=selected>5?selected-5:0;
    const int count=static_cast<int>(std::min<size_t>(6,c.entries.size()-std::min(list_start,c.entries.size())));
    const int card_gap=9,card_y=540,card_h=27;
    for(int n=0;n<count;++n){
        const size_t idx=list_start+static_cast<size_t>(n);
        const Entry& e=c.entries[idx];
        const bool chosen=idx==selected;
        const int y=card_y+n*(card_h+card_gap);
        if(chosen)rounded(renderer,{main_x,y-3,main_w,card_h+6},{34,91,137,220},8);
        else if(n%2==0)rounded(renderer,{main_x,y-3,main_w,card_h+6},{21,27,37,190},8);
        if(!e.artwork.empty())image(e.artwork,e.artwork_cache,{main_x+9,y-1,35,25},true);
        else{
            rounded(renderer,{main_x+9,y-1,35,25},{40,53,69,255},6);
            icon(renderer,e.kind==EntryKind::Channel?2:e.kind==EntryKind::Asset?3:1,main_x+26,y+11,{168,194,220,255},1);
        }
        text(main_x+56,y+1,e.title,chosen?white:SDL_Color{190,201,216,255},main_w-110,small,23);
        if(chosen)rounded(renderer,{main_x+main_w-19,y+7,7,13},{74,184,255,255},3);
    }
    if(c.entries.empty()){
        rounded(renderer,{main_x,535,main_w,90},{21,27,37,255},12);
        text(main_x+22,564,"No hay contenido para mostrar en esta seccion.",muted,850,font,35);
    }
    rounded(renderer,{225,645,1028,1},{43,48,59,255},1);
    text(228,660,status,muted,760,small,22);
    text(228,690,"Flechas: navegar     Enter: abrir     Esc: volver     X: buscar     F: favorito",SDL_Color{112,125,145,255},900,small,20);
    SDL_RenderPresent(renderer);
}
int View::hit(const Catalog& c,size_t selected,int x,int y)const{
    if(x>=10&&x<=193&&y>=132&&y<132+6*57){
        const int row=(y-132)/57;
        if(row>=0&&row<6)return -2-row;
    }
    const int main_x=225,main_w=1028;
    if(x>=main_x&&x<=main_x+main_w&&y>=540&&y<540+6*36){
        const int row=(y-540)/36;
        const size_t list_start=selected>5?selected-5:0;
        const size_t idx=list_start+static_cast<size_t>(row);
        if(idx<c.entries.size())return static_cast<int>(idx);
    }
    if(x>=main_x&&x<=main_x+main_w&&y>=166&&y<=476&&selected<c.entries.size())return static_cast<int>(selected);
    return -1;
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