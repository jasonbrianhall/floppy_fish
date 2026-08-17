#include <SDL2/SDL.h>
#include <cairo.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cmath>
#include <vector>
#include "visualization.h"

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

// --- Procedural sound effects -----------------------------------------------
// No audio assets on disk - both sounds are synthesized on the fly as short
// PCM buffers and handed to SDL's audio queue, so there's nothing to ship
// or load.
static SDL_AudioDeviceID g_audio_dev = 0;
static SDL_AudioSpec g_audio_spec;

static void audio_init() {
    SDL_AudioSpec want;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 1024;
    g_audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &g_audio_spec, 0);
    if (g_audio_dev != 0) SDL_PauseAudioDevice(g_audio_dev, 0);
    else fprintf(stderr, "Audio unavailable: %s\n", SDL_GetError());
}

// Queues a short frequency sweep from f0 to f1 Hz over dur seconds, with a
// linear fade-out envelope so it doesn't click at the end.
static void audio_play_sweep(double f0, double f1, double dur, double volume) {
    if (g_audio_dev == 0) return;
    int sr = g_audio_spec.freq;
    int n = (int)(sr * dur);
    if (n <= 0) return;
    std::vector<Sint16> buf(n);
    double phase = 0.0;
    for (int i = 0; i < n; i++) {
        double t = (double)i / sr;
        double freq = f0 + (f1 - f0) * (t / dur);
        phase += 2.0 * M_PI * freq / sr;
        double env = 1.0 - (double)i / n;
        double s = sin(phase) * env * volume;
        buf[i] = (Sint16)(s * 32000.0);
    }
    SDL_QueueAudio(g_audio_dev, buf.data(), (Uint32)(buf.size() * sizeof(Sint16)));
}

// Queues a short sequence of steady notes back to back, each with its own
// quick decay envelope - used for the little rising "chime" on scoring.
static void audio_play_notes(const double *freqs, const double *durs, int count, double volume) {
    if (g_audio_dev == 0) return;
    int sr = g_audio_spec.freq;
    std::vector<Sint16> buf;
    for (int k = 0; k < count; k++) {
        int n = (int)(sr * durs[k]);
        double phase = 0.0;
        for (int i = 0; i < n; i++) {
            double env = 1.0 - (double)i / n;
            phase += 2.0 * M_PI * freqs[k] / sr;
            double s = sin(phase) * env * volume;
            buf.push_back((Sint16)(s * 32000.0));
        }
    }
    if (!buf.empty()) SDL_QueueAudio(g_audio_dev, buf.data(), (Uint32)(buf.size() * sizeof(Sint16)));
}

// A quick upward "whoosh" for the flap.
static void audio_play_flap() {
    audio_play_sweep(320.0, 640.0, 0.09, 0.28);
}

// A bright two-note rising chime for scoring a point.
static void audio_play_score() {
    static const double freqs[2] = {880.0, 1318.5};  // A5 -> E6
    static const double durs[2]  = {0.07, 0.11};
    audio_play_notes(freqs, durs, 2, 0.30);
}

static void audio_play_dead() {
    // 1. Sharp watery impact (fast upward blip)
    audio_play_sweep(300.0, 520.0, 0.03, 0.40);

    // 2. Droplet scatter (three tiny jitter sweeps)
    for (int i = 0; i < 3; i++) {
        double start = 600.0 + (rand() % 200);   // 600–800 Hz
        double end   = 350.0 + (rand() % 150);   // 350–500 Hz
        audio_play_sweep(start, end, 0.02, 0.35);
    }

    // 3. Heavy wet flop (slow downward bend)
    audio_play_sweep(480.0, 160.0, 0.12, 0.45);
}

/*
// A short, squishy "thud" when the floppy fish dies.
static void audio_play_dead() {
    // Fake "noise" by jittering a few tiny sweeps
    for (int i = 0; i < 3; i++) {
        double start = 300.0 + (rand() % 80);   // 300–380 Hz
        double end   = 180.0 + (rand() % 60);   // 180–240 Hz
        audio_play_sweep(start, end, 0.03, 0.30);
    }

    // Add the downward "deflated flop"
    audio_play_sweep(420.0, 180.0, 0.10, 0.30);
}*/



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

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    audio_init();

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
    bool is_floppy_fish_sound = true;
    Visualizer vis = {};
    vis.width = GAME_W;
    vis.height = GAME_H;
    vis.time_offset = 0.0;
    vis.mouse_left_pressed = false;
    vis.mouse_middle_held = false;
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
            } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_s) {
                is_floppy_fish_sound = !is_floppy_fish_sound ;
            } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_F11) {
                is_fullscreen = !is_fullscreen;
                SDL_SetWindowFullscreen(window, is_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
            } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                vis.mouse_left_pressed = true;
                vis.deadcounter=0;
            } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_MIDDLE) {
                vis.mouse_middle_held = true;
            } else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_MIDDLE) {
                vis.mouse_middle_held = false;
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
        if (is_floppy_fish_sound)  {
            if (vis.sound_flap) audio_play_flap();
            if (vis.sound_score) audio_play_score();
            if (vis.sound_dead) audio_play_dead();
        }
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

    shutdown_floppy_fish_system();
    cairo_surface_destroy(surface);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    if (g_audio_dev != 0) SDL_CloseAudioDevice(g_audio_dev);

    // Must come after every other cairo object (surfaces, font faces, ...)
    // has been destroyed, and no cairo calls may happen after it. Frees
    // cairo's own internal static caches (e.g. its toy-font-face hash
    // table) that it otherwise intentionally keeps alive for the life of
    // the process - this is purely so leak checkers like ASan/LSan don't
    // flag them; real (non-debug) builds don't need this call.
    cairo_debug_reset_static_data();

    SDL_Quit();
    return 0;
}
