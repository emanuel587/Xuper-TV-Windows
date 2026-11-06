#pragma once

#ifdef _WIN32

#include "catalog.hpp"
#include <SDL.h>
#include <atomic>
#include <cstddef>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

namespace xs {

class WebUi {
public:
    enum EventCode { Ready=1, Navigate, Play, Search, SelectItem, Back, ToggleFavorite, Quit, ToggleFullscreen, ExternalPlay };

    WebUi() = default;
    ~WebUi();
    WebUi(const WebUi&) = delete;
    WebUi& operator=(const WebUi&) = delete;

    bool start(std::string& error, SDL_Window* parent = nullptr);
    void stop();
    bool active() const { return active_.load(); }
    Uint32 event_type() const { return event_type_; }

    void sync(const Catalog& catalog, const Catalog& root_catalog, size_t selected,
              const std::string& status, const std::string& page);
    void set_status(const std::string& status);
    void set_visible(bool visible);
    void resize();
    // Forget the last payload so the next sync() is always delivered (main thread only).
    void invalidate() { last_sync_.clear(); }

private:
    struct BindingContext { WebUi* owner=nullptr; int code=0; };
    static void binding_callback(const char* id,const char* request,void* arg);
    void push_event(int code,const std::string& value={});
    void thread_main();
    void join_thread();

    std::thread thread_;
    std::atomic<bool> thread_finished_{false}; // set when thread_main() has fully returned
    mutable std::mutex mutex_;
    std::condition_variable startup_cv_;
    void* handle_=nullptr;
    void* parent_window_=nullptr;
    void* child_window_=nullptr;          // HWND of the WebView child (set once at startup)
    unsigned long webview_thread_id_=0;   // Win32 thread id running webview_run
    std::atomic<bool> ready_{false};      // page finished loading and called xuperReady
    std::string last_sync_;               // last payload sent; main thread only
    bool startup_done_=false;
    bool startup_ok_=false;
    bool stop_requested_=false;
    std::string startup_error_;
    Uint32 event_type_=static_cast<Uint32>(-1);
    std::atomic<bool> active_{false};
    BindingContext bindings_[10]{};
};

} // namespace xs

#endif
