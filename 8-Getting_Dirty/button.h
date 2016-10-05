#ifndef BUTTON_H
#define BUTTON_H

#include "window.h"

typedef struct Button_struct {
    Window window;
    uint8_t color_toggle;
} Button;

Button* Button_new(int x, int y, int w, int h);
void Button_mousedown_handler(Window* button_window, int x, int y);
void Button_paint(Window* button_window);

#endif //BUTTON_H