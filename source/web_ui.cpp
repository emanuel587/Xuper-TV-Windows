#ifdef _WIN32

#include "web_ui.hpp"
#include <webview/webview.h>
#include <windows.h>
#include <SDL_syswm.h>
#include <objbase.h>
#include <fstream>
#include <future>
#include <functional>
#include <chrono>
#include <algorithm>
#include <new>
#include <sstream>
#include <utility>

namespace xs {
namespace {

std::string read_text_file(const char* path) {
    std::ifstream file(path,std::ios::binary);
    if(!file)return {};
    std::ostringstream data;
    data<<file.rdbuf();
    return data.str();
}

bool replace_once(std::string& text,const std::string& from,const std::string& to) {
    const auto at=text.find(from);
    if(at==std::string::npos)return false;
    text.replace(at,from.size(),to);
    return true;
}

std::string json_quote(const std::string& value) {
    std::string out="\"";
    for(unsigned char c:value) {
        switch(c) {
            case '\\': out+="\\\\"; break;
            case '"': out+="\\\""; break;
            case '\b': out+="\\b"; break;
            case '\f': out+="\\f"; break;
            case '\n': out+="\\n"; break;
            case '\r': out+="\\r"; break;
            case '\t': out+="\\t"; break;
            default:
                if(c<0x20) {
                    static const char hex[]="0123456789abcdef";
                    out+="\\u00";out+=hex[c>>4];out+=hex[c&15];
                } else out+=static_cast<char>(c);
        }
    }
    out+='"';
    return out;
}

std::string load_frontend() {
    std::string html=read_text_file("web-ui/index.html");
    const std::string css=read_text_file("web-ui/styles.css");
    const std::string js=read_text_file("web-ui/app.js");
    if(html.empty()||css.empty()||js.empty())return {};
    const std::string style="<style>\n"+css+"\n</style>";
    const std::string script="<script>\n"+js+"\n</script>";
    if(!replace_once(html,"<link rel=\"stylesheet\" href=\"styles.css\">",style))return {};
    if(!replace_once(html,"<script src=\"app.js\"></script>",script))return {};
    return html;
}

struct DispatchTask {
    std::function<void(webview_t)> work;
    std::promise<void> completed;
};

void dispatch_task(webview_t w,void* arg) {
    std::unique_ptr<DispatchTask> task(static_cast<DispatchTask*>(arg));
    try { task->work(w); task->completed.set_value(); }
    catch(...) { task->completed.set_exception(std::current_exception()); }
}

} // namespace

WebUi::~WebUi(){stop();}

bool WebUi::start(std::string& error,SDL_Window* parent) {
    parent_window_=nullptr;
    if(parent) {
        SDL_SysWMinfo info{};SDL_VERSION(&info.version);
        if(!SDL_GetWindowWMInfo(parent,&info)||info.subsystem!=SDL_SYSWM_WINDOWS) {
            error="No se pudo obtener la ventana principal de Windows para integrar el reproductor.";
            return false;
        }
        parent_window_=info.info.win.window;
    }
    if(thread_.joinable())return active();
    event_type_=SDL_RegisterEvents(1);
    if(event_type_==static_cast<Uint32>(-1)) {
        error="SDL no pudo reservar eventos para la interfaz web.";
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        startup_done_=false;startup_ok_=false;stop_requested_=false;startup_error_.clear();
    }
    thread_=std::thread(&WebUi::thread_main,this);
    std::unique_lock<std::mutex> lock(mutex_);
    if(!startup_cv_.wait_for(lock,std::chrono::seconds(20),[this]{return startup_done_;})) {
        error="WebView2 tardó demasiado en inicializar.";
        stop_requested_=true;
        lock.unlock();
        if(thread_.joinable())thread_.join();
        return false;
    }
    if(!startup_ok_) {
        error=startup_error_.empty()?"No se pudo iniciar WebView2.":startup_error_;
        lock.unlock();if(thread_.joinable())thread_.join();return false;
    }
    return true;
}

void WebUi::thread_main() {
    const HRESULT com_result=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    if(FAILED(com_result)) {
        std::lock_guard<std::mutex> lock(mutex_);
        startup_error_="No se pudo inicializar COM para WebView2.";
        startup_done_=true;startup_cv_.notify_all();return;
    }

    webview_t webview=webview_create(0,nullptr);
    if(webview&&parent_window_) {
        HWND child=static_cast<HWND>(webview_get_window(webview));
        HWND parent=static_cast<HWND>(parent_window_);
        if(child&&parent) {
            SetLastError(0);
            SetParent(child,parent);
            LONG_PTR style=GetWindowLongPtr(child,GWL_STYLE);
            style=(style&~static_cast<LONG_PTR>(WS_POPUP|WS_CAPTION|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_SYSMENU))|
                  WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
            SetWindowLongPtr(child,GWL_STYLE,style);
            RECT bounds{};GetClientRect(parent,&bounds);
            SetWindowPos(child,nullptr,0,0,bounds.right-bounds.left,bounds.bottom-bounds.top,
                         SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_SHOWWINDOW);
        }
    }
    if(!webview) {
        std::lock_guard<std::mutex> lock(mutex_);
        startup_error_="No se encontró WebView2 Runtime. Instala Microsoft Edge WebView2 Runtime.";
        startup_done_=true;startup_cv_.notify_all();
        CoUninitialize();return;
    }

    bindings_[0]={this,Ready};
    bindings_[1]={this,Navigate};
    bindings_[2]={this,Play};
    bindings_[3]={this,Search};
    bindings_[4]={this,SelectItem};
    bindings_[5]={this,Back};
    bindings_[6]={this,ToggleFavorite};
    bindings_[7]={this,Quit};
    bindings_[8]={this,ToggleFullscreen};
    const char* names[]={"xuperReady","xuperNavigate","xuperPlay","xuperSearch","xuperSelectItem","xuperBack","xuperFavorite","xuperQuit","xuperFullscreen"};
    bool ok=webview_set_title(webview,"Xuper TV")>=0 &&
            webview_set_size(webview,1360,850,WEBVIEW_HINT_NONE)>=0;
    if(ok&&parent_window_) {
        HWND child=static_cast<HWND>(webview_get_window(webview));
        HWND parent=static_cast<HWND>(parent_window_);
        RECT bounds{};GetClientRect(parent,&bounds);
        if(child)SetWindowPos(child,nullptr,0,0,bounds.right-bounds.left,bounds.bottom-bounds.top,
                              SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_SHOWWINDOW);
    }
    for(size_t i=0;i<9&&ok;++i)
        ok=webview_bind(webview,names[i],&WebUi::binding_callback,&bindings_[i])>=0;

    const std::string html=load_frontend();
    if(html.empty())ok=false;
    if(ok)ok=webview_set_html(webview,html.c_str())>=0;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if(ok) {
            handle_=webview;
            active_=true;
        } else startup_error_="No se pudo cargar web-ui/index.html, styles.css o app.js.";
        startup_ok_=ok;startup_done_=true;
    }
    startup_cv_.notify_all();

    bool should_run=ok;
    { std::lock_guard<std::mutex> lock(mutex_); should_run=should_run&&!stop_requested_; }
    if(should_run)webview_run(webview);
    bool should_quit=false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        handle_=nullptr;
        should_quit=!stop_requested_;
    }
    webview_destroy(webview);
    active_=false;
    CoUninitialize();

    if(should_quit) {
        SDL_Event event{};event.type=SDL_QUIT;SDL_PushEvent(&event);
    }
}

void WebUi::binding_callback(const char* id,const char* request,void* arg) {
    auto* context=static_cast<BindingContext*>(arg);
    if(!context||!context->owner)return;
    std::string value;
    if(request&&context->code!=Ready&&context->code!=Play&&context->code!=Back)
        value=webview::detail::json_parse(request,"",0);
    context->owner->push_event(context->code,value);
    webview_t handle=nullptr;
    {
        std::lock_guard<std::mutex> lock(context->owner->mutex_);
        handle=static_cast<webview_t>(context->owner->handle_);
    }
    if(handle&&id)webview_return(handle,id,0,"null");
}

void WebUi::push_event(int code,const std::string& value) {
    if(event_type_==static_cast<Uint32>(-1))return;
    auto* payload=new(std::nothrow) std::string(value);
    SDL_Event event{};event.type=event_type_;event.user.code=code;event.user.data1=payload;
    if(SDL_PushEvent(&event)<=0)delete payload;
}

void WebUi::sync(const Catalog& catalog,const Catalog& root_catalog,size_t selected,const std::string& status,const std::string& page) {
    auto serialize=[](const Catalog& source) {
        std::ostringstream json;
        json<<'[';
        const size_t count=std::min<size_t>(source.entries.size(),300);
        for(size_t i=0;i<count;++i) {
            if(i)json<<',';
            const auto& entry=source.entries[i];
            const char* kind_name="media";
            switch(entry.kind) {
                case EntryKind::Media: kind_name="media"; break;
                case EntryKind::Category: kind_name="category"; break;
                case EntryKind::Channel: kind_name="channel"; break;
                case EntryKind::Section: kind_name="section"; break;
                case EntryKind::VodCategory: kind_name="vod-category"; break;
                case EntryKind::Asset: kind_name="asset"; break;
                case EntryKind::Episode: kind_name="episode"; break;
                case EntryKind::SearchPage: kind_name="search-page"; break;
                case EntryKind::VodPage: kind_name="vod-page"; break;
                case EntryKind::LiveSection: kind_name="live-section"; break;
                case EntryKind::SearchAction: kind_name="search-action"; break;
                case EntryKind::Donation: kind_name="donation"; break;
                case EntryKind::Favorites: kind_name="favorites"; break;
                case EntryKind::Resume: kind_name="resume"; break;
            }
            json<<"{\"title\":"<<json_quote(entry.title)
                <<",\"artwork\":"<<json_quote(entry.artwork)
                <<",\"description\":"<<json_quote(entry.description)
                <<",\"kind\":"<<json_quote(kind_name)
                <<",\"program_type\":"<<json_quote(entry.program_type)
                <<'}';
        }
        json<<']';
        return json.str();
    };
    const std::string current_json=serialize(catalog);
    const std::string root_json=serialize(root_catalog);
    std::ostringstream js;
    js<<"if(window.xuperUI){window.xuperUI.renderCatalog("<<current_json<<','<<selected<<",{},"<<root_json<<");"
      <<"window.xuperUI.setPage("<<json_quote(page)<<",false);"
      <<"window.xuperUI.setStatus("<<json_quote(status)<<");}";
    const auto script=js.str();
    webview_t handle=nullptr;
    {std::lock_guard<std::mutex> lock(mutex_);handle=static_cast<webview_t>(handle_);}
    if(!handle)return;
    auto* task=new DispatchTask;
    task->work=[script](webview_t w){webview_eval(w,script.c_str());};
    if(webview_dispatch(handle,&dispatch_task,task)<0)delete task;
}
void WebUi::set_status(const std::string& status) {
    const std::string script="if(window.xuperUI)window.xuperUI.setStatus("+json_quote(status)+");";
    webview_t handle=nullptr;
    {std::lock_guard<std::mutex> lock(mutex_);handle=static_cast<webview_t>(handle_);}
    if(!handle)return;
    auto* task=new DispatchTask;
    task->work=[script](webview_t w){webview_eval(w,script.c_str());};
    if(webview_dispatch(handle,&dispatch_task,task)<0)delete task;
}

void WebUi::set_visible(bool visible) {
    webview_t handle=nullptr;
    {std::lock_guard<std::mutex> lock(mutex_);handle=static_cast<webview_t>(handle_);}
    if(!handle)return;
    auto* task=new DispatchTask;
    task->work=[visible](webview_t w){
        HWND window=static_cast<HWND>(webview_get_window(w));
        if(window){
            HWND parent=GetParent(window);
            if(parent&&visible){
                RECT bounds{};GetClientRect(parent,&bounds);
                SetWindowPos(window,nullptr,0,0,bounds.right-bounds.left,bounds.bottom-bounds.top,
                             SWP_NOZORDER|SWP_NOACTIVATE|SWP_SHOWWINDOW);
                SetForegroundWindow(parent);SetFocus(window);
            }else{
                ShowWindow(window,visible?SW_SHOW:SW_HIDE);
                if(parent&&!visible){SetForegroundWindow(parent);SetFocus(parent);}
            }
        }
    };
    auto future=task->completed.get_future();
    if(webview_dispatch(handle,&dispatch_task,task)<0){delete task;return;}
    future.wait_for(std::chrono::seconds(3));
}

void WebUi::resize() {
    webview_t handle=nullptr;
    {std::lock_guard<std::mutex> lock(mutex_);handle=static_cast<webview_t>(handle_);}
    if(!handle)return;
    auto* task=new DispatchTask;
    task->work=[](webview_t w){
        HWND window=static_cast<HWND>(webview_get_window(w));
        HWND parent=window?GetParent(window):nullptr;
        if(window&&parent){
            RECT bounds{};GetClientRect(parent,&bounds);
            SetWindowPos(window,nullptr,0,0,bounds.right-bounds.left,bounds.bottom-bounds.top,
                         SWP_NOZORDER|SWP_NOACTIVATE);
        }
    };
    if(webview_dispatch(handle,&dispatch_task,task)<0)delete task;
}

void WebUi::stop() {
    webview_t handle=nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_requested_=true;
        handle=static_cast<webview_t>(handle_);
    }
    if(handle)webview_terminate(handle);
    if(thread_.joinable())thread_.join();
    active_=false;
}

} // namespace xs

#endif
