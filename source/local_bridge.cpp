#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>
#include "local_bridge.hpp"
#include <atomic>
#include <thread>
#include <mutex>
#include <random>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <map>
#include <functional>

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"winhttp.lib")

namespace xs {
namespace {
std::string lower(std::string s){for(char& c:s)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));return s;}
bool send_all(SOCKET s,const char* p,size_t n){while(n){int k=send(s,p,static_cast<int>((std::min)(n,size_t(1<<20))),0);if(k<=0)return false;p+=k;n-=static_cast<size_t>(k);}return true;}
std::string random_token(){static std::mutex m;static std::mt19937_64 rng(std::random_device{}());std::lock_guard<std::mutex> l(m);static const char h[]="0123456789abcdef";std::string x(32,'0');for(char& c:x)c=h[rng()&15];return x;}
bool split_url(const std::string& u,std::wstring& host,std::wstring& path,bool& secure,unsigned short& port){
 const size_t p=u.find("://");if(p==std::string::npos)return false;const std::string scheme=lower(u.substr(0,p));secure=scheme=="https";if(!secure&&scheme!="http")return false;
 size_t a=p+3,b=u.find_first_of("/?#",a);std::string authority=u.substr(a,b==std::string::npos?b:b-a);if(authority.empty()||authority.find('@')!=std::string::npos)return false;
 port=secure?443:80;size_t colon=authority.rfind(':');if(colon!=std::string::npos&&authority.find(']')==std::string::npos){try{port=static_cast<unsigned short>(std::stoi(authority.substr(colon+1)));}catch(...){return false;}authority.resize(colon);}
 if(authority.empty())return false;std::string target=b==std::string::npos?"/":u.substr(b);if(target[0]=='?'||target[0]=='#')target="/"+target;
 auto widen=[](const std::string& s){int n=MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,nullptr,0);std::wstring w(n?static_cast<size_t>(n):0,L'\\0');if(n>1){MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,&w[0],n);w.resize(static_cast<size_t>(n-1));}return w;};
 host=widen(authority);path=widen(target);return !host.empty()&&!path.empty();
}
std::string resolve_ref(const std::string& base,const std::string& ref){
 if(ref.empty())return ref;if(ref.find("://")!=std::string::npos)return ref;
 std::string clean=base.substr(0,base.find_first_of("?#"));
 size_t scheme=clean.find("://"),slash=clean.find('/',scheme+3);std::string origin=slash==std::string::npos?clean:clean.substr(0,slash);
 if(ref.rfind("//",0)==0)return clean.substr(0,scheme+1)+ref;
 if(ref[0]=='/')return origin+ref;
 std::string dir=clean.substr(0,clean.find_last_of('/')+1);return dir+ref;
}
std::string rewrite_hls(const std::string& body,const std::string& base,const std::string& token,const std::function<std::string(const std::string&)>& add_url){
 std::istringstream in(body);std::string line,out;
 auto local=[&](const std::string& u){return "/"+token+"/"+add_url(u);};
 while(std::getline(in,line)){
  if(!line.empty()&&line.back()=='\r')line.pop_back();
  if(line.rfind("#",0)==0){
   size_t p=0;while((p=line.find("URI=\"",p))!=std::string::npos){size_t a=p+5,b=line.find('"',a);if(b==std::string::npos)break;std::string u=line.substr(a,b-a);line.replace(a,b-a,local(resolve_ref(base,u)));p=a+line.size();}
   out+=line+"\r\n";
  }else if(!line.empty())out+=local(resolve_ref(base,line))+"\r\n";else out+="\r\n";
 }
 return out;
}
}
struct LocalBridge::Impl {
 SOCKET listener=INVALID_SOCKET;std::thread accept_thread;std::atomic<bool> running{false};std::string token,source;std::vector<std::pair<std::string,std::string>> headers;std::mutex urls_mutex;std::map<std::string,std::string> urls;std::atomic<unsigned long long> next_url{1};std::mutex clients_mutex;std::vector<std::thread> clients;
 void serve(SOCKET s){
  std::string req;char buf[4096];while(req.find("\r\n\r\n")==std::string::npos&&req.size()<16384){int n=recv(s,buf,sizeof(buf),0);if(n<=0){closesocket(s);return;}req.append(buf,n);}
  std::istringstream in(req);std::string method,target,version;in>>method>>target>>version;
  if((method!="GET"&&method!="HEAD")||target.rfind("/"+token+"/",0)!=0){const char* r="HTTP/1.1 404 Not Found\r\nContent-Length:0\r\nConnection:close\r\n\r\n";send_all(s,r,strlen(r));closesocket(s);return;}
  std::string url=source;std::string key=target.substr(token.size()+2);if(!key.empty()){std::lock_guard<std::mutex> lock(urls_mutex);auto it=urls.find(key);if(it==urls.end()){closesocket(s);return;}url=it->second;}
  std::wstring host,path;bool secure=false;unsigned short port=0;if(!split_url(url,host,path,secure,port)){closesocket(s);return;}
  HINTERNET session=WinHttpOpen(L"XuperTV-LocalRelay/1.0",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
  HINTERNET conn=session?WinHttpConnect(session,host.c_str(),port,0):nullptr;
  HINTERNET request=conn?WinHttpOpenRequest(conn,method=="HEAD"?L"HEAD":L"GET",path.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,secure?WINHTTP_FLAG_SECURE:0):nullptr;
  std::wstring extra;
  for(const auto& h:headers){if(h.first.find_first_of("\\r\\n:")!=std::string::npos||h.second.find_first_of("\\r\\n")!=std::string::npos)continue;std::string line=h.first+": "+h.second+"\\r\\n";int n=MultiByteToWideChar(CP_UTF8,0,line.c_str(),-1,nullptr,0);if(n>1){std::wstring w(n,L'\\0');MultiByteToWideChar(CP_UTF8,0,line.c_str(),-1,&w[0],n);w.resize(static_cast<size_t>(n-1));extra+=w;}}
  std::string client_headers;std::istringstream hin(req);std::string hline;std::getline(hin,hline);while(std::getline(hin,hline)&&hline!="\\r"){if(!hline.empty()&&hline.back()=='\\r')hline.pop_back();auto c=hline.find(':');if(c==std::string::npos)continue;std::string name=lower(hline.substr(0,c));if(name=="range"||name=="if-range"||name=="if-none-match")client_headers+=hline+"\\r\\n";}
  if(request){if(!client_headers.empty()){int n=MultiByteToWideChar(CP_UTF8,0,client_headers.c_str(),-1,nullptr,0);std::wstring w(n?static_cast<size_t>(n):0,L'\\0');if(n>1){MultiByteToWideChar(CP_UTF8,0,client_headers.c_str(),-1,&w[0],n);w.resize(static_cast<size_t>(n-1));}extra+=w;}
   DWORD redirect_policy=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;WinHttpSetOption(request,WINHTTP_OPTION_REDIRECT_POLICY,&redirect_policy,sizeof(redirect_policy));
   BOOL ok=WinHttpSendRequest(request,extra.empty()?WINHTTP_NO_ADDITIONAL_HEADERS:extra.c_str(),extra.empty()?0:static_cast<DWORD>(extra.size()),WINHTTP_NO_REQUEST_DATA,0,0,0)&&WinHttpReceiveResponse(request,nullptr);
   if(ok){DWORD status=0,size=sizeof(status);WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&size,WINHTTP_NO_HEADER_INDEX);
    wchar_t ctype[256]{};size=sizeof(ctype);WinHttpQueryHeaders(request,WINHTTP_QUERY_CONTENT_TYPE,WINHTTP_HEADER_NAME_BY_INDEX,ctype,&size,WINHTTP_NO_HEADER_INDEX);
    wchar_t crange[256]{};size=sizeof(crange);WinHttpQueryHeaders(request,WINHTTP_QUERY_CONTENT_RANGE,WINHTTP_HEADER_NAME_BY_INDEX,crange,&size,WINHTTP_NO_HEADER_INDEX);
    wchar_t length[64]{};size=sizeof(length);WinHttpQueryHeaders(request,WINHTTP_QUERY_CONTENT_LENGTH,WINHTTP_HEADER_NAME_BY_INDEX,length,&size,WINHTTP_NO_HEADER_INDEX);
    auto narrow=[](const wchar_t* w){int n=WideCharToMultiByte(CP_UTF8,0,w,-1,nullptr,0,nullptr,nullptr);std::string out(n?static_cast<size_t>(n):0,'\\0');if(n>1){WideCharToMultiByte(CP_UTF8,0,w,-1,&out[0],n,nullptr,nullptr);out.resize(static_cast<size_t>(n-1));}return out;};
    std::string ct=narrow(ctype),cl=narrow(length),range=narrow(crange);bool playlist=lower(ct).find("mpegurl")!=std::string::npos||lower(url).find(".m3u8")!=std::string::npos;
    std::string response="HTTP/1.1 "+std::to_string(status)+" "+(status==206?"Partial Content":status==200?"OK":"Upstream Error")+"\r\nConnection: close\r\n";
    if(!ct.empty())response+="Content-Type: "+ct+"\r\n";if(!range.empty())response+="Content-Range: "+range+"\r\n";
    if(!playlist&&!cl.empty())response+="Content-Length: "+cl+"\r\n";
    if(playlist){std::string body;DWORD avail=0;while(WinHttpQueryDataAvailable(request,&avail)&&avail){std::vector<char> b(avail);DWORD got=0;if(!WinHttpReadData(request,b.data(),avail,&got)||!got)break;if(body.size()+got>8*1024*1024)break;body.append(b.data(),got);}std::string rewritten=rewrite_hls(body,url,token,[this](const std::string& u){std::string id=std::to_string(next_url.fetch_add(1));std::lock_guard<std::mutex> lock(urls_mutex);urls[id]=u;return id;});response+="Content-Type: application/vnd.apple.mpegurl\r\nContent-Length: "+std::to_string(rewritten.size())+"\r\n\r\n";send_all(s,response.data(),response.size());send_all(s,rewritten.data(),rewritten.size());}
    else{response+="\r\n";send_all(s,response.data(),response.size());if(method!="HEAD"){DWORD avail=0;while(WinHttpQueryDataAvailable(request,&avail)&&avail){std::vector<char>b((std::min)(avail,DWORD(65536)));DWORD got=0;if(!WinHttpReadData(request,b.data(),static_cast<DWORD>(b.size()),&got)||!got)break;if(!send_all(s,b.data(),got))break;}}}
   }else{const char* r="HTTP/1.1 502 Bad Gateway\r\nContent-Length:0\r\nConnection:close\r\n\r\n";send_all(s,r,strlen(r));}
  }
  if(request)WinHttpCloseHandle(request);if(conn)WinHttpCloseHandle(conn);if(session)WinHttpCloseHandle(session);closesocket(s);
 }
 void run(){while(running){SOCKET s=accept(listener,nullptr,nullptr);if(s==INVALID_SOCKET)break;std::lock_guard<std::mutex> l(clients_mutex);clients.emplace_back([this,s]{serve(s);});}}
};
LocalBridge::LocalBridge():impl_(new Impl){}
LocalBridge::~LocalBridge(){stop();delete impl_;}
bool LocalBridge::start(const std::string& source,const std::vector<std::pair<std::string,std::string>>& headers,std::string& local_url,std::string& error){
 stop();std::wstring h,p;bool tls=false;unsigned short port=0;if(!split_url(source,h,p,tls,port)){error="La fuente no es HTTP/HTTPS.";return false;}
 WSADATA wd{};if(WSAStartup(MAKEWORD(2,2),&wd)!=0){error="No se pudo iniciar Winsock.";return false;}
 impl_->listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(impl_->listener==INVALID_SOCKET){WSACleanup();error="No se pudo crear el puente local.";return false;}
 sockaddr_in addr{};addr.sin_family=AF_INET;addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);addr.sin_port=0;
 if(bind(impl_->listener,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))==SOCKET_ERROR||listen(impl_->listener,16)==SOCKET_ERROR){closesocket(impl_->listener);impl_->listener=INVALID_SOCKET;WSACleanup();error="No se pudo abrir el puerto local.";return false;}
 int len=sizeof(addr);getsockname(impl_->listener,reinterpret_cast<sockaddr*>(&addr),&len);
 impl_->source=source;impl_->headers=headers;impl_->token=random_token();impl_->urls.clear();impl_->next_url=1;impl_->running=true;impl_->accept_thread=std::thread([this]{impl_->run();});
 local_url="http://127.0.0.1:"+std::to_string(ntohs(addr.sin_port))+"/"+impl_->token+"/";
 return true;
}
void LocalBridge::stop(){if(!impl_||!impl_->running.exchange(false))return;if(impl_->listener!=INVALID_SOCKET){shutdown(impl_->listener,SD_BOTH);closesocket(impl_->listener);impl_->listener=INVALID_SOCKET;}if(impl_->accept_thread.joinable())impl_->accept_thread.join();{std::lock_guard<std::mutex> l(impl_->clients_mutex);for(auto& t:impl_->clients)if(t.joinable())t.join();impl_->clients.clear();}impl_->headers.clear();impl_->source.clear();impl_->token.clear();{std::lock_guard<std::mutex> l(impl_->urls_mutex);impl_->urls.clear();}WSACleanup();}
}
#endif
