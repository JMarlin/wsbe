#ifndef DESKTOP_H
#define DESKTOP_H

#include "list.h"
#include "context.h"
#include "window.h"


//================| Desktop Class Declaration |================//

typedef struct Desktop_struct {
    List* children;
    Context* context;
} Desktop;

//Methods
Desktop* Desktop_new(Context* context);
Window* Desktop_create_window(Desktop* desktop, unsigned int x, unsigned int y,  
                          unsigned int width, unsigned int height);
void Desktop_paint(Desktop* desktop);

#endif //DESKTOP_H
