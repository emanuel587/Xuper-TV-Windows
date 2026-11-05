#pragma once
#include <string>
#include <algorithm>
#include <cctype>
namespace xs {
// Only fixed labels or three HTTP status digits escape this classifier.
// libmpv/FFmpeg messages may contain signed URLs and must never be logged verbatim.
inline std::string playback_error_hint(std::string message){
    std::transform(message.begin(),message.end(),message.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
    for(const char* prefix:{"http error ","http/1.1 ","http/1.0 ","http/2 ","server returned "}){
        size_t start=0;while((start=message.find(prefix,start))!=std::string::npos){
            const size_t code=start+std::char_traits<char>::length(prefix);start=code;
            if(code+3<=message.size()&&message[code]>='4'&&message[code]<='5'&&message[code+1]>='0'&&message[code+1]<='9'&&message[code+2]>='0'&&message[code+2]<='9')return "HTTP "+message.substr(code,3);
        }
    }
    if(message.find("certificate")!=std::string::npos||message.find("tls handshake")!=std::string::npos)return "Verificacion TLS";
    if(message.find("timed out")!=std::string::npos||message.find("timeout")!=std::string::npos)return "Tiempo de conexion agotado";
    if(message.find("connection refused")!=std::string::npos)return "Conexion rechazada";
    if(message.find("protocol not found")!=std::string::npos)return "Protocolo no compatible";
    if(message.find("decoder")!=std::string::npos&&message.find("not found")!=std::string::npos)return "Decodificador no disponible";
    if(message.find("invalid data found")!=std::string::npos||message.find("failed to recognize file format")!=std::string::npos)return "Formato de video no reconocido";
    return {};
}
}
