#include <inttypes.h>
#include "context.h"


//================| Context Class Implementation |================//

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
