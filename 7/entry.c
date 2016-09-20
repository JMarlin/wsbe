#include "context.h"
#include "desktop.h"
#include "../fake_lib/fake_os.h"

//================| Entry Point |================//

//Our desktop object needs to be sharable by our main function
//as well as our mouse event callback
Desktop* desktop;

//The callback that our mouse device will trigger on mouse updates
void main_mouse_callback(uint16_t mouse_x, uint16_t mouse_y, uint8_t buttons) {

    Desktop_process_mouse(desktop, mouse_x, mouse_y, buttons);
}

void Button_paint(Window* window) {

    Context_fill_rect(window->context, 1, 1, window->width - 1,
                      window->height - 1, WIN_BGCOLOR);
    Context_draw_rect(window->context, 0, 0, window->width,
                      window->height, 0xFF000000);
    Context_draw_rect(window->context, 3, 3, window->width - 6,
                      window->height - 6, WIN_BGCOLOR - 0x101010);
    Context_draw_rect(window->context, 4, 4, window->width - 8,
                      window->height - 8, WIN_BGCOLOR - 0x101010);                                        
}

//Create and draw a few rectangles and exit
int main(int argc, char* argv[]) {

    //Fill this in with the info particular to your project
    Context* context = Context_new(0, 0, 0);
    context->buffer = fake_os_getActiveVesaBuffer(&context->width, &context->height);

    //Create the desktop 
    desktop = Desktop_new(context);

    //Sprinkle it with windows 
    Window_create_window((Window*)desktop, 10, 10, 300, 200, 0);
    Window* win_1 = Window_create_window((Window*)desktop, 100, 150, 400, 400, 0);
    Window_create_window((Window*)desktop, 200, 100, 200, 600, 0);
    Window* child = Window_create_window(win_1, 307, 357, 80, 40, 0 /*WIN_NODECORATION*/);
    Window* subchild = Window_create_window(child, 5, 5, 40, 20, WIN_NODECORATION);
    //child->paint_function = Button_paint;
    subchild->paint_function = Button_paint;

    //Draw them
    Window_paint((Window*)desktop);

    //Install our handler of mouse events
    fake_os_installMouseCallback(main_mouse_callback);

    //Polling alternative:
    //    while(1) {
    //
    //        fake_os_waitForMouseUpdate(&mouse_x, &mouse_y, &buttons);
    //        Desktop_process_mouse(desktop, mouse_x, mouse_y, buttons);
    //    }

    return 0; 
}
