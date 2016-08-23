#include "../fake_lib/fake_os.h"
#include <inttypes.h>
#include <stdlib.h>
#include <emscripten.h>

//================| ListNode Class |================//

//A type to encapsulate an individual item in a linked list
typedef struct ListNode_struct {
    void* payload;
    struct ListNode_struct* prev;
    struct ListNode_struct* next;
} ListNode;

//Basic listnode constructor
ListNode* ListNode_new(void* payload) {

    //Malloc and/or fail null
    ListNode* list_node;
    if(!(list_node = (ListNode*)malloc(sizeof(ListNode))))
        return list_node;

    //Assign initial properties
    list_node->prev = (ListNode*)0;
    list_node->next = (ListNode*)0;
    list_node->payload = payload; 

    return list_node;
}


//================| List Class |================//

//A type to encapsulate a basic dynamic list
typedef struct List_struct {
    unsigned int count; //All we know for now is that there will be a number of items
    ListNode* root_node;
} List;

//Basic list constructor
List* List_new() {
    
    //Malloc and/or fail null
    List* list;
    if(!(list = (List*)malloc(sizeof(List))))
        return list;

    //Fill in initial property values
    //(All we know for now is that we start out with no items) 
    list->count = 0;
    list->root_node = (ListNode*)0;

    return list;
}

//Insert a payload at the end of the list
//Zero is fail, one is success
int List_add(List* list, void* payload) {

    //Try to make a new node, exit early on fail 
    ListNode* new_node;
    if(!(new_node = ListNode_new(payload))) 
        return 0;

    //If there aren't any items in the list yet, assign the
    //new item to the root node
    if(!list->root_node) {
 
        list->root_node = new_node;        
    } else {

        //Otherwise, we'll find the last node and add our new node after it
        ListNode* current_node = list->root_node;

        //Fast forward to the end of the list 
        while(current_node->next)
            current_node = current_node->next;

        //Make the last node and first node point to each other
        current_node->next = new_node;
        new_node->prev = current_node; 
    }

    //Update the number of items in the list and return success
    list->count++;

    return 1;
}

//Get the payload of the list item at the given index
//Indices are zero-based
void* List_get_at(List* list, unsigned int index) {

    //If there's nothing in the list or we're requesting beyond the end of
    //the list, return nothing
    if(list->count == 0 || index >= list->count) 
        return (void*)0;

    //Iterate through the items in the list until we hit our index
    ListNode* current_node = list->root_node;

    //Iteration, making sure we don't hang on malformed lists
    for(unsigned int current_index = 0; (current_index < index) && current_node; current_index++)
        current_node = current_node->next;

    //Return the payload, guarding against malformed lists
    return current_node ? current_node->payload : (void*)0;
}


//================| Context Class |================//

//A structure for holding information about a framebuffer
typedef struct Context_struct {  
    uint32_t* buffer; //A pointer to our framebuffer
    uint16_t width; //The dimensions of the framebuffer
    uint16_t height; 
} Context;

//Simple for-loop rectangle into a context
void Context_fillRect(Context* context, unsigned int x, unsigned int y,  
                      unsigned int width, unsigned int height, uint32_t color) {

    unsigned int cur_x;
    unsigned int max_x = x + width;
    unsigned int max_y = y + height;

    //Make sure we don't go outside of the framebuffer:
    if(max_x > context->width)
        max_x = context->width;    

    if(max_y > context->height)
        max_y = context->height;

    //Draw the rectangle into the framebuffer line-by line
    //(bonus points if you write an assembly routine to do it faster)
    for( ; y < max_y; y++) 
        for(cur_x = x ; cur_x < max_x; cur_x++) 
            context->buffer[y * context->width + cur_x] = color;
}


//================| Window Class |================//

typedef struct Window_struct {  
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    Context* context;
} Window;

//Window constructor
Window* Window_new(unsigned int x, unsigned int y,  
                   unsigned int width, unsigned int height, Context* context) {

    //Try to allocate space for a new WindowObj and fail through if malloc fails
    Window* window;
    if(!(window = (Window*)malloc(sizeof(Window))))
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
void Window_paint(Window* window) {

    uint32_t fill_color = 0xFF000000 |            //Opacity
                          pseudo_rand_8() << 16 | //B
                          pseudo_rand_8() << 8  | //G
                          pseudo_rand_8();        //R

    Context_fillRect(window->context, window->x, window->y,
                     window->width, window->height, fill_color);
}


//================| Desktop Class |================//

typedef struct Desktop_struct {
    List* children;
    Context* context;
} Desktop;

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
