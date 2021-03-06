#include "../fake_lib/fake_os.h"
#include <inttypes.h>
#include <stdlib.h>

//A structure for holding information about a framebuffer
typedef struct Context_struct {  
    uint32_t* buffer; //A pointer to our framebuffer
    uint16_t width; //The dimensions of the framebuffer
    uint16_t height; 
} Context;

//Simple for-loop rectangle into a context
void Context_fillRect(Context* context, unsigned int x, unsigned int y,  
                      unsigned int width, unsigned int height, uint32_t color) {

    unsigned int cur_x;
    unsigned int max_x = x + width;
    unsigned int max_y = y + height;

    //Make sure we don't go outside of the framebuffer:
    if(max_x > context->width)
        max_x = context->width;    

    if(max_y > context->height)
        max_y = context->height;

    //Draw the rectangle into the framebuffer line-by line
    //(bonus points if you write an assembly routine to do it faster)
    for( ; y < max_y; y++) 
        for(cur_x = x ; cur_x < max_x; cur_x++) 
            context->buffer[y * context->width + cur_x] = color;
}

//Window 'class'
typedef struct Window_struct {  
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    Context* context;
} Window;

//Window constructor
Window* Window_new(unsigned int x, unsigned int y,  
                   unsigned int width, unsigned int height, Context* context) {

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

//Here's a quick, crappy pseudo-RNG since you probably don't have one
uint8_t pseudo_rand_8() {

    static uint16_t seed = 0;
    return (uint8_t)(seed = (12657 * seed + 12345) % 256);
}

//Method for painting a WindowObj to its context:
void Window_paint(Window* window) {

    uint32_t fill_color = 0xFF000000 |            //Opacity
                          pseudo_rand_8() << 16 | //B
                          pseudo_rand_8() << 8  | //G
                          pseudo_rand_8();        //R

    Context_fillRect(window->context, window->x, window->y,
                     window->width, window->height, fill_color);
}

//Entry point: Create and draw a few rectangles and exit
int main(int argc, char* argv[]) {

    //Fill this in with the info particular to your project
    Context context = { 0, 0, 0 };
    context.buffer = fake_os_getActiveVesaBuffer(&context.width, &context.height);

    //Create a few windows
    Window* win1 = Window_new(10, 10, 300, 200, &context);
    Window* win2 = Window_new(100, 150, 400, 400, &context);
    Window* win3 = Window_new(200, 100, 200, 600, &context);

    //And draw them
    Window_paint(win1);
    Window_paint(win2);
    Window_paint(win3);    

    return 0;
}
