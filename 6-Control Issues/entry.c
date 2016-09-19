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

//Create and draw a few rectangles and exit
int main(int argc, char* argv[]) {

    //Fill this in with the info particular to your project
    Context* context = Context_new(0, 0, 0);
    context->buffer = fake_os_getActiveVesaBuffer(&context->width, &context->height);

    //Create the desktop 
    desktop = Desktop_new(context);

    //Sprinkle it with windows 
    Window_create_window((Window*)desktop, 10, 10, 300, 200, 0);
    Window_create_window((Window*)desktop, 100, 150, 400, 400, 0);
    Window_create_window((Window*)desktop, 200, 100, 200, 600, 0);

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
