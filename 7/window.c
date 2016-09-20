#include <inttypes.h>
#include <stdlib.h>
#include "window.h"


//================| Window Class Implementation |================//

//Here's a quick, crappy pseudo-RNG since you probably don't have one
uint8_t pseudo_rand_8() {

    static uint16_t seed = 0;
    return (uint8_t)(seed = (12657 * seed + 12345) % 256);
}

//Window constructor
Window* Window_new(int16_t x, int16_t y, uint16_t width,
                   uint16_t height, uint16_t flags, Context* context) {

    //Try to allocate space for a new WindowObj and fail through if malloc fails
    Window* window;
    if(!(window = (Window*)malloc(sizeof(Window))))
        return window;

    //Attempt to initialize the new window
    if(!Window_init(window, x, y, width, height, flags, context)) {
    
        free(window);
        return (Window*)0;
    }

    return window;
}

//Seperate object allocation from initialization so we can implement
//our inheritance scheme
int Window_init(Window* window, int16_t x, int16_t y, uint16_t width,
                uint16_t height, uint16_t flags, Context* context) {

    //Moved over here from the desktop 
    //Create child list or clean up and fail
    if(!(window->children = List_new()))
        return 0;

    //Assign the property values
    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    window->context = context;
    window->flags = flags;
    window->parent = (Window*)0;
    window->drag_child = (Window*)0;
    window->drag_off_x = 0;
    window->drag_off_y = 0;
    window->last_button_state = 0;
    window->paint_function = Window_paint_handler;
  
    return 1;
}

//Recursively get the absolute on-screen x-coordinate of this window
int Window_screen_x(Window* window) {

    if(window->parent)
        if(window->parent->flags & WIN_NODECORATION)
            return window->x + Window_screen_x(window->parent);
        else
            return window->x + Window_screen_x(window->parent) + WIN_BORDERWIDTH;
    
    return window->x;
}

//Recursively get the absolute on-screen y-coordinate of this window
int Window_screen_y(Window* window) {

    if(window->parent)
        if(window->parent->flags & WIN_NODECORATION)
            return window->y + Window_screen_y(window->parent);
        else
            return window->y + Window_screen_y(window->parent) + WIN_TITLEHEIGHT;

    return window->y;
}

void Window_draw_border(Window* window) {

    int screen_x = Window_screen_x(window);
    int screen_y = Window_screen_y(window);

    //Draw a 3px border around the window 
    Context_draw_rect(window->context, screen_x, screen_y,
                      window->width, window->height, WIN_BORDERCOLOR);
    Context_draw_rect(window->context, screen_x + 1, screen_y + 1,
                      window->width - 2, window->height - 2, WIN_BORDERCOLOR);
    Context_draw_rect(window->context, screen_x + 2, screen_y + 2,
                      window->width - 4, window->height - 4, WIN_BORDERCOLOR);
    
    //Draw a 3px border line under the titlebar
    Context_horizontal_line(window->context, screen_x + 3, screen_y + 28,
                            window->width - 6, WIN_BORDERCOLOR);
    Context_horizontal_line(window->context, screen_x + 3, screen_y + 29,
                            window->width - 6, WIN_BORDERCOLOR);
    Context_horizontal_line(window->context, screen_x + 3, screen_y + 30,
                            window->width - 6, WIN_BORDERCOLOR);

    //Fill in the titlebar background
    Context_fill_rect(window->context, screen_x + 3, screen_y + 3,
                      window->width - 6, 25, WIN_TITLECOLOR);
}

//Apply clipping for window bounds without subtracting child window rects
void Window_apply_bound_clipping(Window* window, int in_recursion) {

    Rect* temp_rect;
    int screen_x, screen_y;
    List* clip_windows;
    Window* clipping_window;

    //Build the visibility rectangle for this window
    //If the window is decorated and we're recursing, we want to limit
    //the window's drawable area to the area inside the window decoration.
    //If we're not recursing, however, it means we're about to paint 
    //ourself and therefore we want to wait until we've finished painting
    //the window border to shrink the clipping area 
    screen_x = Window_screen_x(window);
    screen_y = Window_screen_y(window);
    
    if((!(window->flags & WIN_NODECORATION)) && in_recursion) {

        //Limit client drawable area 
        screen_x += WIN_BORDERWIDTH;
        screen_y += WIN_TITLEHEIGHT;
        temp_rect = Rect_new(screen_y, screen_x,
                             screen_y + window->height - WIN_TITLEHEIGHT - WIN_BORDERWIDTH - 1, 
                             screen_x + window->width - (2*WIN_BORDERWIDTH) - 1);
    } else {

        temp_rect = Rect_new(screen_y, screen_x, screen_y + window->height - 1, 
                             screen_x + window->width - 1);
    }

    //If there's no parent (meaning we're at the top of the window tree)
    //then we just add our rectangle and exit
    if(!window->parent) {

        Context_add_clip_rect(window->context, temp_rect);
        return;
    }

    //Otherwise, we first reduce our clipping area to the visibility area of our parent
    Window_apply_bound_clipping(window->parent, 1);

    //Now that we've reduced our clipping area to our parent's clipping area, we can
    //intersect our own bounds rectangle to get our main visible area  
    Context_intersect_clip_rect(window->context, temp_rect);

    //And finally, we subtract the rectangles of any siblings that are occluding us 
    clip_windows = Window_get_windows_above(window->parent, window);

    while(clip_windows->count) {
        
        clipping_window = (Window*)List_remove_at(clip_windows, 0);

        //Make sure we don't try and clip the window from itself
        if(clipping_window == window)
            continue;

        //Get a rectangle from the window, subtract it from the clipping 
        //region, and dispose of it
        screen_x = Window_screen_x(clipping_window);
        screen_y = Window_screen_y(clipping_window);

        temp_rect = Rect_new(screen_y, screen_x,
                             screen_y + clipping_window->height - 1,
                             screen_x + clipping_window->width - 1);
        Context_subtract_clip_rect(window->context, temp_rect);
        free(temp_rect);
    }

    //Dispose of the used-up list 
    free(clip_windows);
}

//Another override-redirect function
void Window_paint(Window* window) {

    int i, screen_x, screen_y, child_screen_x, child_screen_y;
    Window* current_child;
    Rect* temp_rect;

    //Start by limiting painting to the window's visible area
    Window_apply_bound_clipping(window, 0);

    //Set the context translation
    screen_x = Window_screen_x(window);
    screen_y = Window_screen_y(window);

    //If we have window decorations turned on, draw them and then further
    //limit the clipping area to the inner drawable area of the window 
    if(!(window->flags & WIN_NODECORATION)) {

        //Draw border
        Window_draw_border(window);

        //Limit client drawable area 
        screen_x += WIN_BORDERWIDTH;
        screen_y += WIN_TITLEHEIGHT;
        temp_rect = Rect_new(screen_y, screen_x,
                             screen_y + window->height - WIN_TITLEHEIGHT - WIN_BORDERWIDTH - 1, 
                             screen_x + window->width - (2*WIN_BORDERWIDTH) - 1);
        Context_intersect_clip_rect(window->context, temp_rect);
    }

    //Then subtract the screen rectangles of any children 
    //NOTE: We don't do this in Window_apply_bound_clipping because, due to 
    //its recursive nature, it would cause the screen rectangles of all of 
    //our parent's children to be subtracted from the clipping area -- which
    //would eliminate this window. 
    for(i = 0; i < window->children->count; i++) {

        current_child = (Window*)List_get_at(window->children, i);

        child_screen_x = Window_screen_x(current_child);
        child_screen_y = Window_screen_y(current_child);

        temp_rect = Rect_new(child_screen_y, child_screen_x,
                             child_screen_y + current_child->height - 1,
                             child_screen_x + current_child->width - 1);
        Context_subtract_clip_rect(window->context, temp_rect);
        free(temp_rect);
    }

    //Finally, with all the clipping set up, we can set the context's 0,0 to the top-left corner
    //of the window's drawable area, and call the window's final paint function 
    window->context->translate_x = screen_x;
    window->context->translate_y = screen_y;
    window->paint_function(window);

    //Now that we're done drawing this window, we can clear the changes we made to the context
    Context_clear_clip_rects(window->context);
    window->context->translate_x = 0;
    window->context->translate_y = 0;
    
    //Since we're still painting the whole screen whenever anything changes, we must also 
    //call on all of our children to paint themselves so we don't miss painting anything 
    for(i = 0; i < window->children->count; i++) {

        current_child = (Window*)List_get_at(window->children, i);
        Window_paint(current_child);
    }
}

//This is the default paint method for a new window
void Window_paint_handler(Window* window) {

    //Fill in the window background
    Context_fill_rect(window->context, 0, 0,
                      window->width - (2*WIN_BORDERWIDTH), 
                      window->height - (WIN_TITLEHEIGHT + WIN_BORDERWIDTH), WIN_BGCOLOR);
}

//Used to get a list of windows overlapping the passed window
List* Window_get_windows_above(Window* parent, Window* child) {

    int i;
    Window* current_window;
    List* return_list;

    //Attempt to allocate the output list
    if(!(return_list = List_new()))
        return return_list;

    //We just need to get a list of all items in the
    //child list at higher indexes than the passed window
    //We start by finding the passed child in the list
    for(i = 0; i < parent->children->count; i++)
        if(child == (Window*)List_get_at(parent->children, i))
            break;

    //Now we just need to add the remaining items in the list
    //to the output (IF they overlap, of course)
    //NOTE: As a bonus, this will also automatically fall through
    //if the window wasn't found
    for(; i < parent->children->count; i++) {

        current_window = List_get_at(parent->children, i);

        //Our good old rectangle intersection logic
        if(current_window->x <= (child->x + child->width - 1) &&
		   (current_window->x + current_window->width - 1) >= child->x &&
		   current_window->y <= (child->y + child->height - 1) &&
		   (current_window->y + current_window->height - 1) >= child->y)
            List_add(return_list, current_window); //Insert the overlapping window
    }

    return return_list; 
}

//Interface between windowing system and mouse device
void Window_process_mouse(Window* window, uint16_t mouse_x,
                          uint16_t mouse_y, uint8_t mouse_buttons) {

    int i, inner_x1, inner_y1, inner_x2, inner_y2;
    Window* child;

    //Check to see if the mouse is within the body of a child window
    if(!window->drag_child) { 

        for(i = window->children->count - 1; i >= 0; i--) {

            child = (Window*)List_get_at(window->children, i);

            //Get the window bounds
            inner_x1 = child->x;
            inner_y1 = child->y;
            inner_x2 = child->x + child->width;
            inner_y2 = child->y + child->height;

            //With the area to check adjusted, do the actual check
            if(mouse_x >= inner_x1 && mouse_x < inner_x2 &&
               mouse_y >= inner_y1 && mouse_y < inner_y2) {

                //If the child window is decorated, the 'body' of the window
                //excludes the window decorations
                if(!(child->flags & WIN_NODECORATION)) {
                
                    inner_x1 += WIN_BORDERWIDTH;
                    inner_y1 += WIN_TITLEHEIGHT;
                    inner_x2 -= WIN_BORDERWIDTH;
                    inner_y2 -= WIN_BORDERWIDTH;

                    if(!(mouse_x >= inner_x1 && mouse_x < inner_x2 &&
                        mouse_y >= inner_y1 && mouse_y < inner_y2)) 
                        break;
                }

                //Forward the mouse event to the matched child and exit
                Window_process_mouse(child, mouse_x - child->x, mouse_y - child->y, mouse_buttons);
                return;
            }
        }
    }


    //Check to see if mouse button has been depressed since last mouse update
    if(mouse_buttons) {
        
        //Events for mouse up -> down transition
        if(!window->last_button_state) {

            //If we had a button depressed, then we need to see if the mouse was
            //over any of the child windows
            //We go front-to-back in terms of the window stack for free occlusion
            for(i = window->children->count - 1; i >= 0; i--) {

                child = (Window*)List_get_at(window->children, i);

                //See if the mouse position lies within the bounds of the current
                //window's 31 px tall titlebar
                if(!(child->flags & WIN_NODECORATION) && //Can't drag a window without a titlebar
                   mouse_x >= child->x && mouse_x < (child->x + child->width) &&
                   mouse_y >= child->y && mouse_y < (child->y + 31)) {

                    //The mouse was over this window when the mouse was pressed, so
                    //we need to raise it
                    List_remove_at(window->children, i); //Pull window out of list
                    List_add(window->children, (void*)child); //Insert at the top 

                    //We'll also set this window as the window being dragged
                    //until such a time as the mouse is released
                    window->drag_off_x = mouse_x - child->x;
                    window->drag_off_y = mouse_y - child->y;
                    window->drag_child = child;

                    //Since we hit a window, we can stop looking
                    break;
                }
            }
        } 
    } else {

        //If the mouse is not down, we need to make sure our drag status is cleared
        window->drag_child = (Window*)0;
    }

    //Update drag window to match the mouse if we have an active drag window
    if(window->drag_child) {

        window->drag_child->x = mouse_x - window->drag_off_x;
        window->drag_child->y = mouse_y - window->drag_off_y;
    }

    //Update the stored mouse button state to match the current state 
    window->last_button_state = mouse_buttons;
}

//A method to automatically create a new window in the provided parent window
Window* Window_create_window(Window* window, int16_t x, int16_t y,  
                             uint16_t width, int16_t height, uint16_t flags) {

    //Attempt to create the window instance
    Window* new_window;
    if(!(new_window = Window_new(x, y, width, height, flags, window->context)))
        return new_window;

    //Attempt to add the window to the end of the parent's children list
    //If we fail, make sure to clean up all of our allocations so far 
    if(!List_add(window->children, (void*)new_window)) {

        free(new_window);
        return (Window*)0;
    }

    //Set the new child's parent 
    new_window->parent = window;

    return new_window;
}
