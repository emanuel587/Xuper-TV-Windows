#include "catalog.hpp"
#include <cassert>
#include <sstream>
#include <iostream>
int main() {
    std::istringstream in("\xEF\xBB\xBF#EXTM3U\r\n#EXTINF:-1 group-title=\"News, HD\",Canal uno\r\nhttps://example.org/a.m3u8?token=local-test\r\n#EXTINF:-1,Duplicado\nhttps://example.org/a.m3u8?token=local-test\n#EXTVLCOPT:http-user-agent=custom\nhttps://example.org/b\n#EXTINF:-1,Mal\njavascript:alert(1)\nsdmc:/switch/test.mp4\n");
    auto c=xs::read_m3u(in);
    assert(c.entries.size()==3);assert(c.warnings.size()==2);
    assert(c.entries[0].title=="Canal uno");assert(c.entries[1].title=="Duplicado");
    assert(c.entries[2].source=="sdmc:/switch/test.mp4");
    assert(xs::valid_source("https://example.org/video?q=a%20b"));
    for(const char* s:{"https://", "http:///x", "https://user:pass@example.org/x", "file:///etc/passwd", "https://example.org/a\nb", "https://example.org/\"x", "https://example.org/a b", "sdmc:/a/../b"}) assert(!xs::valid_source(s));
    std::istringstream opts("#EXTINF:-1,Header\nhttps://example.org/a|User-Agent=test\n#EXTINF:-1,OK\nhttps://example.org/b\n");
    auto c2=xs::read_m3u(opts);assert(c2.entries.size()==1);assert(c2.warnings.size()==1);
    std::istringstream hls("#EXTM3U\n#EXT-X-STREAM-INF:BANDWIDTH=123\nvariant/playlist.m3u8\n");
    assert(xs::read_m3u(hls).entries.empty());
    std::cout<<"catalog: all checks passed\n";
}
