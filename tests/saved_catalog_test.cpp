#include "saved_catalog.hpp"
#include <filesystem>
#include <cassert>
#include <chrono>
#include <iostream>
int main(){
    const auto dir=std::filesystem::temp_directory_path()/("xs-saved-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directory(dir);const auto path=(dir/"state.json").string();
    xs::SavedCatalog saved(path);xs::Entry channel;channel.kind=xs::EntryKind::Channel;channel.title="Noticias";channel.channel_code="c1";channel.column_id=20;channel.source="https://secret.invalid/video?token=private";channel.artwork="https://image.invalid/a?secret=test";channel.artwork_cache=xs::artwork_key(channel.artwork);
    assert(saved.toggle(channel)&&saved.contains(channel));assert(xs::SavedCatalog(path).favorites.size()==1);
    std::ifstream file(path);const std::string data((std::istreambuf_iterator<char>(file)),{});assert(data.find("secret")==std::string::npos&&data.find("token")==std::string::npos&&data.find("http")==std::string::npos);file.close();
    assert(saved.toggle(channel)&&!saved.contains(channel));assert(xs::SavedCatalog(path).favorites.empty());
    assert(saved.toggle(channel));std::filesystem::rename(path,path+".bak");xs::SavedCatalog restored(path);assert(restored.contains(channel));assert(restored.save());
    xs::Entry episode;episode.kind=xs::EntryKind::Episode;episode.title="Episodio 1";episode.content_id="e1";episode.series_id="s1";episode.program_type="0";assert(restored.remember(episode)&&restored.remember(episode));assert(xs::SavedCatalog(path).recent.size()==1);
    std::filesystem::create_directory(path+".tmp");assert(!restored.toggle(channel));assert(restored.contains(channel)&&xs::SavedCatalog(path).contains(channel));
    assert(xs::search_key("PrUeBa Ñ") == xs::search_key("prueba n"));assert(xs::search_key("Café HD")=="cafehd");
    std::filesystem::remove_all(dir);std::cout<<"favorites: persistence, remove, backup recovery, failed-write rollback, privacy, recents dedup and accent matching passed\n";
}
