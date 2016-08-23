#include "context.h"
#include "desktop.h"
#include "../fake_lib/fake_os.h"

//================| Entry Point |================//

//Create and draw a few rectangles and exit
int main(int argc, char* argv[]) {

    //Fill this in with the info particular to your project
    Context context = { 0, 0, 0 };
    context.buffer = fake_os_getActiveVesaBuffer(&context.width, &context.height);

    //Create the desktop 
    Desktop* desktop = Desktop_new(&context);

    //Sprinkle it with windows 
    Desktop_create_window(desktop, 10, 10, 300, 200);
    Desktop_create_window(desktop, 100, 150, 400, 400);
    Desktop_create_window(desktop, 200, 100, 200, 600);

    //And draw them
    Desktop_paint(desktop);

    return 0;
}
