#ifndef WINDOW_H
#define WINDOW_H 

#include "context.h"
#include <inttypes.h>

//================| Window Class Declaration |================//

//Feel free to play with this 'theme'
#define WIN_BGCOLOR     0xFFBBBBBB //A generic grey
#define WIN_TITLECOLOR  0xFFBE9270 //A nice subtle blue
#define WIN_BORDERCOLOR 0xFF000000 //Straight-up black

typedef struct Window_struct {  
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    Context* context;
} Window;

//Methods
Window* Window_new(int16_t x, int16_t y,  
                   uint16_t width, uint16_t height, Context* context);
void Window_paint(Window* window);

#endif //WINDOW_H
