#include "textbox.h"

TextBox* TextBox_new(int x, int y, int width, int height) {

    //Basically the same thing as button init
    TextBox* text_box;
    if(!(text_box = (TextBox*)malloc(sizeof(TextBox))))
        return text_box;

    if(!Window_init((Window*)text_box, x, y, width, height, WIN_NODECORATION, (Context*)0)) {

        free(text_box);
        return (TextBox*)0;
    }

    //Override default window draw callback
    text_box->window.paint_function = TextBox_paint;

    return text_box;
}

void TextBox_paint(Window* text_box_window) {

    int title_len;

    //White background
    Context_fill_rect(text_box_window->context, 1, 1, text_box_window->width - 2,
                      text_box_window->height - 2, 0xFFFFFFFF);

    //Simple black border
    Context_draw_rect(text_box_window->context, 0, 0, text_box_window->width,
                      text_box_window->height, 0xFF000000);

    //Get the title length
    for(title_len = 0; text_box_window->title[title_len]; title_len++);

    //Convert it into pixels
    title_len *= 8;

    //Draw the title centered within the button
    if(text_box_window->title)
        Context_draw_text(text_box_window->context, text_box_window->title,
                          text_box_window->width - title_len - 6,
                          (text_box_window->height / 2) - 6,
                          0xFF000000);                                    
}
