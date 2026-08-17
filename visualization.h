#ifndef FF_ENV_H
#define FF_ENV_H

// Minimal stand-in for the parts of zenamp's Visualizer/visualization.h that
// floppyfish.cpp actually touches, so the game file can compile completely
// on its own (no GTK, no audio pipeline, no other visualizations).

#include <cairo.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI  3.14159265359
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef struct {
    int width, height;
    double time_offset;      // seconds since start, advanced by the host app
    bool mouse_left_pressed; // host sets true on click; game clears it after handling
    bool mouse_middle_held;  // host keeps this true for as long as the middle button is held; game only reads it, never clears it
    bool sound_flap;         // game sets true the frame the fish flaps; host plays a sound and it self-clears next frame
    bool sound_score;        // game sets true the frame a point is scored; same contract
    bool sound_dead;         // Floppy fish is dead
    int deadcounter;
} Visualizer;

void init_floppy_fish_system(Visualizer *vis);
void update_floppy_fish(Visualizer *vis, double dt);
void draw_floppy_fish(Visualizer *vis, cairo_t *cr);

#endif
