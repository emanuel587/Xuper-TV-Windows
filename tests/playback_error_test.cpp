#include "playback_error.hpp"
#include <cassert>
#include <iostream>
int main(){
    assert(xs::playback_error_hint("[ffmpeg] HTTP error 403 Forbidden https://server.invalid/?token=private")=="HTTP 403");
    assert(xs::playback_error_hint("HTTP/1.1 404 Not Found")=="HTTP 404");
    assert(xs::playback_error_hint("server returned 503 unavailable")=="HTTP 503");
    assert(xs::playback_error_hint("Certificate verify failed for token=private")=="Verificacion TLS");
    assert(xs::playback_error_hint("Invalid data found when processing input")=="Formato de video no reconocido");
    assert(xs::playback_error_hint("https://server.invalid/file-403?token=private").empty());
    assert(xs::playback_error_hint("headers: Content-Auth: private").empty());
    std::cout<<"playback error: fixed labels, HTTP codes and secret-bearing messages passed\n";
}
