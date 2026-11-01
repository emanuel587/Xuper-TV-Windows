#pragma once
#include "catalog.hpp"
#include <SDL.h>
#include <memory>
namespace xs {
class Artwork {
    struct Impl;std::unique_ptr<Impl> impl;
public:
    explicit Artwork(SDL_Renderer* renderer);
    ~Artwork();
    SDL_Texture* get(const std::string& url,const std::string& key);
    bool poll();
};
}
