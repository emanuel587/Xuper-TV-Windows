
#include "view.hpp"
#include "trace.hpp"
#include "donation_qr.hpp"
#include "platform.hpp"
#include <algorithm>

namespace xs {

static constexpr SDL_Color white{245,247,252,255};
static constexpr SDL_Color muted{153,164,184,255};
static constexpr SDL_Color accent{104,190,255,255};
static constexpr SDL_Color panel{26,31,46,255};
static constexpr SDL_Color gold{255,207,113,255};
static constexpr SDL_Color sidebar{12,16,27,255};
static constexpr SDL_Color surface{20,25,39,255};
static constexpr SDL_Color line{39,47,65,255};
static constexpr SDL_Color purple{141,112,255,255};

static const char* nav[]={
    "Inicio",
    "TV en vivo",
    "Peliculas",
    "Series",
    "Favoritos",
    "Apoyar"
};

static const char* nav_icon[]={
    "H",
    "TV",
    "P",
    "S",
    "*",
    "+"
};

static const char* label(EntryKind kind,const std::string& type){
    switch(kind){
        case EntryKind::Channel: return "EN VIVO";
        case EntryKind::Asset: return type=="0"?"SERIE":"PELICULA";
        case EntryKind::Episode: return "REPRODUCIR";
        case EntryKind::VodPage:
        case EntryKind::SearchPage: return "SIGUIENTE PAGINA";
        case EntryKind::Category:
        case EntryKind::VodCategory: return "CATEGORIA";
        default: return "EXPLORAR";
    }
}

bool View::open(std::string& error){
    trace("view:init");

#ifndef __SWITCH__
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"0");
    const Uint32 sdl_flags=
        SDL_INIT_VIDEO|
        SDL_INIT_EVENTS|
        SDL_INIT_GAMECONTROLLER;
#else
    const Uint32 sdl_flags=
        SDL_INIT_VIDEO|
        SDL_INIT_EVENTS;
#endif

    if(SDL_Init(sdl_flags)!=0){
        error=SDL_GetError();
        return false;
    }

    if(TTF_Init()!=0){
        error=TTF_GetError();
        return false;
    }

    window=SDL_CreateWindow(
        "Xuper TV para Windows",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        720,
        SDL_WINDOW_RESIZABLE
    );

    if(!window){
        error=SDL_GetError();
        return false;
    }

    renderer=SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC
    );

    if(!renderer){
        renderer=SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_SOFTWARE
        );
    }

    if(!renderer){
        error=SDL_GetError();
        return false;
    }

    SDL_RenderSetLogicalSize(renderer,1280,720);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"1");

#ifdef __SWITCH__
    const char* path="sdmc:/switch/xuperswitch/font.ttf";
#else
    const std::string font_path=find_font();
    const char* path=font_path.c_str();
#endif

    font=TTF_OpenFont(path,22);
    small=TTF_OpenFont(path,17);
    large=TTF_OpenFont(path,46);
    heading=TTF_OpenFont(path,30);

    if(!font||!small||!large||!heading){
        error=TTF_GetError();
        return false;
    }

    images=std::make_unique<Artwork>(renderer);

    trace("view:ready");
    return true;
}

void View::rect(SDL_Rect r,SDL_Color c){
    SDL_SetRenderDrawColor(
        renderer,
        c.r,
        c.g,
        c.b,
        c.a
    );

    SDL_RenderFillRect(renderer,&r);
}

void View::text(
    int x,
    int y,
    const std::string& value,
    SDL_Color color,
    int width,
    TTF_Font* type,
    int max_height
){
    if(value.empty()||width<=0)return;

    auto* s=TTF_RenderUTF8_Blended_Wrapped(
        type?type:font,
        value.c_str(),
        color,
        width
    );

    if(!s)return;

    auto* t=SDL_CreateTextureFromSurface(renderer,s);
    SDL_Rect dst{x,y,s->w,s->h};

    if(t){
        if(max_height>0){
            SDL_Rect clip{x,y,width,max_height};
            SDL_RenderSetClipRect(renderer,&clip);
        }

        SDL_RenderCopy(renderer,t,nullptr,&dst);

        SDL_RenderSetClipRect(renderer,nullptr);
        SDL_DestroyTexture(t);
    }

    SDL_FreeSurface(s);
}

void View::background(){
    for(int y=0;y<720;++y){
        const auto r=static_cast<Uint8>(7+y*8/720);
        const auto g=static_cast<Uint8>(10+y*10/720);
        const auto b=static_cast<Uint8>(19+y*17/720);

        SDL_SetRenderDrawColor(
            renderer,
            r,
            g,
            b,
            255
        );

        SDL_RenderDrawLine(renderer,0,y,1280,y);
    }
}

void View::brand(int x,int y,bool big){
    const int n=big?56:32;

    rect({x,y,n,n},accent);
    rect({x+4,y+4,n-8,n-8},{17,25,40,255});

    text(
        x+(big?10:6),
        y+(big?-2:-1),
        "X",
        accent,
        n,
        big?large:heading
    );

    text(
        x+n+12,
        y-3,
        "XUPER TV",
        white,
        380,
        big?large:font
    );

    text(
        x+n+14,
        y+(big?54:28),
        "ENTRETENIMIENTO",
        accent,
        400,
        small
    );
}

void View::splash(const std::string& status){
    background();

    brand(405,218,true);

    text(
        405,
        330,
        "Tu entretenimiento, en un solo lugar.",
        muted,
        650,
        font
    );

    rect({405,398,470,4},{32,47,65,255});

    const int p=static_cast<int>((SDL_GetTicks()/5)%350);
    rect({405+p,398,120,4},accent);

    text(405,425,status,white,650,font,80);

    text(
        405,
        560,
        "Apoyar el desarrollo es voluntario. Disponible en Inicio.",
        muted,
        680,
        small
    );

    text(
        405,
        670,
        "0.14.1  |  Windows",
        muted,
        700,
        small
    );

    SDL_RenderPresent(renderer);
}

int View::columns(const Catalog& c)const{
    if(!c.entries.empty()&&c.entries[0].kind==EntryKind::Asset)
        return 5;

    if(!c.entries.empty()&&c.entries[0].kind==EntryKind::Channel)
        return 4;

    return 3;
}

int View::page_size(const Catalog& c)const{
    const int col=columns(c);
    return col*(col==4?3:2);
}

static bool detail(const Catalog& c){
    return !c.entries.empty()&&
           c.entries[0].kind==EntryKind::Episode;
}

bool View::image(
    const std::string& url,
    const std::string& key,
    SDL_Rect bounds
){
    auto* t=images?images->get(url,key):nullptr;

    if(!t)return false;

    int w=0,h=0;

    SDL_QueryTexture(t,nullptr,nullptr,&w,&h);

    if(!w||!h)return false;

    const double scale=std::min(
        static_cast<double>(bounds.w)/w,
        static_cast<double>(bounds.h)/h
    );

    SDL_Rect dst{
        bounds.x+(bounds.w-int(w*scale))/2,
        bounds.y+(bounds.h-int(h*scale))/2,
        int(w*scale),
        int(h*scale)
    };

    SDL_RenderCopy(renderer,t,nullptr,&dst);

    return true;
}

void View::draw(
    const Catalog& c,
    size_t selected,
    const std::string& status,
    bool full_memory
){
    poll_images();
    background();

    // =========================================
    // MENU LATERAL
    // =========================================

    rect({0,0,240,720},sidebar);
    rect({239,0,1,720},line);

    brand(24,30);

    rect({16,112,207,1},line);

    text(
        27,
        124,
        "MENU PRINCIPAL",
        muted,
        190,
        small,
        24
    );

    for(int i=0;i<6;++i){
        const int y=157+i*62;

        const bool focus=nav_focus==i;
        const bool active=active_nav==i;

        if(active||focus){
            rect(
                {13,y,3,46},
                focus?purple:accent
            );

            rect(
                {16,y,207,46},
                focus
                    ?SDL_Color{37,39,61,255}
                    :SDL_Color{24,36,54,255}
            );
        }

        // Icono del menu
        rect(
            {27,y+8,30,30},
            (focus||active)
                ?SDL_Color{39,54,78,255}
                :SDL_Color{23,31,47,255}
        );

        text(
            32,
            y+12,
            nav_icon[i],
            (focus||active)?accent:muted,
            24,
            small,
            25
        );

        // Nombre de la seccion
        text(
            68,
            y+8,
            nav[i],
            (focus||active)?white:muted,
            145,
            font,
            32
        );
    }

    // Pie del menu
    rect({16,590,207,1},line);

    text(26,610,"XUPER TV",white,190,small);
    text(26,636,"Windows",accent,190,small);
    text(26,670,"v0.14.1",muted,190,small);

    // =========================================
    // ENCABEZADO PRINCIPAL
    // =========================================

    text(
        278,
        25,
        c.title.empty()?"Explorar":c.title,
        white,
        950,
        heading,
        44
    );

    text(
        280,
        72,
        full_memory
            ?"TV EN VIVO   /   PELICULAS   /   SERIES"
            :"MODO ALBUM",
        full_memory?muted:gold,
        950,
        small,
        28
    );

    rect({278,105,946,1},line);

    int top=145;

    // =========================================
    // PANEL DE DETALLES
    // =========================================

    if(detail(c)){
        rect({278,115,100,138},panel);

        image(
            c.artwork,
            c.artwork_cache,
            {282,119,92,130}
        );

        text(
            398,
            122,
            "LISTO PARA VER",
            accent,
            800,
            small
        );

        text(
            398,
            156,
            c.description.empty()
                ?"Elige una pelicula o un episodio para comenzar."
                :c.description,
            muted,
            800,
            font,
            94
        );

        top=273;
    }
    else{
        const std::string count_text=
            std::to_string(c.entries.size())+
            " opciones";

        text(
            280,
            113,
            count_text,
            accent,
            930,
            small,
            24
        );
    }

    // =========================================
    // CATALOGO
    // =========================================

    const int cols=columns(c);
    const int count=page_size(c);
    const int gap=14;
    const int w=(946-gap*(cols-1))/cols;

    const int h=
        detail(c)
            ?160
            :cols==5
                ?235
                :cols==4
                    ?148
                    :222;

    const size_t start=(selected/count)*count;

    for(
        size_t i=start;
        i<c.entries.size()&&i<start+count;
        ++i
    ){
        const auto& e=c.entries[i];

        const int k=static_cast<int>(i-start);

        const int x=278+(k%cols)*(w+gap);
        const int y=top+(k/cols)*(h+14);

        const bool chosen=
            i==selected&&nav_focus<0;

        // Borde de seleccion
        rect(
            {x-3,y-3,w+6,h+6},
            chosen
                ?accent
                :SDL_Color{21,29,44,255}
        );

        rect({x,y,w,h},panel);

        const bool poster=e.kind==EntryKind::Asset;
        const bool channel=e.kind==EntryKind::Channel;

        const int art_h=
            poster?h-63
            :channel?h-53
            :detail(c)?68
            :128;

        rect(
            {x+6,y+6,w-12,art_h},
            {18,26,40,255}
        );

        // Imagen o icono alternativo
        if(
            e.artwork_cache.empty()||
            !image(
                e.artwork,
                e.artwork_cache,
                {x+10,y+9,w-20,art_h-6}
            )
        ){
            const char* icon=
                poster
                    ?(e.program_type=="0"?"SERIES":"CINE")
                :channel
                    ?"TV"
                :e.kind==EntryKind::Donation
                    ?"+"
                :e.kind==EntryKind::Favorites
                    ?" * "
                :e.kind==EntryKind::Resume
                    ?">"
                :e.kind==EntryKind::SearchAction
                    ?"?"
                :e.kind==EntryKind::Section
                    ?(e.program_type=="0"?"SERIES":"CINE")
                :e.kind==EntryKind::LiveSection
                    ?"EN VIVO"
                :e.kind==EntryKind::Episode
                    ?">"
                :"+";

            text(
                x+16,
                y+std::max(12,(art_h-36)/2),
                icon,
                chosen?accent:muted,
                w-30,
                heading,
                45
            );
        }

        // Etiqueta de pelicula o canal
        if(poster||channel){
            rect(
                {x+9,y+8,poster?80:89,22},
                {16,28,39,255}
            );

            text(
                x+12,
                y+9,
                label(e.kind,e.program_type),
                accent,
                poster?80:89,
                small,
                22
            );
        }

        // Nombre del contenido
        text(
            x+10,
            y+art_h+10,
            e.title,
            white,
            w-20,
            poster||channel?small:font,
            poster?47:channel?43:60
        );

        if(!poster&&!channel){
            text(
                x+12,
                y+h-25,
                label(e.kind,e.program_type),
                muted,
                w-24,
                small,
                22
            );
        }

        // Favorito seleccionado
        if(chosen&&favorite&&channel){
            rect(
                {x+w-44,y+9,32,28},
                accent
            );

            text(
                x+w-38,
                y+8,
                "*",
                {10,25,30,255},
                26,
                font
            );
        }
    }

    // =========================================
    // CATALOGO VACIO
    // =========================================

    if(c.entries.empty()){
        text(
            280,
            200,
            c.title=="Favoritos"
                ?"Tus canales favoritos apareceran aqui.\nSelecciona un canal y pulsa el stick derecho para guardarlo."
                :"No hay contenido para mostrar.",
            muted,
            850,
            font,
            150
        );
    }

    // =========================================
    // BARRA INFERIOR
    // =========================================

    rect({260,631,1000,1},line);

    text(
        280,
        642,
        status,
        muted,
        940,
        small,
        30
    );

    text(
        280,
        681,
        "A Abrir   B Volver   X Buscar   Y Inicio   R3 Favorito   L/R Pagina   + Salir",
        white,
        970,
        small,
        24
    );

    // =========================================
    // ACCIONES DEL ELEMENTO SELECCIONADO
    // =========================================

    if(
        !c.entries.empty()&&
        selected<c.entries.size()&&
        c.entries[selected].kind==EntryKind::Channel
    ){
        rect({1080,113,145,26},panel);

        text(
            1090,
            115,
            favorite?"Quitar favorito":"+ Favorito",
            accent,
            140,
            small,
            24
        );
    }
    else if(!c.entries.empty()){
        text(
            1130,
            113,
            std::to_string(selected+1)+
                " / "+
                std::to_string(c.entries.size()),
            muted,
            120,
            small,
            24
        );
    }

    SDL_RenderPresent(renderer);
}

// =============================================
// DETECCION DE CLICS
// =============================================

int View::hit(
    const Catalog& c,
    size_t selected,
    int x,
    int y
)const{

    if(
        !c.entries.empty()&&
        selected<c.entries.size()&&
        c.entries[selected].kind==EntryKind::Channel&&
        x>=1080&&x<=1225&&
        y>=113&&y<=139
    ){
        return -8;
    }

    // Menu lateral
    if(
        x>=16&&x<=223&&
        y>=157&&y<529
    ){
        const int row=(y-157)/62;

        if(
            row>=0&&
            row<6&&
            (y-157)%62<46
        ){
            return -2-row;
        }
    }

    // Tarjetas del catalogo
    const int cols=columns(c);
    const int count=page_size(c);
    const int w=(946-14*(cols-1))/cols;

    const int top=detail(c)?273:145;

    const int h=
        detail(c)
            ?160
            :cols==5
                ?235
                :cols==4
                    ?148
                    :222;

    if(x<278||y<top||y>=630)
        return -1;

    const int col=(x-278)/(w+14);
    const int row=(y-top)/(h+14);

    if(
        col>=cols||
        (x-278)%(w+14)>=w||
        (y-top)%(h+14)>=h
    ){
        return -1;
    }

    const size_t page_start=(selected/count)*count;

    const size_t i=
        page_start+
        row*cols+
        col;

    if(
        i<c.entries.size()&&
        i<page_start+count
    ){
        return static_cast<int>(i);
    }

    return -1;
}

// =============================================
// PANTALLA DE APOYO
// =============================================

void View::donation(){
    background();

    brand(76,52);

    text(
        76,
        170,
        "Apoya el desarrollo",
        white,
        720,
        large
    );

    text(
        76,
        252,
        "Tu aportacion ayuda a seguir mejorando\nXuper TV.",
        muted,
        680,
        heading,
        100
    );

    text(
        76,
        382,
        "Escanea el QR con tu celular y revisa\nel destinatario y el importe en Mercado Pago.",
        white,
        680,
        font,
        80
    );

    text(
        76,
        492,
        "Aportacion voluntaria. Puedes usar la app sin donar.",
        accent,
        680,
        font,
        70
    );

    text(
        76,
        590,
        "link.mercadopago.com.mx/\nxuperbyjonaruacho",
        muted,
        680,
        font,
        70
    );

    rect(
        {842,206,340,340},
        {255,255,255,255}
    );

    for(int y=0;y<33;++y){
        for(int x=0;x<33;++x){
            if(donation_qr[y][x]=='1'){
                rect(
                    {848+(x+4)*8,212+(y+4)*8,8,8},
                    {0,0,0,255}
                );
            }
        }
    }

    text(
        900,
        565,
        "MERCADO PAGO",
        accent,
        300,
        small
    );

    text(
        76,
        680,
        "B / A / Y  Continuar a la app",
        white,
        900,
        small
    );

    SDL_RenderPresent(renderer);
}

// =============================================
// CIERRE
// =============================================

void View::close(){
    images.reset();

    for(auto** f:{&font,&small,&large,&heading}){
        if(*f){
            TTF_CloseFont(*f);
            *f=nullptr;
        }
    }

    if(renderer){
        SDL_DestroyRenderer(renderer);
        renderer=nullptr;
    }

    if(window){
        SDL_DestroyWindow(window);
        window=nullptr;
    }

    if(TTF_WasInit())
        TTF_Quit();

    if(SDL_WasInit(SDL_INIT_VIDEO))
        SDL_Quit();
}

}
