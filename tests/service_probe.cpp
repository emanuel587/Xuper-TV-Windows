#include "service.hpp"
#include <iostream>
#include <string>
int main(int argc,char** argv) {
    if(argc!=3)return 2;
    const std::string mode=argv[2];
    auto result=mode.rfind("bootstrap",0)==0?xs::service_bootstrap(argv[1]):xs::service_login(argv[1],"demo@example.invalid","test-password");
    if(mode=="bootstrap_http_forbidden")return !result.authenticated?0:1;
    if(mode.rfind("bootstrap_ranger_",0)==0){
        if(mode=="bootstrap_ranger_region")return !result.authenticated&&result.status.find("Restriccion regional")!=std::string::npos?0:1;
        if(!result.authenticated||!result.session)return 1;
        xs::Entry entry;entry.kind=(mode=="bootstrap_ranger_vod"||mode=="bootstrap_ranger_vod_quality")?xs::EntryKind::Episode:xs::EntryKind::Channel;
        entry.channel_code="channel-test";entry.content_id="movie-video";
        auto source=xs::service_source(result.session,entry,mode=="bootstrap_ranger_vod_quality"?1:0);
        if(mode=="bootstrap_ranger_vod_quality"){
            if(source.variants.size()!=2||source.variants[1].find("1080p")==std::string::npos||source.subtitles.size()!=1)return 1;
            std::string data,error;if(!xs::service_subtitle(result.session,source.subtitles[0],data,error)||data.find("Texto de prueba")==std::string::npos)return 1;
            auto insecure=source.subtitles[0];insecure.url.replace(0,5,"http");
            if(xs::service_subtitle(result.session,insecure,data,error)||!data.empty())return 1;
            auto bad=source.subtitles[0];bad.md5="00000000000000000000000000000000";
            if(xs::service_subtitle(result.session,bad,data,error)||!data.empty()||error.find("huella")==std::string::npos)return 1;
        }
        if(mode=="bootstrap_ranger_http"||mode=="bootstrap_ranger_live"||mode=="bootstrap_ranger_live_mp4"||mode=="bootstrap_ranger_live_signed"||mode=="bootstrap_ranger_padded"||mode=="bootstrap_ranger_gzip"||mode=="bootstrap_ranger_vod"||mode=="bootstrap_ranger_vod_quality"){
            const bool vod=mode=="bootstrap_ranger_vod"||mode=="bootstrap_ranger_vod_quality";
            const std::string expected=vod?"https://media.example.invalid/watch.m3u8?sig=a%2Fb&expires=123":
                mode=="bootstrap_ranger_live_signed"?"https://media.example.invalid/live/source-live.ts?tag=alpha&token=a%2Fb%3D%3D&expires=1793550000":
                mode=="bootstrap_ranger_live_mp4"?"https://media.example.invalid/live/source-live.mp4?sig=a%2Fb&expires=123":
                "https://media.example.invalid/live/source-live.ts?sig=a%2Fb&expires=123";
            if(!source.ok||source.source!=expected)return 1;
            if(mode=="bootstrap_ranger_live_signed")for(const auto& h:source.headers)if(h.first=="Content-Auth")std::cout<<h.second;
            if(mode=="bootstrap_ranger_live"){
                bool auth=false,app=false,version=false,pragma=false;
                for(const auto& h:source.headers){if(h.first=="Content-Auth")auth=h.second=="/watch.m3u8?sig=a%2Fb&expires=123";
                    if(h.first=="App")app=h.second=="test.app";
                    if(h.first=="App-Version")version=h.second=="43404";
                    if(h.first=="Pragma")pragma=h.second=="akamai-x-cache-on";}
                if(!auth||!app||!version||!pragma)return 1;
            }
            return source.headers.size()==((mode=="bootstrap_ranger_vod"||mode=="bootstrap_ranger_vod_quality")?0u:6u)?0:1;
        }
        if(mode=="bootstrap_ranger_cdn_bad_request")return !source.ok&&source.source.empty()&&source.status.find("HTTP 400")!=std::string::npos?0:1;
        return !source.ok&&source.source.empty()?0:1;
    }
    if(mode=="bootstrap_vod"){
        if(!result.authenticated||!result.session||result.catalog.entries.size()!=3)return 1;
        auto movies=xs::service_browse(result.session,result.catalog.entries[0]);
        if(!movies.ok||movies.catalog.entries.size()!=1)return 1;
        auto films=xs::service_browse(result.session,movies.catalog.entries[0]);
        if(!films.ok||films.catalog.entries.size()!=2||films.catalog.entries[0].artwork!="https://images.example.invalid/movie.jpg"||films.catalog.entries[0].description!="Ficha sintetica")return 1;
        auto detail=xs::service_browse(result.session,films.catalog.entries[0]);
        if(!detail.ok||detail.catalog.entries.size()!=1||detail.catalog.entries[0].kind!=xs::EntryKind::Episode)return 1;
        auto source=xs::service_source(result.session,detail.catalog.entries[0]);
        if(source.ok||source.status.find("Ranger")==std::string::npos)return 1;
        auto page2=xs::service_browse(result.session,films.catalog.entries[1]);
        if(!page2.ok||page2.catalog.entries.size()!=1)return 1;
        auto shows=xs::service_browse(result.session,result.catalog.entries[1]);
        if(!shows.ok||shows.catalog.entries.size()!=1)return 1;
        auto episodes=xs::service_browse(result.session,shows.catalog.entries[0]);
        if(!episodes.ok||episodes.catalog.entries.size()!=2||episodes.catalog.entries[0].series_id!="series-test")return 1;
        auto episode_source=xs::service_source(result.session,episodes.catalog.entries[0]);
        if(episode_source.ok||episode_source.status.find("rechazo permission-denied")==std::string::npos)return 1;
        auto search=xs::service_search(result.session,"Prueba ñ");
        if(!search.ok||search.catalog.entries.size()!=3||search.catalog.entries.back().kind!=xs::EntryKind::SearchPage)return 1;
        auto more=xs::service_browse(result.session,search.catalog.entries.back());
        if(!more.ok||more.catalog.entries.size()!=1)return 1;
        auto empty=xs::service_search(result.session,"");
        if(empty.ok)return 1;
        size_t progress=0;auto unified=xs::service_search_all(result.session,"Prueba ñ",[&](size_t done,size_t total){progress=total;return done<=total;});
        if(!unified.ok||unified.catalog.entries.size()!=4||progress!=1||unified.catalog.entries.front().kind!=xs::EntryKind::Channel)return 1;
        if(unified.catalog.entries.front().artwork!="https://images.example.invalid/channel.png")return 1;
        // Cached channel catalogue serves repeated browsing without another request.
        return xs::service_channels(result.session,200).catalog.entries.size()==1?0:1;
    }
    if(mode.rfind("bootstrap_catalog",0)==0) {
        if(!result.authenticated||!result.session)return 1;
        if(mode=="bootstrap_catalog_missing")return result.catalog.entries.empty()&&result.status.find("seccion TV")!=std::string::npos?0:1;
        if(result.catalog.entries.size()!=1||result.catalog.warnings.size()!=2)return 1;
        const auto category=result.catalog.entries.front();
        if(category.kind!=xs::EntryKind::Category||category.column_id!=200)return 1;
        auto channels=xs::service_channels(result.session,category.column_id);
        if(!channels.ok||channels.catalog.entries.size()!=1||channels.catalog.warnings.size()!=1)return 1;
        const auto channel=channels.catalog.entries.front();
        if(channel.kind!=xs::EntryKind::Channel||channel.channel_code!="channel-test")return 1;
        auto source=xs::service_source(result.session,channel);
        if(mode=="bootstrap_catalog_denied")return !source.ok&&source.source.empty()&&source.status.find("rechazo permission-denied")!=std::string::npos?0:1;
        if(mode=="bootstrap_catalog_opaque"||mode=="bootstrap_catalog_license")return !source.ok&&source.source.empty()&&source.status.find("Ranger")!=std::string::npos?0:1;
        if(!source.ok||source.source!="https://media.example.invalid/live.m3u8?sig=a%2Fb&expires=123")return 1;
        auto again=xs::service_root(result.session);
        return again.ok&&again.catalog.entries.size()==1?0:1;
    }
    if(mode=="bootstrap_success"||mode=="bootstrap_existing")return result.authenticated&&result.catalog.entries.size()==1?0:1;
    if(mode=="bootstrap_version")return !result.authenticated&&result.status.find("version fuera")!=std::string::npos?0:1;
    if(mode=="bootstrap_denied")return !result.authenticated&&result.status.find("rechazo")!=std::string::npos?0:1;
    if(mode=="bootstrap_incomplete")return !result.authenticated&&result.status.find("sesion completa")!=std::string::npos?0:1;
    if(mode=="success")return result.authenticated&&result.catalog.entries.size()==1?0:1;
    if(mode=="denied")return !result.authenticated&&result.status.find("Acceso rechazado")!=std::string::npos?0:1;
    if(mode=="malformed")return !result.authenticated&&result.status.find("decodificar")!=std::string::npos?0:1;
    return 2;
}
