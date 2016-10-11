#ifndef TEXTBOX_H
#define TEXTBOX_H

#include "window.h"

//Yet another basically-just-a-window class
typedef struct TextBox_struct {
    Window window;
} TextBox;

TextBox* TextBox_new(int x, int y, int width, int height);
void TextBox_paint(Window* text_box_window);

#endif //TEXTBOX_H