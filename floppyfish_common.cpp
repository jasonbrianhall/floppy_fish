#include "floppyfish_common.h"

#ifndef M_PI
#define M_PI  3.14159265359
#endif

double ff_hash(double n) {
    double x = sin(n * 127.1) * 43758.5453;
    return x - floor(x);
}

double ff_edge_fade(double x, double w) {
    double fade_zone = w * 0.15;
    double fade = 1.0;
    if (x < fade_zone) fade = x / fade_zone;
    else if (x > w - fade_zone) fade = (w - x) / fade_zone;
    if (fade < 0.0) fade = 0.0;
    if (fade > 1.0) fade = 1.0;
    return fade;
}

// Picks which theme's obstacle art to draw - the one piece of code that
// needs to know all four themes exist.
void ff_draw_obstacle_column(int theme, cairo_t *cr, double x, double y0, double y1,
                              double width, double seed, bool tip_at_y1) {
    switch (theme) {
        case FF_THEME_SHIP:     ff_draw_ship_column(cr, x, y0, y1, width, seed, tip_at_y1); break;
        case FF_THEME_CAVE:     ff_draw_cave_column(cr, x, y0, y1, width, seed, tip_at_y1); break;
        case FF_THEME_ATLANTIS: ff_draw_atlantis_column(cr, x, y0, y1, width, seed, tip_at_y1); break;
        default:                ff_draw_coral_column(cr, x, y0, y1, width, seed, tip_at_y1); break;
    }
}

// Sky gradient, ambient particles, and a distant skyline silhouette for one
// theme. Meant to be composited via cairo_push_group/paint_with_alpha at the
// call site to crossfade between two themes, so this just paints at full
// opacity as if it were the only thing on screen.
void ff_draw_theme_sky(cairo_t *cr, int theme, double w, double h, double bubble_phase) {
    double top_r, top_g, top_b, bot_r, bot_g, bot_b;
    double pr, pg, pb, pa;
    switch (theme) {
        case FF_THEME_SHIP:
            ff_ship_sky_colors(&top_r, &top_g, &top_b, &bot_r, &bot_g, &bot_b);
            ff_ship_particle_color(&pr, &pg, &pb, &pa);
            break;
        case FF_THEME_CAVE:
            ff_cave_sky_colors(&top_r, &top_g, &top_b, &bot_r, &bot_g, &bot_b);
            ff_cave_particle_color(&pr, &pg, &pb, &pa);
            break;
        case FF_THEME_ATLANTIS:
            ff_atlantis_sky_colors(&top_r, &top_g, &top_b, &bot_r, &bot_g, &bot_b);
            ff_atlantis_particle_color(&pr, &pg, &pb, &pa);
            break;
        default:
            ff_reef_sky_colors(&top_r, &top_g, &top_b, &bot_r, &bot_g, &bot_b);
            ff_reef_particle_color(&pr, &pg, &pb, &pa);
            break;
    }

    cairo_pattern_t *sky = cairo_pattern_create_linear(0, 0, 0, h);
    cairo_pattern_add_color_stop_rgb(sky, 0.0, top_r, top_g, top_b);
    cairo_pattern_add_color_stop_rgb(sky, 1.0, bot_r, bot_g, bot_b);
    cairo_set_source(cr, sky);
    cairo_paint(cr);
    cairo_pattern_destroy(sky);

    // Ambient particles: rising bubbles/motes, colored per theme.
    for (int i = 0; i < 22; i++) {
        double bx = fmod(i * 53.0 + w * 0.5, w);
        double speed = 30.0 + (i % 5) * 10.0;
        double by = h - fmod(bubble_phase * speed + i * 71.0, h + 40.0);
        double size = 2.0 + (i % 4);
        cairo_set_source_rgba(cr, pr, pg, pb, pa);
        cairo_arc(cr, bx, by, size, 0, 2 * M_PI);
        cairo_fill(cr);
    }

    double floor_h = h * 0.10;
    double base_y = h - floor_h;
    switch (theme) {
        case FF_THEME_SHIP:     ff_draw_ship_backdrop(cr, w, h, base_y); break;
        case FF_THEME_CAVE:     ff_draw_cave_backdrop(cr, w, h, base_y); break;
        case FF_THEME_ATLANTIS: ff_draw_atlantis_backdrop(cr, w, h, base_y); break;
        default:                ff_draw_reef_backdrop(cr, w, h, base_y); break;
    }
}

// Floor for one theme: sand, ship-deck planking, dark rock, or Atlantean
// mosaic. Same crossfade-via-compositing contract as ff_draw_theme_sky.
void ff_draw_theme_floor(cairo_t *cr, int theme, double w, double h, double floor_h, double bubble_phase) {
    switch (theme) {
        case FF_THEME_SHIP:     ff_draw_ship_floor(cr, w, h, floor_h, bubble_phase); break;
        case FF_THEME_CAVE:     ff_draw_cave_floor(cr, w, h, floor_h, bubble_phase); break;
        case FF_THEME_ATLANTIS: ff_draw_atlantis_floor(cr, w, h, floor_h, bubble_phase); break;
        default:                ff_draw_reef_floor(cr, w, h, floor_h, bubble_phase); break;
    }
}

void ff_draw_seaweed(cairo_t *cr, double x, double base_y, double height, double t, int theme, double alpha_mult) {
    if (alpha_mult <= 0.0) return;
    switch (theme) {
        case FF_THEME_SHIP:     ff_draw_ship_seaweed(cr, x, base_y, height, t, alpha_mult); break;
        case FF_THEME_CAVE:     ff_draw_cave_seaweed(cr, x, base_y, height, t, alpha_mult); break;
        case FF_THEME_ATLANTIS: ff_draw_atlantis_seaweed(cr, x, base_y, height, t, alpha_mult); break;
        default:                ff_draw_reef_seaweed(cr, x, base_y, height, t, alpha_mult); break;
    }
}
