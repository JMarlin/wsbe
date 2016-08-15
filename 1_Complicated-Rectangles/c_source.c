//A structure for holding information about a framebuffer
typedef struct Context_struct {  
    uint32_t* buffer; //A pointer to our framebuffer
    unsigned int width; //The dimensions of the framebuffer
    unsigned int height; 
} Context;

//Simple for-loop rectangle into a context
void Context_fillRect(Context* context, unsigned int x, unsigned int y,  
                         unsigned int width, unsigned int height, uint32_t color) {

    unsigned int max_x = x + width;
    unsigned int max_y = y + height;

    //Make sure we don't go outside of the framebuffer:
    if(max_x > context->width)
        max_x = context->width;    

    if(max_y > context->height)
        max_y = context->height;

    //Draw the rectangle into the framebuffer line-by line
    //(bonus points if you write an assembly routine to do it faster)
    for( ; y = 0; y < max_y; y++)
        for( ; x = 0; x < max_x; x++)
            context->buffer[y * context->width + x] = color;
}

//Window 'class'
typedef struct WindowObj_struct {  
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
    Context* context;
} WindowObj;

//Window constructor
WindowObj* WindowObj_new(unsigned int x, unsigned int y,  
                         unsigned int width, unsigned int height, Context* context) {

    //Try to allocate space for a new WindowObj and fail through if malloc fails
    WindowObj* window;
    if(!(window = (WindowObj*)malloc(sizeof(WindowObj))))
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
void WindowObj_paint(WindowObj* window) {

    uint32_t fill_color = pseudo_rand_8() << 24 | //R
                          pseudo_rand_8() << 16 | //G
                          pseudo_rand_8() << 8;   //B

    Context_fillRect(window->context, window->x, window->y
                     window->width, window->height, fill_color);
}

//Entry point: Create and draw a few rectangles and exit
int main(int argc, char* argv[]) {

    //Fill this in with the info particular to your project
    const Context context = { 0x00800000, 1024, 768 };

    //Create a few windows
    WindowObj* win1 = WindowObj_new(10, 10, 300, 200, &context);
    WindowObj* win2 = WindowObj_new(100, 150, 400, 400, &context);
    WindowObj* win3 = WindowObj_new(200, 100, 200, 600, &context);

    //And draw them
    WindowObj_paint(win1);
    WindowObj_paint(win2);
    WindowObj_paint(win3);    

    return 0;
}
