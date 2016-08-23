#include <inttypes.h>
#include <stdlib.h>
#include "desktop.h"


//================| Desktop Class Implementation |================//

Desktop* Desktop_new(Context* context) {

    //Malloc or fail 
    Desktop* desktop;
    if(!(desktop = (Desktop*)malloc(sizeof(Desktop))))
        return desktop;

    //Create child list or clean up and fail
    if(!(desktop->children = List_new())) {

        //Delete the new desktop object and return null 
        free(desktop);
        return (Desktop*)0;
    }

    //Fill out other properties 
    desktop->context = context;

    return desktop;
}

//A method to automatically create a new window in the provided desktop 
Window* Desktop_create_window(Desktop* desktop, unsigned int x, unsigned int y,  
                          unsigned int width, unsigned int height) {

    //Attempt to create the window instance
    Window* window;
    if(!(window = Window_new(x, y, width, height, desktop->context)))
        return window;

    //Attempt to add the window to the end of the desktop's children list
    //If we fail, make sure to clean up all of our allocations so far 
    if(!List_add(desktop->children, (void*)window)) {

        free(window);
        return (Window*)0;
    }

    return window;
}


//Paint the desktop 
void Desktop_paint(Desktop* desktop) {
  
    //Loop through all of the children and call paint on each of them 
    unsigned int i;
    Window* current_window;

    //Start by clearing the desktop background
    Context_fillRect(desktop->context, 0, 0, desktop->context->width,
                     desktop->context->height, 0xFFFF9933); //Change pixel format if needed 
                                                            //Currently: ABGR

    //Get and draw windows until we stop getting valid windows out of the list 
    for(i = 0; (current_window = (Window*)List_get_at(desktop->children, i)); i++)
        Window_paint(current_window);
        
}
