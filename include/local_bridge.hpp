#pragma once
#ifdef _WIN32
#include <string>
#include <vector>
#include <utility>
namespace xs {
// Loopback-only HTTP relay for an already-authorized live source.
// Credentials and upstream headers remain in memory and are never put in the URL.
class LocalBridge {
public:
    LocalBridge();
    ~LocalBridge();
    LocalBridge(const LocalBridge&)=delete;
    LocalBridge& operator=(const LocalBridge&)=delete;
    bool start(const std::string& source,
               const std::vector<std::pair<std::string,std::string>>& headers,
               std::string& local_url,std::string& error);
    void stop();
private:
    struct Impl;
    Impl* impl_;
};
}
#endif
