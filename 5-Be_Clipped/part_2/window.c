#include <inttypes.h>
#include <stdlib.h>
#include "window.h"


//================| Window Class Implementation |================//

//Here's a quick, crappy pseudo-RNG since you probably don't have one
uint8_t pseudo_rand_8() {

    static uint16_t seed = 0;
    return (uint8_t)(seed = (12657 * seed + 12345) % 256);
}

//Window constructor
Window* Window_new(int16_t x, int16_t y,  
                   uint16_t width, uint16_t height, Context* context) {

    //Try to allocate space for a new WindowObj and fail through if malloc fails
    Window* window;
    if(!(window = (Window*)malloc(sizeof(Window))))
        return window;

    //Assign the property values
    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    window->context = context;

    return window;
}

//Let's start making things look like an actual window
void Window_paint(Window* window) {

    //Draw a 3px border around the window 
    Context_draw_rect(window->context, window->x, window->y,
                      window->width, window->height, WIN_BORDERCOLOR);
    Context_draw_rect(window->context, window->x + 1, window->y + 1,
                      window->width - 2, window->height - 2, WIN_BORDERCOLOR);
    Context_draw_rect(window->context, window->x + 2, window->y + 2,
                      window->width - 4, window->height - 4, WIN_BORDERCOLOR);
    
    //Draw a 3px border line under the titlebar
    Context_horizontal_line(window->context, window->x + 3, window->y + 28,
                            window->width - 6, WIN_BORDERCOLOR);
    Context_horizontal_line(window->context, window->x + 3, window->y + 29,
                            window->width - 6, WIN_BORDERCOLOR);
    Context_horizontal_line(window->context, window->x + 3, window->y + 30,
                            window->width - 6, WIN_BORDERCOLOR);

    //Fill in the titlebar background
    Context_fill_rect(window->context, window->x + 3, window->y + 3,
                      window->width - 6, 25, WIN_TITLECOLOR);

    //Fill in the window background
    Context_fill_rect(window->context, window->x + 3, window->y + 31,
                      window->width - 6, window->height - 34, WIN_BGCOLOR);
}
