#pragma once

#include <stdint.h>

void init_mouse();
void mouse_draw_cursor();
void reset_mouse_cursor_state();
void mouse_set_speed(int speed);
void mouse_set_natural_scroll(bool enabled);
void mouse_set_large_cursor(bool enabled);
