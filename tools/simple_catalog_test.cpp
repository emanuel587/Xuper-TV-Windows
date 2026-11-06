#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "service.hpp"

namespace {
constexpr UINT WM_RESULT=WM_APP+7;
enum class Task { Bootstrap, Channels, Source };
struct Result {
    Task task;
    xs::LoginAttempt login;
    xs::BrowseResult browse;
    xs::SourceResult source;
    int category_id=-1;
};
HWND window_handle=nullptr, categories_list=nullptr, channels_list=nullptr, status_label=nullptr;
HWND refresh_button=nullptr, source_button=nullptr;
std::shared_ptr<xs::ServiceSession> session;
xs::Catalog root_catalog, current_channels;
std::vector<xs::Entry> categories, channels;
std::thread worker;
bool busy=false;

std::wstring wide(const std::string& value) {
    if(value.empty()) return {};
    const int n=MultiByteToWideChar(CP_UTF8,0,value.c_str(),-1,nullptr,0);
    if(n<=1) return {};
    std::wstring out(static_cast<size_t>(n),L'\0');
    MultiByteToWideChar(CP_UTF8,0,value.c_str(),-1,out.data(),n);
    out.pop_back(); return out;
}
void status(const std::string& value) { SetWindowTextW(status_label,wide(value).c_str()); }
void add_item(HWND list,const std::string& value) {
    const auto text=wide(value); SendMessageW(list,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));
}
void fill_channels(const xs::Catalog& catalog) {
    channels.clear(); SendMessageW(channels_list,LB_RESETCONTENT,0,0);
    for(const auto& entry:catalog.entries) if(entry.kind==xs::EntryKind::Channel) {
        channels.push_back(entry); add_item(channels_list,entry.title);
    }
}
void set_busy(bool value) {
    busy=value;
    EnableWindow(refresh_button,!value);
    EnableWindow(categories_list,!value);
    EnableWindow(channels_list,!value);
    EnableWindow(source_button,!value);
}
void start_task(Task task,int category=-1,int channel=-1) {
    if(busy) return;
    set_busy(true);
    status(task==Task::Bootstrap?"Conectando y cargando catalogo...":
           task==Task::Channels?"Consultando canales de la categoria...":"Solicitando fuente autorizada...");
    if(worker.joinable()) worker.join();
    worker=std::thread([task,category,channel] {
        auto* result=new Result{}; result->task=task; result->category_id=category;
        if(task==Task::Bootstrap) result->login=xs::service_bootstrap("service.json");
        else if(task==Task::Channels) result->browse=xs::service_channels(session,category);
        else if(session&&channel>=0&&static_cast<size_t>(channel)<channels.size())
            result->source=xs::service_source(session,channels[static_cast<size_t>(channel)]);
        PostMessageW(window_handle,WM_RESULT,0,reinterpret_cast<LPARAM>(result));
    });
}
void layout(HWND hwnd) {
    RECT r{}; GetClientRect(hwnd,&r); const int w=r.right-r.left,h=r.bottom-r.top;
    MoveWindow(categories_list,16,48,(w-48)/2,h-112,TRUE);
    MoveWindow(channels_list,32+(w-48)/2,48,(w-48)/2,h-112,TRUE);
    MoveWindow(status_label,16,h-48,w-32,24,TRUE);
    MoveWindow(refresh_button,16,12,130,28,TRUE);
    MoveWindow(source_button,156,12,180,28,TRUE);
}
LRESULT CALLBACK proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    switch(msg) {
    case WM_CREATE: {
        window_handle=hwnd;
        CreateWindowW(L"STATIC",L"Categorias",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,nullptr,nullptr,nullptr);
        CreateWindowW(L"STATIC",L"Canales",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,nullptr,nullptr,nullptr);
        categories_list=CreateWindowW(L"LISTBOX",nullptr,WS_CHILD|WS_VISIBLE|WS_BORDER|LBS_NOTIFY|WS_VSCROLL,0,0,0,0,hwnd,reinterpret_cast<HMENU>(101),nullptr,nullptr);
        channels_list=CreateWindowW(L"LISTBOX",nullptr,WS_CHILD|WS_VISIBLE|WS_BORDER|LBS_NOTIFY|WS_VSCROLL,0,0,0,0,hwnd,reinterpret_cast<HMENU>(102),nullptr,nullptr);
        refresh_button=CreateWindowW(L"BUTTON",L"Conectar / recargar",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,reinterpret_cast<HMENU>(103),nullptr,nullptr);
        source_button=CreateWindowW(L"BUTTON",L"Probar fuente del canal",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,reinterpret_cast<HMENU>(104),nullptr,nullptr);
        status_label=CreateWindowW(L"STATIC",L"Listo.",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,nullptr,nullptr,nullptr);
        start_task(Task::Bootstrap); return 0;
    }
    case WM_SIZE: layout(hwnd); return 0;
    case WM_COMMAND:
        if(LOWORD(wp)==103) { start_task(Task::Bootstrap); return 0; }
        if(LOWORD(wp)==101&&HIWORD(wp)==LBN_SELCHANGE) {
            const int index=static_cast<int>(SendMessageW(categories_list,LB_GETCURSEL,0,0));
            if(index>=0&&static_cast<size_t>(index)<categories.size()) start_task(Task::Channels,categories[index].column_id);
            return 0;
        }
        if((LOWORD(wp)==104||(LOWORD(wp)==102&&HIWORD(wp)==LBN_DBLCLK))) {
            const int index=static_cast<int>(SendMessageW(channels_list,LB_GETCURSEL,0,0));
            if(index>=0) start_task(Task::Source,-1,index);
            else status("Selecciona primero un canal.");
            return 0;
        }
        return 0;
    case WM_RESULT: {
        std::unique_ptr<Result> result(reinterpret_cast<Result*>(lp)); set_busy(false);
        if(result->task==Task::Bootstrap) {
            session=result->login.session; root_catalog=std::move(result->login.catalog);
            categories.clear(); SendMessageW(categories_list,LB_RESETCONTENT,0,0);
            for(const auto& entry:root_catalog.entries) {
                if(entry.kind==xs::EntryKind::Category) { categories.push_back(entry); add_item(categories_list,entry.title); }
            }
            if(categories.empty()) fill_channels(root_catalog);
            status(result->login.status);
        } else if(result->task==Task::Channels) {
            current_channels=std::move(result->browse.catalog); fill_channels(current_channels);
            status(result->browse.status);
        } else {
            status(result->source.ok?"Fuente del canal recibida y autorizada. (La reproduccion se agrega en el siguiente paso.)":result->source.status);
        }
        return 0;
    }
    case WM_DESTROY:
        if(worker.joinable()) worker.join();
        PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}
}

int WINAPI wWinMain(HINSTANCE instance,HINSTANCE, PWSTR,int show) {
    INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_STANDARD_CLASSES}; InitCommonControlsEx(&controls);
    WNDCLASSW wc{}; wc.lpfnWndProc=proc; wc.hInstance=instance; wc.lpszClassName=L"XuperSimpleCatalogTest";
    wc.hCursor=LoadCursor(nullptr,IDC_ARROW); wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);
    RegisterClassW(&wc);
    HWND hwnd=CreateWindowW(wc.lpszClassName,L"Xuper TV - Prueba sencilla",WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,CW_USEDEFAULT,900,620,nullptr,nullptr,instance,nullptr);
    if(!hwnd) return 1;
    ShowWindow(hwnd,show); UpdateWindow(hwnd);
    MSG msg{}; while(GetMessageW(&msg,nullptr,0,0)>0) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return static_cast<int>(msg.wParam);
}