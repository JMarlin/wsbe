#ifndef WINDOW_H
#define WINDOW_H 

#include "context.h"
#include <inttypes.h>

//================| Window Class Declaration |================//

typedef struct Window_struct {  
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    Context* context;
} Window;

//Methods
Window* Window_new(uint16_t x, uint16_t y,  
                   uint16_t width, uint16_t height, Context* context);
void Window_paint(Window* window);

#endif //WINDOW_H
