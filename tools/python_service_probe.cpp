#include "service.hpp"
#include "json.hpp"
#include <iostream>
#include <string>
#include <vector>

#define JSON_NOEXCEPTION 1
using Json=nlohmann::json;
static std::shared_ptr<xs::ServiceSession> session;
static xs::Catalog current;

static const char* kind_name(xs::EntryKind k) {
    switch(k) {
        case xs::EntryKind::Category:return "category";
        case xs::EntryKind::Channel:return "channel";
        case xs::EntryKind::Section:return "section";
        case xs::EntryKind::LiveSection:return "live";
        default:return "other";
    }
}
static Json catalog_json(const xs::Catalog& c) {
    Json items=Json::array();
    for(size_t i=0;i<c.entries.size();++i) {
        const auto& e=c.entries[i];
        items.push_back({{"index",i},{"title",e.title},{"kind",kind_name(e.kind)},
                         {"column_id",e.column_id},{"channel_code",e.channel_code},
                         {"content_id",e.content_id}});
    }
    return {{"ok",true},{"title",c.title},{"items",items},{"warnings",c.warnings}};
}
static void emit(const Json& j) { std::cout<<j.dump()<<std::endl; }
int main(int argc,char** argv) {
    if(argc!=2) return 2;
    std::string line;
    while(std::getline(std::cin,line)) {
        Json req=Json::parse(line,nullptr,false);
        if(!req.is_object()){emit({{"ok",false},{"status","Solicitud local no valida."}});continue;}
        const std::string cmd=req.value("cmd",std::string());
        if(cmd=="bootstrap") {
            auto result=xs::service_bootstrap(argv[1]);
            session=result.session;current=std::move(result.catalog);
            if(!result.authenticated){emit({{"ok",false},{"status",result.status}});continue;}
            Json out=catalog_json(current);out["status"]=result.status;emit(out);
        } else if(cmd=="root") {
            if(!session){emit({{"ok",false},{"status","Primero conecta el servicio."}});continue;}
            auto result=xs::service_root(session);if(result.ok)current=std::move(result.catalog);
            Json out=result.ok?catalog_json(current):Json{{"ok",false}};
            out["status"]=result.status;emit(out);
        } else if(cmd=="channels") {
            if(!session){emit({{"ok",false},{"status","Primero conecta el servicio."}});continue;}
            int id=req.value("column_id",-1);
            auto result=xs::service_channels(session,id);
            if(result.ok)current=std::move(result.catalog);
            Json out=result.ok?catalog_json(current):Json{{"ok",false}};
            out["status"]=result.status;emit(out);
        } else if(cmd=="browse") {
            if(!session){emit({{"ok",false},{"status","Primero conecta el servicio."}});continue;}
            size_t index=req.value("index",static_cast<size_t>(-1));
            if(index>=current.entries.size()){emit({{"ok",false},{"status","Seleccion no valida."}});continue;}
            auto result=xs::service_browse(session,current.entries[index]);
            if(result.ok)current=std::move(result.catalog);
            Json out=result.ok?catalog_json(current):Json{{"ok",false}};
            out["status"]=result.status;emit(out);
        } else if(cmd=="source") {
            if(!session){emit({{"ok",false},{"status","Primero conecta el servicio."}});continue;}
            size_t index=req.value("index",static_cast<size_t>(-1));
            if(index>=current.entries.size()||current.entries[index].kind!=xs::EntryKind::Channel){
                emit({{"ok",false},{"status","Selecciona un canal de TV."}});continue;
            }
            auto result=xs::service_source(session,current.entries[index],req.value("variant",0));
            Json headers=Json::array();
            for(const auto& h:result.headers)headers.push_back({{"name",h.first},{"value",h.second}});
            emit({{"ok",result.ok},{"status",result.status},{"source",result.source},{"headers",headers}});
        } else if(cmd=="quit") { emit({{"ok",true}});break; }
        else emit({{"ok",false},{"status","Comando desconocido."}});
    }
    return 0;
}