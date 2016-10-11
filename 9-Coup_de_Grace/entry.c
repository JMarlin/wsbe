#include "context.h"
#include "desktop.h"
#include "calculator.h"
#include "../fake_lib/fake_os.h"

//================| Entry Point |================//

//Our desktop object needs to be sharable by our main function
//as well as our mouse event callback
Desktop* desktop;

//The callback that our mouse device will trigger on mouse updates
void main_mouse_callback(uint16_t mouse_x, uint16_t mouse_y, uint8_t buttons) {

    Desktop_process_mouse(desktop, mouse_x, mouse_y, buttons);
}

//Button handler for creating a new calculator
void spawn_calculator(Button* button, int x, int y) {

    //Create and install a calculator
    Calculator* temp_calc = Calculator_new();
    Window_insert_child((Window*)desktop, (Window*)temp_calc);
    Window_move((Window*)temp_calc, 0, 0);
}

//Create and draw a few rectangles and exit
int main(int argc, char* argv[]) {

    //Fill this in with the info particular to your project
    Context* context = Context_new(0, 0, 0);
    context->buffer = fake_os_getActiveVesaBuffer(&context->width, &context->height);

    //Create the desktop 
    desktop = Desktop_new(context);

    //Create a simple launcher window 
    Button* launch_button = Button_new(10, 10, 150, 30);
    Window_set_title((Window*)launch_button, "New Calculator");
    launch_button->onmousedown = spawn_calculator;
    Window_insert_child((Window*)desktop, (Window*)launch_button);

    //Initial draw
    Window_paint((Window*)desktop, (List*)0, 1);

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
