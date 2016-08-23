#ifndef CONTEXT_H
#define CONTEXT_H

#include <inttypes.h>

//================| Context Class Declaration |================//

//A structure for holding information about a framebuffer
typedef struct Context_struct {  
    uint32_t* buffer; //A pointer to our framebuffer
    uint16_t width; //The dimensions of the framebuffer
    uint16_t height; 
} Context;

//Methods
void Context_fillRect(Context* context, unsigned int x, unsigned int y,  
                      unsigned int width, unsigned int height, uint32_t color);

#endif //CONTEXT_H
