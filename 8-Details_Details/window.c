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
    window->mousedown_function = Window_mousedown_handler;
  
    return 1;
}

//Recursively get the absolute on-screen x-coordinate of this window
int Window_screen_x(Window* window) {

    if(window->parent)
        return window->x + Window_screen_x(window->parent);
    
    return window->x;
}

//Recursively get the absolute on-screen y-coordinate of this window
int Window_screen_y(Window* window) {

    if(window->parent)
        return window->y + Window_screen_y(window->parent);
    
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
void Window_apply_bound_clipping(Window* window, int in_recursion, List* dirty_regions) {

    Rect* temp_rect, current_dirty_rect, clone_dirty_rect;
    int screen_x, screen_y, i;
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
    //Here's our change: If we were passed a dirty region list, we first
    //clone those dirty rects into the clipping region and then intersect
    //the top-level window bounds against it so that we're limited to the
    //dirty region from the outset
    if(!window->parent) {

        if(dirty_regions) {

            //Clone the dirty regions and put them into the clipping list
            for(i = 0; i < dirty_regions->count; i++) {
            
                //Clone
                current_dirty_rect = (Rect*)List_get_at(dirty_regions, i);
                clone_dirty_rect = Rect_new(current_dirty_rect->top,
                                            current_dirty_rect->left,
                                            current_dirty_rect->bottom,
                                            current_dirty_rect->right);
                
                //Add
                Context_add_clip_rect(window->context, clone_dirty_rect);
            }

            //Finally, intersect this top level window against them
            Context_intersect_clip_rect(window->context, temp_rect);
        } else {

            Context_add_clip_rect(window->context, temp_rect);
        }

        return;
    }

    //Otherwise, we first reduce our clipping area to the visibility area of our parent
    Window_apply_bound_clipping(window->parent, 1, dirty_regions);

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
void Window_paint(Window* window, List* dirty_regions, uint8_t paint_children) {

    int i, j, screen_x, screen_y, child_screen_x, child_screen_y;
    Window* current_child;
    Rect* temp_rect;

    //Start by limiting painting to the window's visible area
    Window_apply_bound_clipping(window, 0, dirty_regions);

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
    
    //Even though we're no longer having all mouse events cause a redraw from the desktop
    //down, we still need to call paint on our children in the case that we were called with
    //a dirty region list since each window needs to be responsible for recursively checking
    //if its children were dirtied 
    if(!paint_children)
        return;

    for(i = 0; i < window->children->count; i++) {

        current_child = (Window*)List_get_at(window->children, i);

        if(dirty_regions) {

            //Check to see if the child is affected by any of the
            //dirty region rectangles
            for(j = 0; j < dirty_regions->count; j++) {
            
                temp_rect = (Rect*)List_get_at(dirty_regions, j);
                
                if(temp_rect->left <= (current_child->x + current_child->width - 1) &&
                temp_rect->right >= current_child->x &&
                temp_rect->top <= (current_child->y + current_child->height - 1) &&
                temp_rect->bottom >= current_child->y)
                    break;
            }

            //Skip drawing this child if no intersection was found
            if(j == dirty_regions->count)
                continue;
        }

        //Otherwise, recursively request the child to redraw its dirty areas
        Window_paint(current_child, dirty_regions, 1);
    }
}

//This is the default paint method for a new window
void Window_paint_handler(Window* window) {

    //Fill in the window background
    Context_fill_rect(window->context, 0, 0,
                      window->width, window->height, WIN_BGCOLOR);
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

//We're wrapping this guy so that we can handle any needed redraw
void Window_move(Window* window, int new_x, int new_y) {

    int i;
    int old_x = window->x;
    int old_y = window->y;
    Rect new_window_rect;
    List *replacement_list, *dirty_list, *dirty_windows;

    //To make life a little bit easier, we'll make the not-unreasonable 
    //rule that if a window is moved, it must become the top-most window
    Window_raise(window, 0); //Raise it, but don't repaint it yet

    //We'll hijack our dirty rect collection from our existing clipping operations
    //So, first we'll get the visible regions of the original window position
    Window_apply_bound_clipping(window, 0, (List*)0);

    //Temporarily update the window position
    window->x = new_x;
    window->y = new_y;

    //Calculate the new bounds
    new_window_rect.top = Window_screen_y(window);
    new_window_rect.left = Window_screen_x(window);
    new_window_rect.bottom = new_window_rect.top + window->height - 1;
    new_window_rect.right = new_window_rect.left + window->width - 1;

    //Reset the window position
    window->x = old_x;
    window->y = old_y;

    //Now, we'll get the *actual* dirty area by subtracting the new location of
    //the window 
    Context_subtract_clip_rect(window->context, &new_window_rect);

    //Now that the context clipping tools made the list of dirty rects for us,
    //we can go ahead and steal the list it made for our own purposes
    //(yes, it would be cleaner to spin off our boolean rect functions so that
    //they can be used both here and by the clipping region tools, but I ain't 
    //got time for that junk)
    if(!(replacement_list = List_new())) {

        Context_clear_clip_rects(window->context);
        return;
    }

    dirty_list = window->context->clip_rects;
    window->context->clip_rects = replacement_list;

    //Now, let's get all of the siblings that we overlap before the move
    dirty_windows = Window_get_windows_below(window);

    //And we'll repaint all of them using the dirty rects
    //(removing them from the list as we go for convenience)
    while(dirty_windows->count)
        Window_paint((Window*)List_remove_at(dirty_windows, 0), dirty_list, 1);

    //The one thing that might still be dirty is the parent we're inside of
    Window_paint(window->parent, dirty_list, 0)

    //We're done with the lists, so we can dump them
    while(dirty_list->count)
        free(List_remove_at(dirty_list, 0));

    free(dirty_list);
    free(dirty_windows);

    //With the dirtied siblings redrawn, we can do the final update of 
    //the window location and paint it at that new position
    window->x = new_x;
    window->y = new_y;
    Window_paint(window, (List*)0, 1);
}

//Used to get a list of windows which the passed window overlaps
//Same exact thing as get_windows_above, but goes backwards through
//the list. Could probably be made a little less redundant if you really wanted
List* Window_get_windows_below(Window* parent, Window* child) {

    int i;
    Window* current_window;
    List* return_list;

    //Attempt to allocate the output list
    if(!(return_list = List_new()))
        return return_list;

    //We just need to get a list of all items in the
    //child list at higher indexes than the passed window
    //We start by finding the passed child in the list
    for(i = parent->children->count - 1; i > -1; i--)
        if(child == (Window*)List_get_at(parent->children, i))
            break;

    //Now we just need to add the remaining items in the list
    //to the output (IF they overlap, of course)
    //NOTE: As a bonus, this will also automatically fall through
    //if the window wasn't found
    for(; i > -1; i--) {

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

    //If we had a button depressed, then we need to see if the mouse was
    //over any of the child windows
    //We go front-to-back in terms of the window stack for free occlusion
    for(i = window->children->count - 1; i >= 0; i--) {

        child = (Window*)List_get_at(window->children, i);

        //If mouse isn't window bounds, we can't possibly be interacting with it 
        if(!(mouse_x >= child->x && mouse_x < (child->x + child->width) &&
           mouse_y >= child->y && mouse_y < (child->y + child->height))) 
            continue;

        //Now we'll check to see if we're dragging a titlebar
        if(mouse_buttons && !window->last_button_state) {

            //Let's adjust things so that a raise happens whenever we click inside a 
            //child, to be more consistent with most other GUIs
            List_remove_at(window->children, i); //Pull window out of list
            List_add(window->children, (void*)child); //Insert at the top

            //See if the mouse position lies within the bounds of the current
            //window's 31 px tall titlebar
            //We check the decoration flag since we can't drag a window without a titlebar
            if(!(child->flags & WIN_NODECORATION) && 
                mouse_y >= child->y && mouse_y < (child->y + 31)) {

                //We'll also set this window as the window being dragged
                //until such a time as the mouse is released
                window->drag_off_x = mouse_x - child->x;
                window->drag_off_y = mouse_y - child->y;
                window->drag_child = child;
                
                //We break without setting target_child if we're doing a drag since
                //that shouldn't trigger a mouse event in the child 
                break;
            }
        }

        //Found a target, so forward the mouse event to that window and quit looking
        Window_process_mouse(child, mouse_x - child->x, mouse_y - child->y, mouse_buttons); 
        break;
    }

    //Moving this outside of the mouse-in-child detection since it doesn't really
    //have anything to do with it
    if(!mouse_buttons)
        window->drag_child = (Window*)0;

    //Update drag window to match the mouse if we have an active drag window
    if(window->drag_child) {

        window->drag_child->x = mouse_x - window->drag_off_x;
        window->drag_child->y = mouse_y - window->drag_off_y;
    }

    //If we didn't find a target in the search, then we ourselves are the target of any clicks
    if(window->mousedown_function && mouse_buttons && !window->last_button_state) 
        window->mousedown_function(window, mouse_x, mouse_y);

    //Update the stored mouse button state to match the current state 
    window->last_button_state = mouse_buttons;
}

//The default handler for window mouse events doesn't do anything
void Window_mousedown_handler(Window* window, int x, int y) {
 
    return;
}

//Quick wrapper for shoving a new entry into the child list
void Window_insert_child(Window* window, Window* child) {

    child->parent = window;
    child->context = window->context;
    List_add(window->children, child);
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
