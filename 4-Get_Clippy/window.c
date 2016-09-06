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

    //Moving the color assignment to the window constructor
    //so that we don't get a different color on every redraw
    window->fill_color = 0xFF000000 |            //Opacity
                         pseudo_rand_8() << 16 | //B
                         pseudo_rand_8() << 8  | //G
                         pseudo_rand_8();        //R

    return window;
}

//Method for painting a WindowObj to its context:
void Window_paint(Window* window) {

    Context_fill_rect(window->context, window->x, window->y,
                      window->width, window->height, window->fill_color);
}
