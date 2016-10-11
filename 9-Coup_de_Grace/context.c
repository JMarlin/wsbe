#include <inttypes.h>
#include "context.h"
#include "rect.h"
#include "font.h"


//================| Context Class Implementation |================//

//Constructor for our context
Context* Context_new(uint16_t width, uint16_t height, uint32_t* buffer) {

    //Attempt to allocate
    Context* context;
    if(!(context = (Context*)malloc(sizeof(Context))))
        return context; 

    //Attempt to allocate new rect list 
    if(!(context->clip_rects = List_new())) {

        free(context);
        return (Context*)0;
    }

    //Finish assignments
    context->width = width; 
    context->height = height; 
    context->buffer = buffer;
    context->clipping_on = 0;

    return context;
}

void Context_clipped_rect(Context* context, int x, int y, unsigned int width,
                          unsigned int height, Rect* clip_area, uint32_t color) {

    int cur_x;
    int max_x = x + width;
    int max_y = y + height;

    //Translate the rectangle coordinates by the context translation values
    x += context->translate_x;
    y += context->translate_y;
    max_x += context->translate_x;
    max_y += context->translate_y;

    //Make sure we don't go outside of the clip region:
    if(x < clip_area->left)
        x = clip_area->left;
    
    if(y < clip_area->top)
        y = clip_area->top;

    if(max_x > clip_area->right + 1)
        max_x = clip_area->right + 1;

    if(max_y > clip_area->bottom + 1)
        max_y = clip_area->bottom + 1;

    //Draw the rectangle into the framebuffer line-by line
    //(bonus points if you write an assembly routine to do it faster)
    for(; y < max_y; y++) 
        for(cur_x = x; cur_x < max_x; cur_x++) 
            context->buffer[y * context->width + cur_x] = color;
}

//Simple for-loop rectangle into a context
void Context_fill_rect(Context* context, int x, int y,  
                      unsigned int width, unsigned int height, uint32_t color) {

    int max_x = x + width;
    int max_y = y + height;
    int i;
    Rect* clip_area;
    Rect screen_area;

    //Fix from last time: Make sure we don't try to draw offscreen
    if(max_x > context->width)
        max_x = context->width;

    if(max_y > context->height)
        max_y = context->height;
   
    if(x < 0)
        x = 0;
    
    if(y < 0)
        y = 0;
    
    width = max_x - x;
    height = max_y - y;    

    //If there are clipping rects, draw the rect clipped to
    //each of them. Otherwise, draw unclipped (clipped to the screen)
    if(context->clip_rects->count) {
       
        for(i = 0; i < context->clip_rects->count; i++) {    

            clip_area = (Rect*)List_get_at(context->clip_rects, i);
            Context_clipped_rect(context, x, y, width, height, clip_area, color);
        }
    } else {

        if(!context->clipping_on) {

            screen_area.top = 0;
            screen_area.left = 0;
            screen_area.bottom = context->height - 1;
            screen_area.right = context->width - 1;
            Context_clipped_rect(context, x, y, width, height, &screen_area, color);
        }
    }
}

//A horizontal line as a filled rect of height 1
void Context_horizontal_line(Context* context, int x, int y,
                             unsigned int length, uint32_t color) {

    Context_fill_rect(context, x, y, length, 1, color);
}

//A vertical line as a filled rect of width 1
void Context_vertical_line(Context* context, int x, int y,
                           unsigned int length, uint32_t color) {

    Context_fill_rect(context, x, y, 1, length, color);
}

//Rectangle drawing using our horizontal and vertical lines
void Context_draw_rect(Context* context, int x, int y, 
                       unsigned int width, unsigned int height, uint32_t color) {

    Context_horizontal_line(context, x, y, width, color); //top
    Context_vertical_line(context, x, y + 1, height - 2, color); //left 
    Context_horizontal_line(context, x, y + height - 1, width, color); //bottom
    Context_vertical_line(context, x + width - 1, y + 1, height - 2, color); //right
}

//Update the clipping rectangles to only include those areas within both the
//existing clipping region AND the passed Rect
void Context_intersect_clip_rect(Context* context, Rect* rect) {

    int i;
    List* output_rects;
    Rect* current_rect;
    Rect* intersect_rect;
 
    context->clipping_on = 1;

    if(!(output_rects = List_new()))
        return;

    for(i = 0; i < context->clip_rects->count; i++) {

        current_rect = (Rect*)List_get_at(context->clip_rects, i);
        intersect_rect = Rect_intersect(current_rect, rect);

        if(intersect_rect)
            List_add(output_rects, intersect_rect);
    }

    //Delete the original rectangle list
    while(context->clip_rects->count)
        free(List_remove_at(context->clip_rects, 0));
    free(context->clip_rects);

    //And re-point it to the new one we built above
    context->clip_rects = output_rects;

    //Free the input rect
    free(rect);
}

//split all existing clip rectangles against the passed rect
void Context_subtract_clip_rect(Context* context, Rect* subtracted_rect) {

    //Check each item already in the list to see if it overlaps with
    //the new rectangle
    int i, j;
    Rect* cur_rect;
    List* split_rects;

    context->clipping_on = 1;

    for(i = 0; i < context->clip_rects->count; ) {

        cur_rect = List_get_at(context->clip_rects, i);

        //Standard rect intersect test (if no intersect, skip to next)
        //see here for an example of why this works:
        //http://stackoverflow.com/questions/306316/determine-if-two-rectangles-overlap-each-other#tab-top
        if(!(cur_rect->left <= subtracted_rect->right &&
		   cur_rect->right >= subtracted_rect->left &&
		   cur_rect->top <= subtracted_rect->bottom &&
		   cur_rect->bottom >= subtracted_rect->top)) {

            i++;
            continue;
        }

        //If this rectangle does intersect with the new rectangle, 
        //we need to split it
        List_remove_at(context->clip_rects, i); //Original will be replaced w/splits
        split_rects = Rect_split(cur_rect, subtracted_rect); //Do the split
        free(cur_rect); //We can throw this away now, we're done with it

        //Copy the split, non-overlapping result rectangles into the list 
        while(split_rects->count) {

            cur_rect = (Rect*)List_remove_at(split_rects, 0);
            List_add(context->clip_rects, cur_rect);
        }

        //Free the empty split_rect list 
        free(split_rects);

        //Since we removed an item from the list, we need to start counting over again 
        //In this way, we'll only exit this loop once nothing in the list overlaps 
        i = 0;    
    }
}

void Context_add_clip_rect(Context* context, Rect* added_rect) {
    
    Context_subtract_clip_rect(context, added_rect);

    //Now that we have made sure none of the existing rectangles overlap
    //with the new rectangle, we can finally insert it 
    List_add(context->clip_rects, added_rect);
}

//Remove all of the clipping rects from the passed context object
void Context_clear_clip_rects(Context* context) {

    Rect* cur_rect;

    context->clipping_on = 0;

    //Remove and free until the list is empty
    while(context->clip_rects->count) {

        cur_rect = (Rect*)List_remove_at(context->clip_rects, 0);
        free(cur_rect);
    }
}

//Draw a single character with the specified font color at the specified coordinates
void Context_draw_char_clipped(Context* context, char character, int x, int y,
                               uint32_t color, Rect* bound_rect) {

    int font_x, font_y;
    int off_x = 0;
    int off_y = 0;
    int count_x = 8; //Font is 8x12
    int count_y = 12; 
    uint8_t shift_line;

    //Make sure to take context translation into account
    x += context->translate_x;
    y += context->translate_y;

    //Our font only handles the core set of 128 ASCII chars
    character &= 0x7F;

    //Check to see if the character is even inside of this rectangle
    if(x > bound_rect->right || (x + 8) <= bound_rect->left ||
       y > bound_rect->bottom || (y + 12) <= bound_rect->top)
        return;

    //Limit the drawn portion of the character to the interior of the rect
    if(x < bound_rect->left)
        off_x = bound_rect->left - x;        

    if((x + 8) > bound_rect->right)
        count_x = bound_rect->right - x + 1;

    if(y < bound_rect->top)
        off_y = bound_rect->top - y;

    if((y + 12) > bound_rect->bottom)
        count_y = bound_rect->bottom - y + 1;

    //Now we do the actual pixel plotting loop
    for(font_y = off_y; font_y < count_y; font_y++) {

        //Capture the current line of the specified char
        //Just a normal bmp[y * width + x], but in this
        //case we're dealing with an array of 1bpp
        //8-bit-wide character lines
        shift_line = font_array[font_y * 128 + character];

        //Pre-shift the line by the x-offset
        shift_line <<= off_x;

        for(font_x = off_x; font_x < count_x; font_x++) {

            //Get the current leftmost bit of the current 
            //line of the character and, if it's set, plot a pixel
            if(shift_line & 0x80)
                context->buffer[(font_y + y) * context->width + (font_x + x)] = color;
 
            //Shift in the next bit
            shift_line <<= 1; 
        }
    }
}

//This will be a lot like Context_fill_rect, but on a bitmap font character
void Context_draw_char(Context* context, char character, int x, int y, uint32_t color) {

    int i;
    Rect* clip_area;
    Rect screen_area;

    //If there are clipping rects, draw the character clipped to
    //each of them. Otherwise, draw unclipped (clipped to the screen)
    if(context->clip_rects->count) {
       
        for(i = 0; i < context->clip_rects->count; i++) {    

            clip_area = (Rect*)List_get_at(context->clip_rects, i);
            Context_draw_char_clipped(context, character, x, y, color, clip_area);
        }
    } else {

        if(!context->clipping_on) {

            screen_area.top = 0;
            screen_area.left = 0;
            screen_area.bottom = context->height - 1;
            screen_area.right = context->width - 1;
            Context_draw_char_clipped(context, character, x, y, color, clip_area);
        }
    }
}

//Draw a line of text with the specified font color at the specified coordinates
void Context_draw_text(Context* context, char* string, int x, int y, uint32_t color) {

    for( ; *string; x += 8)
        Context_draw_char(context, *(string++), x, y, color);
}