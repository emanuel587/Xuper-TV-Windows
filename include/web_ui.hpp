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
    enum EventCode { Ready=1, Navigate, Play, Search, SelectItem, Back };

    WebUi() = default;
    ~WebUi();
    WebUi(const WebUi&) = delete;
    WebUi& operator=(const WebUi&) = delete;

    bool start(std::string& error);
    void stop();
    bool active() const { return active_.load(); }
    Uint32 event_type() const { return event_type_; }

    void sync(const Catalog& catalog, size_t selected,
              const std::string& status, const std::string& page);
    void set_status(const std::string& status);
    void set_visible(bool visible);

private:
    struct BindingContext { WebUi* owner=nullptr; int code=0; };
    static void binding_callback(const char* id,const char* request,void* arg);
    void push_event(int code,const std::string& value={});
    void thread_main();

    std::thread thread_;
    mutable std::mutex mutex_;
    std::condition_variable startup_cv_;
    void* handle_=nullptr;
    bool startup_done_=false;
    bool startup_ok_=false;
    bool stop_requested_=false;
    std::string startup_error_;
    Uint32 event_type_=static_cast<Uint32>(-1);
    std::atomic<bool> active_{false};
    BindingContext bindings_[6]{};
};

} // namespace xs

#endif
