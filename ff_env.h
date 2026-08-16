#ifndef FF_ENV_H
#define FF_ENV_H

// Minimal stand-in for the parts of zenamp's Visualizer/visualization.h that
// floppyfish.cpp actually touches, so the game file can compile completely
// on its own (no GTK, no audio pipeline, no other visualizations).

#include <cairo.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

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
} Visualizer;

void init_floppy_fish_system(Visualizer *vis);
void update_floppy_fish(Visualizer *vis, double dt);
void draw_floppy_fish(Visualizer *vis, cairo_t *cr);

#endif
