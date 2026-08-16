#include <SDL2/SDL.h>
#include <cairo.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include "ff_env.h"

// The game is simulated and drawn at a fixed internal resolution, then
// scaled as a whole onto the actual window. This is what keeps a 16:9
// layout regardless of window/monitor shape: the fish, pipes, and scenery
// are always sized relative to this exact canvas, never recomputed against
// the real window dimensions, so resizing (or toggling fullscreen) just
// makes the same picture bigger or smaller instead of re-laying anything out.
static const int GAME_W = 1920;
static const int GAME_H = 1080;

static const int INIT_WIN_W = 480;
static const int INIT_WIN_H = 270; // 16:9, matches GAME_W/GAME_H

// Largest centered GAME_W x GAME_H rect (scaled uniformly) that fits inside
// a win_w x win_h window, i.e. classic letterbox/pillarbox fit.
static SDL_Rect compute_dest_rect(int win_w, int win_h) {
    double scale = (double)win_w / GAME_W;
    double scale_h = (double)win_h / GAME_H;
    if (scale_h < scale) scale = scale_h;

    SDL_Rect r;
    r.w = (int)(GAME_W * scale);
    r.h = (int)(GAME_H * scale);
    r.x = (win_w - r.w) / 2;
    r.y = (win_h - r.h) / 2;
    return r;
}

int main(int argc, char **argv) {
    srand((unsigned)time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // Start borderless-fullscreen at the desktop's native resolution.
    SDL_Window *window = SDL_CreateWindow(
        "Floppy Fish", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        INIT_WIN_W, INIT_WIN_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (!window) {
        fprintf(stderr, "CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "CreateRenderer failed: %s\n", SDL_GetError());
        return 1;
    }

    // Fixed-size canvas: created once, never recreated on resize.
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, GAME_W, GAME_H);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                              SDL_TEXTUREACCESS_STREAMING, GAME_W, GAME_H);

    int win_w = INIT_WIN_W, win_h = INIT_WIN_H;
    SDL_GetWindowSize(window, &win_w, &win_h);
    bool is_fullscreen = true;

    Visualizer vis = {};
    vis.width = GAME_W;
    vis.height = GAME_H;
    vis.time_offset = 0.0;
    vis.mouse_left_pressed = false;
    init_floppy_fish_system(&vis);

    Uint64 last_ticks = SDL_GetPerformanceCounter();
    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_F11) {
                is_fullscreen = !is_fullscreen;
                SDL_SetWindowFullscreen(window, is_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
            } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                vis.mouse_left_pressed = true;
            } else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                // Only the letterbox rect changes here - the game's own
                // canvas (GAME_W x GAME_H) and simulation state are untouched.
                win_w = e.window.data1;
                win_h = e.window.data2;
            }
        }

        Uint64 now = SDL_GetPerformanceCounter();
        double dt = (double)(now - last_ticks) / (double)SDL_GetPerformanceFrequency();
        last_ticks = now;
        if (dt > 0.05) dt = 0.05; // clamp stalls (window drag/resize) to avoid physics jumps

        vis.time_offset += dt;
        update_floppy_fish(&vis, dt);

        cairo_t *cr = cairo_create(surface);
        draw_floppy_fish(&vis, cr);
        cairo_destroy(cr);

        // Cairo's ARGB32 (premultiplied, native-endian) lines up with SDL's
        // ARGB8888 streaming texture byte-for-byte, so a row-by-row copy is
        // all that's needed to get the frame into the texture.
        cairo_surface_flush(surface);
        unsigned char *src = cairo_image_surface_get_data(surface);
        int src_stride = cairo_image_surface_get_stride(surface);

        void *pixels;
        int pitch;
        if (SDL_LockTexture(texture, NULL, &pixels, &pitch) == 0) {
            int row_bytes = GAME_W * 4;
            if (row_bytes > pitch) row_bytes = pitch;
            for (int y = 0; y < GAME_H; y++) {
                memcpy((unsigned char *)pixels + y * pitch, src + y * src_stride, row_bytes);
            }
            SDL_UnlockTexture(texture);
        }

        // Scale the fixed canvas into the window, preserving aspect ratio -
        // letterboxed (black bars) rather than stretched/distorted.
        SDL_Rect dst = compute_dest_rect(win_w, win_h);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, &dst);
        SDL_RenderPresent(renderer);
    }

    cairo_surface_destroy(surface);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
