#include "ranger_codec.hpp"
#include "json.hpp"
#include <mbedtls/base64.h>
#include <fstream>
#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>
using xs::RangerMode;
static std::string tamper(const std::string& wire){
 const std::string custom="jWB7YtC3n9iXbEkUcJl1VxF4STpQoOIaRmh2M-efAgLwPqGr6uyD5vNsdH_Kz0Z8";
 const std::string standard="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
 std::string encoded;for(char c:wire)encoded+=standard[custom.find(c)];while(encoded.size()%4)encoded+='=';
 std::vector<unsigned char> data(encoded.size());size_t count=0;
 assert(!mbedtls_base64_decode(data.data(),data.size(),&count,reinterpret_cast<const unsigned char*>(encoded.data()),encoded.size()));
 assert(count==32);data[15]^=16; // A full padding block's final byte becomes zero.
 std::vector<unsigned char> output(128);size_t length=0;
 assert(!mbedtls_base64_encode(output.data(),output.size(),&length,data.data(),count));
 std::string result;for(size_t i=0;i<length;++i)if(output[i]!='=')result+=custom[standard.find(static_cast<char>(output[i]))];return result;
}
int main(int argc,char** argv){
 std::ifstream file(argc>1?argv[1]:"tests/fixtures/ranger_arm64_vectors.json");
 const std::string bytes((std::istreambuf_iterator<char>(file)),{});
 auto vectors=nlohmann::json::parse(bytes);assert(vectors.is_array()&&vectors.size()==16);
 for(const auto& v:vectors){
  const auto mode=v["kind"]=="live"?RangerMode::Live:RangerMode::Vod;
  const bool response=v["resource"].get<std::string>().rfind("resp:",0)==0;
  const auto seed=v["seed_input"].get<std::string>(),plain=v["plain"].get<std::string>();
  assert(xs::ranger_key_hex(seed,mode)==v["base_key"]);
  assert(xs::ranger_iv_hex(v["base_key"],v["resource"])==v["iv"]);
  std::string wire,decoded;assert(xs::ranger_encode(seed,mode,plain,response,wire));assert(wire==v["encoded"]);
  assert(xs::ranger_decode(seed,mode,wire,response,decoded));assert(decoded==plain);
  const size_t pad=(4-wire.size()%4)%4;
  const std::string padded=wire+std::string(pad,'=');
  assert(xs::ranger_decode(seed,mode,padded,response,decoded)&&decoded==plain);
  if(pad)assert(xs::ranger_decode(seed,mode,padded+"\r\n",response,decoded)&&decoded==plain);
  assert(!xs::ranger_decode(seed,mode,wire+"===",response,decoded));
 }
 std::ifstream id_file("tests/fixtures/ranger_id_arm64_vectors.json");
 const auto ids=nlohmann::json::parse(id_file);assert(ids.size()==3);
 for(const auto& v:ids)assert(xs::ranger_request_id(v["user"].get<uint32_t>(),v["device"].get<uint64_t>(),
     v["seconds"].get<uint32_t>(),v["nonce"].get<uint32_t>())==v["value"]);
 std::ifstream live_file("tests/fixtures/live_auth_arm64_vectors.json");
 const auto live_vectors=nlohmann::json::parse(live_file);assert(live_vectors.size()==4);
 for(const auto& v:live_vectors)assert(xs::ranger_live_content_auth(v["resource"],v["instance"].get<int32_t>(),v["moment_ms"].get<uint64_t>())==v["content_auth"]);
 assert(xs::ranger_live_content_auth("/live?token=",0,123)=="/live?token=");
 assert(xs::ranger_live_content_auth("/bad\r\nInjected: 1",0,0).empty());
 std::ifstream path_file("tests/fixtures/live_path_arm64_vectors.json");const auto paths=nlohmann::json::parse(path_file);assert(paths.size()==4);
 for(const auto& v:paths)assert(xs::ranger_slb_live_path(v["code"],v["format"])==v["path"]);
 for(const auto& invalid:{"","../bad","bad?token=x","bad#x","bad\\path"})assert(xs::ranger_slb_live_path(invalid,"ts").empty());
 for(const auto mode:{RangerMode::Live,RangerMode::Vod})for(const size_t n:{0u,1u,15u,16u,17u,31u,32u,255u,4096u,65519u,65535u}){
  std::string plain(n,'\0');for(size_t i=0;i<n;++i)plain[i]=static_cast<char>(i*37);
  std::string wire,decoded;assert(xs::ranger_encode("synthetic-seed",mode,plain,false,wire));
  assert(xs::ranger_decode("synthetic-seed",mode,wire,false,decoded));assert(decoded==plain);
 }
 std::string wire,decoded="must be cleared";
 assert(!xs::ranger_encode("seed",RangerMode::Live,std::string(65536,'x'),false,wire));assert(wire.empty());
 assert(!xs::ranger_encode(std::string(4097,'k'),RangerMode::Live,"test",false,wire));
 for(const auto& malformed:{"", "a", "aaaa=", "aaaa\n", "!!!!", "aaaa "}){
  assert(!xs::ranger_decode("seed",RangerMode::Live,malformed,true,decoded));assert(decoded.empty());
 }
 assert(!xs::ranger_decode("seed",RangerMode::Live,std::string(90000,'a'),true,decoded));
 assert(xs::ranger_encode("synthetic-seed",RangerMode::Live,"1234567890123456",false,wire));
 assert(!xs::ranger_decode("synthetic-seed",RangerMode::Live,tamper(wire),false,decoded));assert(decoded.empty());
 assert(xs::ranger_key_hex("test-key",RangerMode::Live)!=xs::ranger_key_hex("test-key",RangerMode::Vod));
 std::cout<<"ranger: 16 transport + 3 ID + 4 Live auth + 4 Live path original ARM64 vectors, limits, binary payloads and malformed padding passed\n";
}
