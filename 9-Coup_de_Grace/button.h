#ifndef BUTTON_H
#define BUTTON_H

#include "window.h"

struct Button_struct;

typedef void (*ButtonMousedownHandler)(struct Button_struct*, int, int);

typedef struct Button_struct {
    Window window;
    uint8_t color_toggle;
    ButtonMousedownHandler onmousedown;
} Button;

Button* Button_new(int x, int y, int w, int h);
void Button_mousedown_handler(Window* button_window, int x, int y);
void Button_paint(Window* button_window);

#endif //BUTTON_H