#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

#include <cstdint>

class EngineUI {
private:
    uint64_t m_mem_arena_size;
    char* m_mem_arena;

public:
    EngineUI();
    ~EngineUI();
    SDL_AppResult processEvent(SDL_Event* event);
};