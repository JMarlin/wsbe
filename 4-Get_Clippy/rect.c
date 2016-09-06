#include "rect.h"

//================| Rect Class Implementation |================//

//Allocate a new rectangle object
Rect* Rect_new(int top, int left, int bottom, int right) {

    //Attempt to allocate the object
    Rect* rect;
    if(!(rect = (Rect*)malloc(sizeof(Rect))))
        return rect;

    //Assign intial values
    rect->top = top;
    rect->left = left;
    rect->bottom = bottom;
    rect->right = right;

    return rect;
}

//Explode subject_rect into a list of contiguous rects which are
//not occluded by cutting_rect
// ________                ____ ___
//|s    ___|____          |o   |o__|____
//|____|___|   c|   --->  |____|       c|   
//     |________|              |________|
List* Rect_split(Rect* subject_rect, Rect* cutting_rect) {

    //Allocate the list of result rectangles
    List* output_rects;
    if(!(output_rects = List_new()))
        return output_rects;

    //We're going to modify the subject rect as we go,
    //so we'll clone it so as to not upset the object 
    //we were passed
    Rect subject_copy;
    subject_copy.top = subject_rect->top;
    subject_copy.left = subject_rect->left;
    subject_copy.bottom = subject_rect->bottom;
    subject_copy.right = subject_rect->right;

    //We need a rectangle to hold new rectangles before
    //they get pushed into the output list
    Rect* temp_rect;

    //Begin splitting
    //1 -Split by left edge if that edge is between the subject's left and right edges 
    if(cutting_rect->left >= subject_copy.left && cutting_rect->left <= subject_copy.right) {

        //Try to make a new rectangle spanning from the subject rectangle's left and stopping before 
        //the cutting rectangle's left
        if(!(temp_rect = Rect_new(subject_copy.top, subject_copy.left,
                                  subject_copy.bottom, cutting_rect->left - 1))) {

            //If the object creation failed, we need to delete the list and exit failed
            free(output_rects);

            return (List*)0;
        }

        //Add the new rectangle to the output list
        List_add(output_rects, temp_rect);

        //Shrink the subject rectangle to exclude the split portion
        subject_copy.left = cutting_rect->left;
    }

    //2 -Split by top edge if that edge is between the subject's top and bottom edges 
    if(cutting_rect->top >= subject_copy.top && cutting_rect->top <= subject_copy.bottom) {

        //Try to make a new rectangle spanning from the subject rectangle's top and stopping before 
        //the cutting rectangle's top
        if(!(temp_rect = Rect_new(subject_copy.top, subject_copy.left,
                                  cutting_rect->top - 1, subject_copy.right))) {

            //If the object creation failed, we need to delete the list and exit failed
            //This time, also delete any previously allocated rectangles
            for(; output_rects->count; temp_rect = List_remove_at(output_rects, 0))
                free(temp_rect);

            free(output_rects);

            return (List*)0;
        }

        //Add the new rectangle to the output list
        List_add(output_rects, temp_rect);

        //Shrink the subject rectangle to exclude the split portion
        subject_copy.top = cutting_rect->top;
    }

    //3 -Split by right edge if that edge is between the subject's left and right edges 
    if(cutting_rect->right >= subject_copy.left && cutting_rect->right <= subject_copy.right) {

        //Try to make a new rectangle spanning from the subject rectangle's right and stopping before 
        //the cutting rectangle's right
        if(!(temp_rect = Rect_new(subject_copy.top, cutting_rect->right + 1,
                                  subject_copy.bottom, subject_copy.right))) {

            //Free on fail
            for(; output_rects->count; temp_rect = List_remove_at(output_rects, 0))
                free(temp_rect);

            free(output_rects);

            return (List*)0;
        }

        //Add the new rectangle to the output list
        List_add(output_rects, temp_rect);

        //Shrink the subject rectangle to exclude the split portion
        subject_copy.right = cutting_rect->right;
    }

    //4 -Split by bottom edge if that edge is between the subject's top and bottom edges 
    if(cutting_rect->bottom >= subject_copy.top && cutting_rect->bottom <= subject_copy.bottom) {

        //Try to make a new rectangle spanning from the subject rectangle's bottom and stopping before 
        //the cutting rectangle's bottom
        if(!(temp_rect = Rect_new(cutting_rect->bottom + 1, subject_copy.left,
                                  subject_copy.bottom, subject_copy.right))) {

            //Free on fail
            for(; output_rects->count; temp_rect = List_remove_at(output_rects, 0))
                free(temp_rect);

            free(output_rects);

            return (List*)0;
        }

        //Add the new rectangle to the output list
        List_add(output_rects, temp_rect);

        //Shrink the subject rectangle to exclude the split portion
        subject_copy.bottom = cutting_rect->bottom;
    }

    //Finally, after all that, we can return the output rectangles 
    return output_rects;
}
