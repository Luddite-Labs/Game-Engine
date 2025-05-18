#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_init.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3/SDL_assert.h>
#include <clay.h>

#include <glm/common.hpp>

#include <cmath>
#include <string_view>
#include <filesystem>

#include <engine/engine.hpp>
#include <renderer/renderer.hpp>
#include <physics/physics-engine.hpp>
#include <audio/audio-engine.hpp>
#include <misc/log.hpp>

// Engine wraps Game and shows UI to edit state of Game 
// On Game build only Game remains
// Nodes, Memory etc. with the flow

struct AppContext {
    SDL_Window* window;
    Renderer* renderer;
    AudioEngine* audio_engine;
    PhysicsEngine* physics_engine;
    Engine* engine;
    SDL_AppResult app_status = SDL_APP_CONTINUE;
};

SDL_AppResult SDL_Fail() {
    LOG_ERROR("Error %s", SDL_GetError());
    return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppInit(void** app_state, int argc, char* argv[]) {
    if (not SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)){
        return SDL_Fail();
    }
    
    if (not TTF_Init()) {
        return SDL_Fail();
    }
    
    SDL_Window* window = SDL_CreateWindow("Window", 352, 430, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (not window) {
        return SDL_Fail();
    }
    
    // print some information about the window
    SDL_ShowWindow(window);
    {
        int width, height, bb_width, bb_height;
        SDL_GetWindowSize(window, &width, &height);
        SDL_GetWindowSizeInPixels(window, &bb_width, &bb_height);
        LOG_INFO("Window size: %ix%i", width, height);
        LOG_INFO("Backbuffer size: %ix%i", bb_width, bb_height);
        if (width != bb_width){
            LOG_INFO("This is a highdpi environment.");
        }
    }

    Renderer* renderer = new Renderer(window);
    AudioEngine* audio_engine = new AudioEngine();
    PhysicsEngine* physics_engine = new PhysicsEngine();
    Engine* engine = new Engine();
    // set up the application data
    *app_state = new AppContext{
        .window = window,
        .renderer = renderer,
        .audio_engine = audio_engine,
        .physics_engine = physics_engine,
        .engine = engine,
    };
    
    LOG_INFO("Application started successfully!");

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* app_state, SDL_Event* event) {
    auto* app = (AppContext*)app_state;
    
    if (event->type == SDL_EVENT_QUIT) {
        app->app_status = SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* app_state) {
    auto* app = (AppContext*)app_state;
    return app->app_status;
}

void SDL_AppQuit(void* app_state, SDL_AppResult result) {
    auto* app = (AppContext*)app_state;
    if (app) {
        delete app->renderer;
        delete app->audio_engine;
        delete app->physics_engine;
        delete app->engine;
        SDL_DestroyWindow(app->window);
        delete app;
    }
    TTF_Quit();

    LOG_INFO("Application quit successfully!");
    SDL_Quit();
}
