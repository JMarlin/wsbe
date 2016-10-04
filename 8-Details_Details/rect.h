#ifndef RECT_H
#define RECT_H

#include "list.h"
#include <stdlib.h>

//================| Rect Class Declaration |================//

typedef struct Rect_struct {
    int top;
    int left;
    int bottom;
    int right;
} Rect;

//Method declarations
Rect* Rect_new(int top, int left, int bottom, int right);
List* Rect_split(Rect* subject_rect, Rect* cutting_rect);
Rect* Rect_intersect(Rect* rect_a, Rect* rect_b);

#endif //RECT_H