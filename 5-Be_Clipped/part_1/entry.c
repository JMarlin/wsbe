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
    Desktop_create_window(desktop, 10, 10, 300, 200);
    Desktop_create_window(desktop, 100, 150, 400, 400);
    Desktop_create_window(desktop, 200, 100, 200, 600);

    //Draw them
    Desktop_paint(desktop);

    //Install our handler of mouse events
    fake_os_installMouseCallback(main_mouse_callback);

    //If you were doing a more standard top level event loop
    //(eg: weren't dealing with the quirks of making this thing
    //compile to JS), it would look more like this:
    //    while(1) {
    //
    //        fake_os_waitForMouseUpdate(&mouse_x, &mouse_y, &buttons);
    //        Desktop_process_mouse(desktop, mouse_x, mouse_y, buttons);
    //    }

    //In a real OS, since we wouldn't want this thread unloaded and 
    //thereby lose our callback code, you would probably want to
    //hang here or something if using a callback model
    return 0; 
}
