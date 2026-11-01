#include "installation.hpp"
#include <filesystem>
#include <cassert>
#include <chrono>
#include <iostream>
int main(){
    const auto root=std::filesystem::temp_directory_path()/("xs-install-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directory(root);
    xs::InstallationJson base={{"portal_hosts",{"https://example.invalid"}},{"key_base64","synthetic"},{"password_salt","synthetic"},{"app_id","test.app"}};
    int calls=0;auto random=[&](unsigned char* p,size_t n){++calls;for(size_t i=0;i<n;++i)p[i]=static_cast<unsigned char>(calls+i);return true;};
    xs::InstallationJson a,b;std::string error;
    for(const auto* name:{"one","two","state","invalid"}){std::filesystem::create_directory(root/name);std::ofstream(root/name/"service.template.json")<<base.dump();}
    auto one=(root/"one/service.json").string(),two=(root/"two/service.json").string();
    assert(xs::load_installation(one,a,error,random));assert(a["installation_serial"].get<std::string>().size()==32);assert(a["device_id"].get<std::string>().size()==17);
    assert(xs::load_installation(one,b,error,random)&&a==b&&calls==1);
    assert(xs::load_installation(two,b,error,random)&&a["installation_serial"]!=b["installation_serial"]&&a["device_id"]!=b["device_id"]);
    std::ofstream(root/"state/device_state.json")<<"{}";assert(!xs::load_installation((root/"state/service.json").string(),b,error,random));assert(calls==2);
    std::ofstream(root/"invalid/service.json")<<"broken";assert(!xs::load_installation((root/"invalid/service.json").string(),b,error,random));assert(calls==2);
    base["installation_serial"]="private";std::ofstream(root/"two/service.template.json")<<base.dump();std::filesystem::remove(two);assert(!xs::load_installation(two,b,error,random));assert(calls==2);
    std::filesystem::remove_all(root);std::cout<<"first-run identity, persistence, independent installs, existing config and activation protection passed\n";
}
