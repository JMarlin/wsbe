---
layout: post
title:  "9 - Coup de Grace"
author: "Joe Marlin"
---

<p>Okay, we're at the end. Though it's not perfect, and you can always find a reason to add a feature to something, all we really have left to cover are a few flourishy details and a demo of how all this junk works. Today is the day, though. After this, you're on your own. But I think you'll have picked up enough tools by the end of all this that that should be more exciting than terrifying.</p>

<p>We have a lot to write today, so let's just dive right in: </p>

<h2 id="prettymouse">Pretty Mouse</h2>

<p>I think I can almost hear a collective audible sigh of relief on this one. This is an easy get that I really just haven't considered important enough to handle, but I know you want it so let's go ahead and make it happen.</p>

<p>When I said easy, I meant it. All we're going to do is, when we normally do the mouse drawing in <code>Desktop_process_mouse()</code>, use the dirty rect system we implemented last time to request a redraw on the area of the desktop (naturally including any affected children) of the rectangle that the mouse was previously residing in before we moved it. Then we just copy a nice cursor bitmap from an array onto the screen at the new mouse position.</p>

<p>Let's take a look at our cursor data first:</p>

<pre><code class="language-C">//Information for drawing a pretty mouse
#define MOUSE_WIDTH 11
#define MOUSE_HEIGHT 18
#define MOUSE_BUFSZ (MOUSE_WIDTH * MOUSE_HEIGHT)

//Mouse image data
#define cX 0xFF000000 //Black
#define cO 0xFFFFFFFF //White
#define c_ 0x00000000 //Clear

unsigned int mouse_img[MOUSE_BUFSZ] = {  
    cX, c_, c_, c_, c_, c_, c_, c_, c_, c_, c_,
    cX, cX, c_, c_, c_, c_, c_, c_, c_, c_, c_,
    cX, cO, cX, c_, c_, c_, c_, c_, c_, c_, c_,
    cX, cO, cO, cX, c_, c_, c_, c_, c_, c_, c_,
    cX, cO, cO, cO, cX, c_, c_ ,c_, c_, c_, c_,
    cX, cO, cO, cO, cO, cX, c_, c_, c_, c_, c_,
    cX, cO, cO, cO, cO, cO, cX, c_, c_, c_, c_,
    cX, cO, cO, cO, cO, cO, cO, cX, c_, c_, c_,
    cX, cO, cO, cO, cO, cO, cO, cO, cX, c_, c_,
    cX, cO, cO, cO, cO, cO, cO, cO, cO, cX, c_,
    cX, cO, cO, cO, cO, cO, cO, cO, cO, cO, cX,
    cX, cX, cX, cX, cO, cO, cO, cX, cX, cX, cX,
    c_, c_, c_, c_, cX, cO, cO, cX, c_, c_, c_,
    c_, c_, c_, c_, cX, cO, cO, cX, c_, c_, c_,
    c_, c_, c_, c_, c_, cX, cO, cO, cX, c_, c_,
    c_, c_, c_, c_, c_, cX, cO, cO, cX, c_, c_,
    c_, c_, c_, c_, c_, c_, cX, cO, cX, c_, c_,
    c_, c_, c_, c_, c_, c_, c_, cX, cX, c_, c_ 
};
</code></pre>

<p>&nbsp;</p>

<p>The defines are probably a little ill-advised or at least gross, but I think it helps you to see what's going on in our bitmap. Like everything else so far, this is going to assume that this pixel data is going to match our raw 32-bit framebuffer pixel format, so you may need to adjust things accordingly. But, really, at the end of the day I just threw something together in MS paint and then, once I was happy, hand-wrote what I had drawn into an array. Not too crazy. Now let's get into <code>Desktop_process_mouse()</code> and put that thing on the screen:</p>

<pre><code class="language-C">//Our overload of the Window_process_mouse function used to capture the screen mouse position 
void Desktop_process_mouse(Desktop* desktop, uint16_t mouse_x,  
                           uint16_t mouse_y, uint8_t mouse_buttons) {

    int i, x, y;
    Window* child;
    List* dirty_list;
    Rect* mouse_rect;

    //Do the old generic mouse handling
    Window_process_mouse((Window*)desktop, mouse_x, mouse_y, mouse_buttons);

    //With that done, we dive into drawing the new mouse
    //First, we build a dirty rect list for the mouse area
    if(!(dirty_list = List_new()))
        return;

    if(!(mouse_rect = Rect_new(desktop-&gt;mouse_y, desktop-&gt;mouse_x, 
                               desktop-&gt;mouse_y + MOUSE_HEIGHT - 1,
                               desktop-&gt;mouse_x + MOUSE_WIDTH - 1))) {

        free(dirty_list);
        return;
    }

    List_add(dirty_list, mouse_rect);

    //Do a dirty update for the desktop, which will, in turn, do a 
    //dirty update for all affected child windows
    Window_paint((Window*)desktop, dirty_list, 1); 

    //Clean up mouse dirty list
    List_remove_at(dirty_list, 0);
    free(dirty_list);
    free(mouse_rect);

    //Update mouse position
    desktop-&gt;mouse_x = mouse_x;
    desktop-&gt;mouse_y = mouse_y;

    //No more hacky mouse, instead we're going to rather inefficiently 
    //copy the pixels from our mouse image into the framebuffer
    //If you decided to write a blit command into the context later, you could really
    //just replace this whole section with that, because that's all this is
    for(y = 0; y &lt; MOUSE_HEIGHT; y++) {

        //Make sure we don't draw off the bottom of the screen
        if((y + mouse_y) &gt;= desktop-&gt;window.context-&gt;height)
            break;

        for(x = 0; x &lt; MOUSE_WIDTH; x++) {

            //Make sure we don't draw off the right side of the screen
            if((x + mouse_x) &gt;= desktop-&gt;window.context-&gt;width)
                break;

            //Don't place a pixel if it's transparent (still going off of ABGR here,
            //change to suit your palette)
            if(mouse_img[y * MOUSE_WIDTH + x] &amp; 0xFF000000)
                desktop-&gt;window.context-&gt;buffer[(y + mouse_y)
                                                * desktop-&gt;window.context-&gt;width 
                                                + (x + mouse_x)
                                               ] = mouse_img[y * MOUSE_WIDTH + x];
        }
    }
}
</code></pre>

<p>&nbsp;</p>

<p>Compile it. Play around with it. How much of an improvement is that, huh? You know, I guess if we're tuning things up to make this thing look more like an actual GUI, while we're at it we might also want to add a:</p>

<p>&nbsp;</p>

<h2 id="bitmapfont">Bitmap Font</h2>

<p>What? Text? People use that?</p>

<p>Bitmap fonts are a super easy thing, and you probably already know all about them, but I'm going to describe them anyhow. Basically, a bitmap font is what you had before the days of nice true type and other kinds of scalable vector font systems. A bitmap font is exactly what it sounds like: each character is a small bitmap, and basically it just gets copied onto the screen wherever we want to draw our string. Usually it's not copied directly, but it's something like a 1bpp bitmap where 0 means not to draw anything and 1 means to draw a pixel with the current font color. </p>

<p>That's the trivial part. The more complex part is getting it to work nicely with our clipping system. But we'll get to that.</p>

<p>In our specific case, I manually made a 1bpp bitmap in mspaint that was 12px tall and 1024px long and looked a bit like this:</p>

<p><img src="/web/20180715114827im_/http://trackze.ro/content/images/2016/10/font.png" alt=""/></p>

<p>Actually, it looked exactly like that because that is the original image (well, converted to PNG). Anyhow, as you can see, that gives us 128 8px wide character images corresponding to the base 128 ASCII digits. Once I made that, I used <a href="https://web.archive.org/web/20180715114827/https://github.com/JMarlin/P5-Redux/blob/master/P5OSPPB/mods/vesa/imgconvert.c">this simple little bit of C</a> to convert the image into an array of bytes and included it in the code much like this:</p>

<pre><code class="language-C">uint8_t font_array[] = {  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x24, 0x00,
    0x00, 0x00, 0x00, 0x10, 0x08, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    //etcetera...
</code></pre>

<p>(If you're writing directly from the blog and you want that final array in its entirety, you can pull it from <a href="https://web.archive.org/web/20180715114827/https://github.com/JMarlin/wsbe/blob/master/9-Coup_de_Grace/font.h">font.h in the repo</a>.)</p>

<p>Doesn't look like much, but that's just our font bitmap as an array (just like the mouse bitmap array, but this one is 1bpp). But what that basically gives us is an array of chars where each char is one line of one character image (that's the reason we chose 8-pixel-wide characters, just makes it nice and convenient that a line of 8 pixels fits evenly into a byte at 1bpp). That is: <code>font_array[65]</code> is the first line of the image for ASCII char 65, which happens to be capital A. Since it wraps around to the next line of the image for every multiple of the image width like any other bitmap, that also means that <code>font_array[128 + 65]</code> = the second line of capital A, and, more generally, that <code>font_array[128 * i + c]</code> stores the <code>i</code><sup>th</sup> line of the image for ASCII character <code>c</code>. Get it? Got it? Good. </p>

<p>First, let's just write a function that poops a character onto the screen without thinking about clipping, and then we'll get into clipping it next:</p>

<pre><code class="language-C">void Context_draw_char(Context* context, char character, int x, int y, uint32_t color) {

    int font_x, font_y;
    uint8_t shift_line;

    //Make sure to take context translation into account
    x += context-&gt;translate_x;
    y += context-&gt;translate_y;

    //Our font only handles the core set of 128 ASCII chars
    character &amp;= 0x7F;

    //The basic pixel plotting loop
    //Outer for-loops just index an 8x12 area of the font
    for(font_y = 0; font_y &lt; 12; font_y++) { //

        //Capture the current line of the specified char
        //Just a normal bmp[y * width + x], but in this
        //case we're dealing with an array of 1bpp
        //8-bit-wide character lines
        shift_line = font_array[font_y * 128 + character];

        for(font_x = 0; font_x &lt; 8; font_x++) {

            //Now, instead of indexing over x-values, we just shift through the bits
            //from the current line of the character image to find if we need to draw
            //a pixel of the foreground color into the framebuffer or not
            if(shift_line &amp; 0x80)
                context-&gt;buffer[(font_y + y) * context-&gt;width + (font_x + x)] = color;

            //Shift in the next bit of the line
            shift_line &lt;&lt;= 1; 
        }
    }
}
</code></pre>

<p>&nbsp;</p>

<p>I hope that made sense. It's a lot like we're copying an image into the framebuffer, but since we're dealing with 1bpp lines we're checking the individual value of each bit in the line instead of placing it directly into the framebuffer. Anyhow, digest that, because we're about to wrap it in some clipping logic so we can use it in our window paint handlers like any other of our context drawing methods.</p>

<p>This is actually incredibly similar to our basic clipped rectangle drawing. We're going to move our pixel plotting loop into a separate function that plots only those bits of the character that appear within a given rectangle, and then replace the guts of our <code>Context_draw_char()</code> method with a loop that calls this clipped character function for each rectangle currently in the context clipping rectangle collection:</p>

<pre><code class="language-C">//The function that our code will call to draw a character from our bitmap font onto the screen
//This will be a lot like Context_fill_rect, but on a bitmap font character
void Context_draw_char(Context* context, char character, int x, int y, uint32_t color) {

    int i;
    Rect* clip_area;
    Rect screen_area;

    //If there are clipping rects, draw the character clipped to
    //each of them. Otherwise, draw unclipped (clipped to the screen)
    if(context-&gt;clip_rects-&gt;count) {

        for(i = 0; i &lt; context-&gt;clip_rects-&gt;count; i++) {    

            clip_area = (Rect*)List_get_at(context-&gt;clip_rects, i);
            Context_draw_char_clipped(context, character, x, y, color, clip_area);
        }
    } else {

        if(!context-&gt;clipping_on) {

            screen_area.top = 0;
            screen_area.left = 0;
            screen_area.bottom = context-&gt;height - 1;
            screen_area.right = context-&gt;width - 1;
            Context_draw_char_clipped(context, character, x, y, color, clip_area);
        }
    }
}
</code></pre>

<p>&nbsp;</p>

<p>Yup. Pretty much an exact ripoff of <code>Context_fill_rect()</code>. But let's do the guts now, the <code>Context_draw_char_clipped()</code> function that the above calls on to do its actual dirty work:</p>

<pre><code class="language-C">//Draw a single character with the specified font color at the specified coordinates
//and limited to the area of the specified rectangle
void Context_draw_char_clipped(Context* context, char character, int x, int y,  
                               uint32_t color, Rect* bound_rect) {

    int font_x, font_y;
    int off_x = 0;
    int off_y = 0;
    int count_x = 8; //Font is 8x12
    int count_y = 12; 
    uint8_t shift_line;

    //Make sure to take context translation into account
    x += context-&gt;translate_x;
    y += context-&gt;translate_y;

    //Our font only handles the core set of 128 ASCII chars
    character &amp;= 0x7F;

    //Check to see if the character is even inside of this rectangle
    if(x &gt; bound_rect-&gt;right || (x + 8) &lt;= bound_rect-&gt;left ||
       y &gt; bound_rect-&gt;bottom || (y + 12) &lt;= bound_rect-&gt;top)
        return;

    //Limit the drawn portion of the character to the interior of the rect
    //Starting x offset into character image as limited by the left edge of the rect 
    if(x &lt; bound_rect-&gt;left)
        off_x = bound_rect-&gt;left - x;        

    //Width of character image as limited by the right edge of the rect
    if((x + 8) &gt; bound_rect-&gt;right)
        count_x = bound_rect-&gt;right - x + 1;

    //Starting y offset into character image as limited by the top edge of the rect
    if(y &lt; bound_rect-&gt;top)
        off_y = bound_rect-&gt;top - y;

    //Height of character image as limited by the bottom edge of the rect
    if((y + 12) &gt; bound_rect-&gt;bottom)
        count_y = bound_rect-&gt;bottom - y + 1;

    //Now we do the actual pixel plotting loop
    //Note the *slight* changes made to handle the possibly reduced character drawing area
    for(font_y = off_y; font_y &lt; count_y; font_y++) {

        //Capture the current line of the specified char
        //Just a normal bmp[y * width + x], but in this
        //case we're dealing with an array of 1bpp
        //8-bit-wide character lines
        shift_line = font_array[font_y * 128 + character];

        //Pre-shift the line by the x-offset
        shift_line &lt;&lt;= off_x;

        for(font_x = off_x; font_x &lt; count_x; font_x++) {

            //Get the current leftmost bit of the current 
            //line of the character and, if it's set, plot a pixel
            if(shift_line &amp; 0x80)
                context-&gt;buffer[(font_y + y) * context-&gt;width + (font_x + x)] = color;

            //Shift in the next bit
            shift_line &lt;&lt;= 1; 
        }
    }
}
</code></pre>

<p>&nbsp;</p>

<p>Honestly, the above should make sense to you if you understand how our rectangle clipping works. It's just that, in this case, instead of drawing a smaller portion of a rectangle, we're drawing a smaller portion of a character.</p>

<p>Last thing we might want to add is a small utility function to draw a line of text from a string without having to manually draw each character:</p>

<pre><code class="language-C">//Draw a line of text with the specified font color at the specified coordinates
void Context_draw_text(Context* context, char* string, int x, int y, uint32_t color) {

    for( ; *string; x += 8)
        Context_draw_char(context, *(string++), x, y, color);
}
</code></pre>

<p>&nbsp;</p>

<p>Want to check it out? This was kind of a long section, so I know you want the break and to play around a bit. Let's add a little text to the desktop paint handler so we can see all of our hard work in action:</p>

<pre><code class="language-C">//Paint the desktop 
void Desktop_paint_handler(Window* desktop_window) {

    //Fill the desktop
    Context_fill_rect(desktop_window-&gt;context, 0, 0, desktop_window-&gt;context-&gt;width, 
                      desktop_window-&gt;context-&gt;height, 0xFFFF9933);

    //Draw some test text
    Context_draw_text(desktop_window-&gt;context, "Windowing Systems by Example",
                      0, desktop_window-&gt;height - 12, 0xFFFFFFFF);
}
</code></pre>

<p>&nbsp;</p>

<p>Compile our changes and check out how the string gets nicely drawn at the bottom of the desktop and gets clipped and occluded by other windows just like everything else we draw. Even my derpy little font really spruces everything up.</p>

<p>So now that we can do text, why not use it to draw:</p>

<p>&nbsp;</p>

<h2 id="windowtitles">Window Titles</h2>

<p>Now <em>this</em> is going to make our windows look a lot more like windows! Really, all we have to do is add a string for the title of the window to the <code>Window</code> class, update our border drawing command to draw a string into the titlebar if there's a title assigned to the current window, and write some functions for setting and modifying that title that will make sure things get redrawn nicely if we want to do so. Class property updates first:</p>

<pre><code class="language-C">//Feel free to play with this 'theme'
//(We've added a couple of new colors for active and inactive title text)
#define WIN_BGCOLOR     0xFFBBBBBB //A generic grey
#define WIN_TITLECOLOR  0xFFD09070 //A nice subtle blue
#define WIN_TITLECOLOR_INACTIVE 0xFF908080 //A darker shade 
#define WIN_TEXTCOLOR 0xFFFFE0E0
#define WIN_TEXTCOLOR_INACTIVE 0xFFBBBBBB
#define WIN_BORDERCOLOR 0xFF000000 //Straight-up black
#define WIN_TITLEHEIGHT 31 
#define WIN_BORDERWIDTH 3

typedef struct Window_struct {  
    struct Window_struct* parent;
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    uint16_t flags;
    Context* context;
    struct Window_struct* drag_child;
    struct Window_struct* active_child;
    List* children;
    uint16_t drag_off_x;
    uint16_t drag_off_y;
    uint8_t last_button_state;
    WindowPaintHandler paint_function;
    WindowMousedownHandler mousedown_function;
    char* title; //NEW
} Window;

//And, of course, the tail end of Window_init():

    //Assign the property values
    window-&gt;x = x;
    window-&gt;y = y;
    window-&gt;width = width;
    window-&gt;height = height;
    window-&gt;context = context;
    window-&gt;flags = flags;
    window-&gt;parent = (Window*)0;
    window-&gt;drag_child = (Window*)0;
    window-&gt;drag_off_x = 0;
    window-&gt;drag_off_y = 0;
    window-&gt;last_button_state = 0;
    window-&gt;paint_function = Window_paint_handler;
    window-&gt;mousedown_function = Window_mousedown_handler;
    window-&gt;active_child = (Window*)0;
    window-&gt;title = (char*)0;

    return 1;
}
</code></pre>

<p>&nbsp;</p>

<p>Okay, it's boilerplate, but there's that out of the way. Now, let's update <code>Window_draw_border</code> to actually draw that title. It's as easy as sticking in a call to our new string drawing function:</p>

<pre><code class="language-C">void Window_draw_border(Window* window) {

    int screen_x = Window_screen_x(window);
    int screen_y = Window_screen_y(window);

    //Draw a 3px border around the window[OLD]
    Context_draw_rect(window-&gt;context, screen_x, screen_y,
                      window-&gt;width, window-&gt;height, WIN_BORDERCOLOR);
    Context_draw_rect(window-&gt;context, screen_x + 1, screen_y + 1,
                      window-&gt;width - 2, window-&gt;height - 2, WIN_BORDERCOLOR);
    Context_draw_rect(window-&gt;context, screen_x + 2, screen_y + 2,
                      window-&gt;width - 4, window-&gt;height - 4, WIN_BORDERCOLOR);

    //Draw a 3px border line under the titlebar[OLD]
    Context_horizontal_line(window-&gt;context, screen_x + 3, screen_y + 28,
                            window-&gt;width - 6, WIN_BORDERCOLOR);
    Context_horizontal_line(window-&gt;context, screen_x + 3, screen_y + 29,
                            window-&gt;width - 6, WIN_BORDERCOLOR);
    Context_horizontal_line(window-&gt;context, screen_x + 3, screen_y + 30,
                            window-&gt;width - 6, WIN_BORDERCOLOR);

    //Fill in the titlebar background [OLD]
    Context_fill_rect(window-&gt;context, screen_x + 3, screen_y + 3,
                      window-&gt;width - 6, 25,
                      window-&gt;parent-&gt;active_child == window ? 
                          WIN_TITLECOLOR : WIN_TITLECOLOR_INACTIVE);

    //NEW: Draw the window title
    Context_draw_text(window-&gt;context, window-&gt;title, screen_x + 10, screen_y + 10,
                      window-&gt;parent-&gt;active_child == window ? 
                          WIN_TEXTCOLOR : WIN_TEXTCOLOR_INACTIVE);
}
</code></pre>

<p>&nbsp;</p>

<p>And, really, we're pretty much done with implementing window titles. With the one exception that it would be fairly nice to be able to set them. We could definitely just do <code>window-&gt;title = "Window";</code>, but I'm going to make it a little more robust than that. For one, I want to be able to hook a border redraw as soon as a window changes its title text so that our display is up-to-date. But I'm also going to clone the string passed to the set function into the heap so that the function doesn't have to worry about whether the passed string is a literal or a heap value or whatever. Just a preference thing. Anyway:</p>

<pre><code class="language-C">//Assign a string to the title of the window
void Window_set_title(Window* window, char* new_title) {

    int len, i;

    //Make sure to free any preexisting title 
    if(window-&gt;title) {

        for(len = 0; window-&gt;title[len]; len++);
        free(window-&gt;title);
    }

    //We don't have strlen, so we're doing this manually
    for(len = 0; new_title[len]; len++);

    //Try to allocate new memory to clone the string
    //(+1 because of the trailing zero in a c-string)
    if(!(window-&gt;title = (char*)malloc((len + 1) * sizeof(char))))
        return;

    //Clone the passed string into the window's title
    //Including terminating zero
    for(i = 0; i &lt;= len; i++)
        window-&gt;title[i] = new_title[i];

    //Make sure the change is reflected on-screen
    if(window-&gt;flags &amp; WIN_NODECORATION) //This will make sense when we do button labels next
        Window_invalidate(window, 0, 0, window-&gt;height - 1, window-&gt;width - 1);
    else
        Window_update_title(window);
}
</code></pre>

<p>&nbsp;</p>

<p>Groovy. But before you run that right away, I ask you <em>what if we could extend what we wrote just a little bit to also implement</em>:</p>

<p>&nbsp;</p>

<h2 id="buttonlabels">Button Labels</h2>

<p>Well, we can and we will. That's the answer to that question. Buttons are just undecorated windows, and since they're undecorated they won't ever get a border redraw and, even if they have some set, will never draw any title text. Until we force them to by updating the default button painting method to hijack the title text (this is why we invalidate the control if it's not decorated in <code>Window_set_title()</code> above):</p>

<pre><code class="language-C">void Button_paint(Window* button_window) {

    int title_len; 
    Button* button = (Button*)button_window;

//Old stuff:
    uint32_t border_color;
    if(button-&gt;color_toggle)
        border_color = WIN_TITLECOLOR;
    else
        border_color = WIN_BGCOLOR - 0x101010;

    Context_fill_rect(button_window-&gt;context, 1, 1, button_window-&gt;width - 1,
                      button_window-&gt;height - 1, WIN_BGCOLOR);
    Context_draw_rect(button_window-&gt;context, 0, 0, button_window-&gt;width,
                      button_window-&gt;height, 0xFF000000);
    Context_draw_rect(button_window-&gt;context, 3, 3, button_window-&gt;width - 6,
                      button_window-&gt;height - 6, border_color);
    Context_draw_rect(button_window-&gt;context, 4, 4, button_window-&gt;width - 8,
                      button_window-&gt;height - 8, border_color);    

//New stuff: Just draw the title (centered) if we have one
    //Get the title length
    for(title_len = 0; button_window-&gt;title[title_len]; title_len++);

    //Convert it into pixels
    title_len *= 8;

    //Draw the title centered within the button
    if(button_window-&gt;title)
        Context_draw_text(button_window-&gt;context, button_window-&gt;title,
                          (button_window-&gt;width / 2) - (title_len / 2),
                          (button_window-&gt;height / 2) - 6,
                          WIN_BORDERCOLOR);                                    
}
</code></pre>

<p>&nbsp;</p>

<p>Okay, <em>now</em> I would suggest you add a <code>Window_set_title((Window*)button, "Button");</code> and maybe a <code>Window_set_title(window, "Window");</code> to your <code>main()</code>, compile, and check out how cool things are starting to look.</p>

<p>But we must focus on the prize. We wanted to have a calculator by the end of this series. Which means by the end of this article. Which means one more quick and minor change to the Button class to make building a GUI app feasible by adding:</p>

<p>&nbsp;</p>

<h2 id="buttonevents">Button Events</h2>

<p>This one's quick: When we get to making an application using all of our window and control classes, we're going to want each button click to fire off a function to actually do something. Okay, but don't buttons, as a derivative class of <code>Window</code>, already have an <code>onmousedown</code> handler? Totally fair, and totally true. We <em>could</em> just override that handler to create a special button that does exactly what we want.</p>

<p>But there's a problem: Overriding our button's <code>onmousedown</code> necessarily overrides any default handler such as the one that updates the toggled visual state of a button (and, if you ever wanted to get around to making a slightly less janky 'normal' button by implementing both real mousedown and mouseup window events and using them to make your button look depressed or not, you would also immediately lose that effect by overriding those events to trigger an action in your app).</p>

<p>But that's easily solvable: Let's just give the button class it's own <code>onmousedown</code> handler and call it at the end of the existing one. Then we can set an arbitrary callback when the button gets clicked without losing that default, intrinsic button behavior. So we'll add a new handler to the <code>Button</code> struct:</p>

<pre><code class="language-C">struct Button_struct;

typedef void (*ButtonMousedownHandler)(struct Button_struct*, int, int);

typedef struct Button_struct {  
    Window window;
    uint8_t color_toggle;
    ButtonMousedownHandler onmousedown;
} Button;
</code></pre>

<p>&nbsp;</p>

<p>And then we can slot it into the end of the current <code>Button</code> <code>onmousedown</code> handler:</p>

<pre><code class="language-C">void Button_mousedown_handler(Window* button_window, int x, int y) {

//Old stuff:
    Button* button = (Button*)button_window;

    button-&gt;color_toggle = !button-&gt;color_toggle;

    //Since the button has visibly changed state, we need to invalidate the
    //area that needs updating
    Window_invalidate((Window*)button, 0, 0,
                      button-&gt;window.height - 1, button-&gt;window.width - 1);

//New stuff:
    //Fire the assocaited button click event if it exists
    if(button-&gt;onmousedown)
        button-&gt;onmousedown(button, x, y);
}
</code></pre>

<p>&nbsp;</p>

<p>Now, if we want a button in our app, we can just give it a title like we would with any other window and then attach a custom function to handle the button being clicked without having to do much extra work and without breaking anything. </p>

<p>Well, that's useful. But we can't very well make a GUI app if all we have is input controls. So what say we implement a:</p>

<p>&nbsp;</p>

<h2 id="textboxclass">TextBox Class</h2>

<p>We need this so that our calculator can have some kind of display. Since we're not dealing with keyboard handling, they're going to be output-only (but that's a fun project for you, you'd just add one or more callback handlers to the window class for key events and then, starting at the desktop, send keypresses to the active child's active child's active child's [...] active child).</p>

<p>A lot of this is going to look like a button, but simpler. We won't be hooking the mouse event, so all we're really going to do is inherit from <code>Window</code> and override the paint handler to draw a rectangle with the textbox's current title in it. Let's start with the class definition:</p>

<pre><code class="language-C">//Yet another basically-just-a-window class
typedef struct TextBox_struct {  
    Window window;
} TextBox;
</code></pre>

<p>&nbsp;</p>

<p>I mean, you would probably want to add more down the line. But where we're going we don't need extra properties. So let's do implementation: The custom paint handler and the constructor that assigns it:</p>

<pre><code class="language-C">//The constructor just allocates the memory, inits the basic window stuff, and then 
//overrides the paint method
TextBox* TextBox_new(int x, int y, int width, int height) {

    //Basically the same thing as button init
    TextBox* text_box;
    if(!(text_box = (TextBox*)malloc(sizeof(TextBox))))
        return text_box;

    if(!Window_init((Window*)text_box, x, y, width, height, WIN_NODECORATION, (Context*)0)) {

        free(text_box);
        return (TextBox*)0;
    }

    //Override default window draw callback
    text_box-&gt;window.paint_function = TextBox_paint;

    return text_box;
}

//And then that paint handler just draws the title right-justified into a rectangle
void TextBox_paint(Window* text_box_window) {

    int title_len;

    //White background
    Context_fill_rect(text_box_window-&gt;context, 1, 1, text_box_window-&gt;width - 2,
                      text_box_window-&gt;height - 2, 0xFFFFFFFF);

    //Simple black border
    Context_draw_rect(text_box_window-&gt;context, 0, 0, text_box_window-&gt;width,
                      text_box_window-&gt;height, 0xFF000000);

    //Get the title length
    for(title_len = 0; text_box_window-&gt;title[title_len]; title_len++);

    //Convert it into pixels
    title_len *= 8;

    //Draw the title centered within the button
    if(text_box_window-&gt;title)
        Context_draw_text(text_box_window-&gt;context, text_box_window-&gt;title,
                          text_box_window-&gt;width - title_len - 6,
                          (text_box_window-&gt;height / 2) - 6,
                          0xFF000000);                                    
}
</code></pre>

<p>&nbsp;</p>

<p>I guess it's not so much a textbox at this point as much as it is a label that <em>looks</em> like a textbox, but you're free to modify and extend it as you see fit. For our purposes, this is all we're going to need.</p>

<p>Oh, except this:</p>

<pre><code class="language-C">//Add the characters from the passed string to the end of the window title
void Window_append_title(Window* window, char* additional_chars) {

    char* new_string;
    int original_length, additional_length, i;

    //Just set the title if there isn't already one
    if(!window-&gt;title) {

        Window_set_title(window, additional_chars);
        return;
    }

    //Get the length of the original string
    for(original_length = 0; window-&gt;title[original_length]; original_length++);

    //Get the length of the new string
    for(additional_length = 0; additional_chars[additional_length]; additional_length++);

    //Try to malloc a new string of the needed size
    if(!(new_string = (char*)malloc(sizeof(char) * (original_length + additional_length + 1)))) {
        return;
    }

    //Copy the base string into the new string
    for(i = 0; window-&gt;title[i]; i++)
        new_string[i] = window-&gt;title[i];

    //Copy the appended chars at the end of the new string
    for(i = 0; additional_chars[i]; i++)
        new_string[original_length + i] = additional_chars[i];

    //Add the final zero char
    new_string[original_length + i] = 0;

    //And swap the string pointers
    free(window-&gt;title);
    window-&gt;title = new_string;

    //Make sure the change is reflected on-screen
    if(window-&gt;flags &amp; WIN_NODECORATION)
        Window_invalidate(window, 0, 0, window-&gt;height - 1, window-&gt;width - 1);
    else
        Window_update_title(window); 
}
</code></pre>

<p>&nbsp;</p>

<p>Yeah, a bit of a left turn. But, for our textbox/label, that's going to be really useful to have. Particularly for shoving new digits into the calculator display.</p>

<p>So that's another control out of the way. But, before we get too excited about writing the calculator, we first need to:</p>

<p>&nbsp;</p>

<h2 id="calculatorclass">Calculator Class</h2>

<p>Oh, wait, I guess that's pretty much it. Huh.</p>

<p>Okay,  this one I'm mostly going to just dump on you. There's a lot of code, but not much to describe. We're just going to define a new <code>Window</code>-deriving class with some buttons and a textbox, and then define a function that we'll attach to our buttons to update the display. </p>

<p>Let's define it:</p>

<pre><code class="language-C">typedef struct Calculator_struct {  
    Window window; //'inherit' Window
    TextBox* text_box;
    Button* button_1;
    Button* button_2;
    Button* button_3;
    Button* button_4;
    Button* button_5;
    Button* button_6;
    Button* button_7;
    Button* button_8;
    Button* button_9;
    Button* button_0;
    Button* button_add;
    Button* button_sub;
    Button* button_div;
    Button* button_mul;
    Button* button_ent;
    Button* button_c;
} Calculator;
</code></pre>

<p>&nbsp;</p>

<p>Pretty much what it says on the box. It might seem weird that I'm storing pointers for all of the buttons when they're already going to be in the <code>Calculator</code>'s child list, but I'm saving those extra pointers so that, in the button mousedown handler, I can compare the received button pointer to those values to figure out which button was pressed. There are certainly much more elegant ways to do it -- for instance, I could just give each button its own handler function instead of tying them all to the same one -- but this is how I've done it, so shush.</p>

<p>I guess now that we have a definition, we should go ahead and write a constructor to set it up. Warning: we're initializing sixteen mostly identical buttons so this is a whoooole lot of very dense, repetitive code. This would be why GUI designing tools are a very popular thing that exists:</p>

<pre><code class="language-C">Calculator* Calculator_new(void) {

    Calculator* calculator;

    //Attempt to allocate and initialize the window
    if(!(calculator = (Calculator*)malloc(sizeof(Calculator))))
        return calculator;

    if(!Window_init((Window*)calculator, 0, 0,
                    (2 * WIN_BORDERWIDTH) + 145,
                    WIN_TITLEHEIGHT + WIN_BORDERWIDTH + 170,
                    0, (Context*)0)) {

        free(calculator);
        return (Calculator*)0;
    }

    //Set a default title 
    Window_set_title((Window*)calculator, "Calculator");

    //Create the buttons
    calculator-&gt;button_7 = Button_new(WIN_BORDERWIDTH + 5, WIN_TITLEHEIGHT + 30, 30, 30);
    Window_set_title((Window*)calculator-&gt;button_7, "7");
    Window_insert_child((Window*)calculator, (Window*)calculator-&gt;button_7);

    calculator-&gt;button_8 = Button_new(WIN_BORDERWIDTH + 40, WIN_TITLEHEIGHT + 30, 30, 30);
    Window_set_title((Window*)calculator-&gt;button_8, "8");
    Window_insert_child((Window*)calculator, (Window*)calculator-&gt;button_8);

    calculator-&gt;button_9 = Button_new(WIN_BORDERWIDTH + 75, WIN_TITLEHEIGHT + 30, 30, 30);
    Window_set_title((Window*)calculator-&gt;button_9, "9");
    Window_insert_child((Window*)calculator, (Window*)calculator-&gt;button_9);

    calculator-&gt;button_add = Button_new(WIN_BORDERWIDTH + 110, WIN_TITLEHEIGHT + 30, 30, 30);
    Window_set_title((Window*)calculator-&gt;button_add, "+");
    Window_insert_child((Window*)calculator, (Window*)calculator-&gt;button_add);

    calculator-&gt;button_4 = Button_new(WIN_BORDERWIDTH + 5, WIN_TITLEHEIGHT + 65, 30, 30);
    Window_set_title((Window*)calculator-&gt;button_4, "4");
    Window_insert_child((Window*)calculator, (Window*)calculator-&gt;button_4);

    calculator-&gt;button_5 = Button_new(WIN_BORDERWIDTH + 40, WIN_TITLEHEIGHT + 65, 30, 30);
    Window_set_title((Window*)calculator-&gt;button_5, "5");
    Window_insert_child((Window*)calculator, (Window*)calculator-&gt;button_5);

    calculator-&gt;button_6 = Button_new(WIN_BORDERWIDTH + 75, WIN_TITLEHEIGHT + 65, 30, 30);
    Window_set_title((Window*)calculator-&gt;button_6, "6");
    Window_insert_child((Window*)calculator, (Window*)calculator-&gt;button_6);

    calculator-&gt;button_sub = Button_new(WIN_BORDERWIDTH + 110, WIN_TITLEHEIGHT + 65, 30, 30);
    Window_set_title((Window*)calculator-&gt;button_sub, "-");
    Window_insert_child((Window*)calculator, (Window*)calculator-&gt;button_sub);

    calculator-&gt;button_1 = Button_new(WIN_BORDERWIDTH + 5, WIN_TITLEHEIGHT + 100, 30, 30);
    Window_set_title((Window*)calculator-&gt;button_1, "1");
    Window_insert_child((Window*)calculator, (Window*)calculator-&gt;button_1);

    calculator-&gt;button_2 = Button_new(WIN_BORDERWIDTH + 40, WIN_TITLEHEIGHT + 100, 30, 30);
    Window_set_title((Window*)calculator-&gt;button_2, "2");
    Window_insert_child((Window*)calculator, (Window*)calculator-&gt;button_2);

    calculator-&gt;button_3 = Button_new(WIN_BORDERWIDTH + 75, WIN_TITLEHEIGHT + 100, 30, 30);
    Window_set_title((Window*)calculator-&gt;button_3, "3");
    Window_insert_child((Window*)calculator, (Window*)calculator-&gt;button_3);

    calculator-&gt;button_mul = Button_new(WIN_BORDERWIDTH + 110, WIN_TITLEHEIGHT + 100, 30, 30);
    Window_set_title((Window*)calculator-&gt;button_mul, "*");
    Window_insert_child((Window*)calculator, (Window*)calculator-&gt;button_mul);

    calculator-&gt;button_c = Button_new(WIN_BORDERWIDTH + 5, WIN_TITLEHEIGHT + 135, 30, 30);
    Window_set_title((Window*)calculator-&gt;button_c, "C");
    Window_insert_child((Window*)calculator, (Window*)calculator-&gt;button_c);

    calculator-&gt;button_0 = Button_new(WIN_BORDERWIDTH + 40, WIN_TITLEHEIGHT + 135, 30, 30);
    Window_set_title((Window*)calculator-&gt;button_0, "0");
    Window_insert_child((Window*)calculator, (Window*)calculator-&gt;button_0);

    calculator-&gt;button_ent = Button_new(WIN_BORDERWIDTH + 75, WIN_TITLEHEIGHT + 135, 30, 30);
    Window_set_title((Window*)calculator-&gt;button_ent, "=");
    Window_insert_child((Window*)calculator, (Window*)calculator-&gt;button_ent);

    calculator-&gt;button_div = Button_new(WIN_BORDERWIDTH + 110, WIN_TITLEHEIGHT + 135, 30, 30);
    Window_set_title((Window*)calculator-&gt;button_div, "/");
    Window_insert_child((Window*)calculator, (Window*)calculator-&gt;button_div);

    //We'll use the same handler to handle all of the buttons
    calculator-&gt;button_1-&gt;onmousedown = calculator-&gt;button_2-&gt;onmousedown = 
        calculator-&gt;button_3-&gt;onmousedown = calculator-&gt;button_4-&gt;onmousedown =
        calculator-&gt;button_5-&gt;onmousedown = calculator-&gt;button_6-&gt;onmousedown =
        calculator-&gt;button_7-&gt;onmousedown = calculator-&gt;button_8-&gt;onmousedown =
        calculator-&gt;button_9-&gt;onmousedown = calculator-&gt;button_0-&gt;onmousedown =
        calculator-&gt;button_add-&gt;onmousedown = calculator-&gt;button_sub-&gt;onmousedown = 
        calculator-&gt;button_mul-&gt;onmousedown = calculator-&gt;button_div-&gt;onmousedown =
        calculator-&gt;button_ent-&gt;onmousedown = calculator-&gt;button_c-&gt;onmousedown =
        Calculator_button_handler;          

    //Create the textbox
    calculator-&gt;text_box = TextBox_new(WIN_BORDERWIDTH + 5, WIN_TITLEHEIGHT + 5, 135, 20);
    Window_set_title((Window*)calculator-&gt;text_box, "0");
    Window_insert_child((Window*)calculator, (Window*)calculator-&gt;text_box);

    //Return the finished calculator
    return calculator;
}
</code></pre>

<p>&nbsp;</p>

<p>Blegh. Maybe the next tutorial series will be on writing a layout tool, because that is goddamn fugly. But really straightforward. Just a lot of creating a button, setting the title of the button, and installing the button into the main calculator window.</p>

<p>It would certainly behoove us to write that mousedown handler that every single one of them relies on:</p>

<pre><code class="language-C">void Calculator_button_handler(Button* button, int x, int y) {

    //Get the parent calculator
    Calculator* calculator = (Calculator*)button-&gt;window.parent;

    //If zero was pressed and the display isn't already just "0"
    //then we append a zero to the end of the textbox
    if(button == calculator-&gt;button_0) {

        if(!(calculator-&gt;text_box-&gt;window.title[0] == '0' &amp;&amp;
            calculator-&gt;text_box-&gt;window.title[1] == 0))
            Window_append_title((Window*)calculator-&gt;text_box, "0");
    }

    //If one was pressed, we set the title to '1' if it was just "0"
    //and append a '1' to the title otherwise
    if(button == calculator-&gt;button_1) {

        if(!(calculator-&gt;text_box-&gt;window.title[0] == '0' &amp;&amp;
            calculator-&gt;text_box-&gt;window.title[1] == 0))
            Window_append_title((Window*)calculator-&gt;text_box, "1");
        else
            Window_set_title((Window*)calculator-&gt;text_box, "1");
    }

    //If two was pressed, we set the title to '2' if it was just "0"
    //and append a '2' to the title otherwise
    if(button == calculator-&gt;button_2) {

        if(!(calculator-&gt;text_box-&gt;window.title[0] == '0' &amp;&amp;
            calculator-&gt;text_box-&gt;window.title[1] == 0))
            Window_append_title((Window*)calculator-&gt;text_box, "2");
        else
            Window_set_title((Window*)calculator-&gt;text_box, "2");
    }

    //If three was pressed, we set the title to '3' if it was just "0"
    //and append a '3' to the title otherwise
    if(button == calculator-&gt;button_3) {

        if(!(calculator-&gt;text_box-&gt;window.title[0] == '0' &amp;&amp;
            calculator-&gt;text_box-&gt;window.title[1] == 0))
            Window_append_title((Window*)calculator-&gt;text_box, "3");
        else
            Window_set_title((Window*)calculator-&gt;text_box, "3");
    }

    //Uh...
    if(button == calculator-&gt;button_4) {

        if(!(calculator-&gt;text_box-&gt;window.title[0] == '0' &amp;&amp;
            calculator-&gt;text_box-&gt;window.title[1] == 0))
            Window_append_title((Window*)calculator-&gt;text_box, "4");
        else
            Window_set_title((Window*)calculator-&gt;text_box, "4");
    }

    //Yeah...
    if(button == calculator-&gt;button_5) {

        if(!(calculator-&gt;text_box-&gt;window.title[0] == '0' &amp;&amp;
            calculator-&gt;text_box-&gt;window.title[1] == 0))
            Window_append_title((Window*)calculator-&gt;text_box, "5");
        else
            Window_set_title((Window*)calculator-&gt;text_box, "5");
    }

    //I think you get it.
    if(button == calculator-&gt;button_6) {

        if(!(calculator-&gt;text_box-&gt;window.title[0] == '0' &amp;&amp;
            calculator-&gt;text_box-&gt;window.title[1] == 0))
            Window_append_title((Window*)calculator-&gt;text_box, "6");
        else
            Window_set_title((Window*)calculator-&gt;text_box, "6");
    }

    //...
    if(button == calculator-&gt;button_7) {

        if(!(calculator-&gt;text_box-&gt;window.title[0] == '0' &amp;&amp;
            calculator-&gt;text_box-&gt;window.title[1] == 0))
            Window_append_title((Window*)calculator-&gt;text_box, "7");
        else
            Window_set_title((Window*)calculator-&gt;text_box, "7");
    }

    //Seriously, though
    if(button == calculator-&gt;button_8) {

        if(!(calculator-&gt;text_box-&gt;window.title[0] == '0' &amp;&amp;
            calculator-&gt;text_box-&gt;window.title[1] == 0))
            Window_append_title((Window*)calculator-&gt;text_box, "8");
        else
            Window_set_title((Window*)calculator-&gt;text_box, "8");
    }

    //Whoo, finally.
    if(button == calculator-&gt;button_9) {

        if(!(calculator-&gt;text_box-&gt;window.title[0] == '0' &amp;&amp;
            calculator-&gt;text_box-&gt;window.title[1] == 0))
            Window_append_title((Window*)calculator-&gt;text_box, "9");
        else
            Window_set_title((Window*)calculator-&gt;text_box, "9");
    }

    //The clear button should just reset the textbox back to "0"
    if(button == calculator-&gt;button_c) {
        Window_set_title((Window*)calculator-&gt;text_box, "0");
    }
}
</code></pre>

<p>&nbsp;</p>

<p>Pretty damn fugly, but there you have it, a simple 'calculator'. I'm sure there's a more elegant way to write that big 'ol handler that just uses, like, an array and a for loop to find the number pressed without using basically the same code ten times, but that's for people with more time on their hands than I. </p>

<p>Okay, yeah. Our calculator doesn't calculate. Frankly, I figured this is close enough of a demo. All you'd really have to do is write a string-&gt;int and then an int-&gt;string function and wire it in there, but I thought that would be kind of beside the point. </p>

<p>But that means we're finally done with this crazy long finale, right? Surely! </p>

<p>Ha, nope. We still have to handle some:</p>

<p>&nbsp;</p>

<h2 id="bugfixes">Bugfixes</h2>

<p>First thing: This is going to crash and burn right now for one simple reason (besides the fact that we haven't updated our entry code at all, but shhhh): When we create the buttons for our calculator, they don't have a parent until we install them. But we're setting their titles before we install them. And that wouldn't be a problem, but setting the title forces a redraw on the button/window, and without setting a parent we have an empty context and an empty parent pointer which we currently don't account for when trying to draw a title or a window. So let's account for it in a few spots:</p>

<pre><code class="language-C">void Window_apply_bound_clipping(Window* window, int in_recursion, List* dirty_regions) {

    Rect *temp_rect, *current_dirty_rect, *clone_dirty_rect;
    int screen_x, screen_y, i;
    List* clip_windows;
    Window* clipping_window;

    //Can't do this without a context
    if(!window-&gt;context)
        return;

//And so on...

void Window_update_title(Window* window) {

    int screen_x, screen_y;

    if(!window-&gt;context)
        return;

//And so on...

void Window_paint(Window* window, List* dirty_regions, uint8_t paint_children) {

    int i, j, screen_x, screen_y, child_screen_x, child_screen_y;
    Window* current_child;
    Rect* temp_rect;

    //Can't paint without a context
    if(!window-&gt;context)
        return;

//And so on...
</code></pre>

<p>&nbsp;</p>

<p>Seems good. But there's one more related issue: with the above changes, our calculator buttons and textbox simply won't get drawn. When we create a calculator, we create the calculator window (with no context), create the buttons (with no context), insert the buttons into the calculator window (giving them the window's context -- which is nothing), and then insert the calculator window into the desktop (giving it the desktop's valid context).</p>

<p>So, as you can see, in that scenario the controls are never getting a valid context and so they can't ever get painted. So let's make sure that when we install a window and give it a context, we pass that context on to all of its children:</p>

<pre><code class="language-C">void Window_insert_child(Window* window, Window* child) {

    child-&gt;parent = window;
    List_add(window-&gt;children, child);
    child-&gt;parent-&gt;active_child = child;

    //Call a function that will recursively set the context for all children
    Window_update_context(child, window-&gt;context);
}

//Here's the new function to make it happen:
void Window_update_context(Window* window, Context* context) {

    int i;

    window-&gt;context = context;

    for(i = 0; i &lt; window-&gt;children-&gt;count; i++)
        Window_update_context((Window*)List_get_at(window-&gt;children, i), context);
}
</code></pre>

<p>&nbsp;</p>

<p>Much better. We have just one more nontrivial thing to mention: A bigass memory leak.</p>

<p>So is the nature of C and C++: You have ultimate control over memory management, which also means ultimate control over screwing up your memory management. When I was testing this code, I ran my 'completed' code for a few minutes just to find I was hitting an out of memory error. Meaning something wasn't getting freed when I was done with it.</p>

<p>Long story short, I replaced all of my frees and mallocs with functions that tallied the allocations and deallocations and, after an hour or so, finally tracked things down to this asshole:</p>

<pre><code class="language-C">void Context_intersect_clip_rect(Context* context, Rect* rect) {

    int i;
    List* output_rects;
    Rect* current_rect;
    Rect* intersect_rect;

    context-&gt;clipping_on = 1;

    if(!(output_rects = List_new()))
        return;

    for(i = 0; i &lt; context-&gt;clip_rects-&gt;count; i++) {

        current_rect = (Rect*)List_get_at(context-&gt;clip_rects, i);
        intersect_rect = Rect_intersect(current_rect, rect);

        if(intersect_rect)
            List_add(output_rects, intersect_rect);
    }

    //HERE: Yeah, that List_remote_at didn't have a free on it,
    //so all of the old rectangles were getting removed from the
    //list but were still taking up heap space. Add the free and
    //she runs perfectly clean
    //Delete the original rectangle list
    while(context-&gt;clip_rects-&gt;count)
        free(List_remove_at(context-&gt;clip_rects, 0));
    free(context-&gt;clip_rects);

    context-&gt;clip_rects = output_rects;

    free(rect);
}
</code></pre>

<p>&nbsp;</p>

<p>There are also a couple of other minor, minor changes to the code that will be published on the github, but they shouldn't be breaking so they're not really worth mentioning among the myriad little issues this code already has (hint: probably many off-by-ones, but also a lot of places where I just plain neglected error handling allocations for the sake of brevity).</p>

<p>So I guess that's enough for now. She'll run. That's good enough for me. So let's make it happen by updating our:</p>

<p>&nbsp;</p>

<h2 id="entry">Entry</h2>

<p>Aw man, this is <em>it</em>. I guess you could do whatever you want, but all I'm going to do is put a button on the desktop and attach it to a handler that spawns a new calculator so that we can spawn so many independent calculators that it makes us sick:</p>

<pre><code class="language-C">int main(int argc, char* argv[]) {

    //Fill this in with the info particular to your project
    Context* context = Context_new(0, 0, 0);
    context-&gt;buffer = fake_os_getActiveVesaBuffer(&amp;context-&gt;width, &amp;context-&gt;height);

    //Create the desktop 
    desktop = Desktop_new(context);

    //Create a simple launcher window 
    Button* launch_button = Button_new(10, 10, 150, 30);
    Window_set_title((Window*)launch_button, "New Calculator");
    launch_button-&gt;onmousedown = spawn_calculator;
    Window_insert_child((Window*)desktop, (Window*)launch_button);

    //Initial draw
    Window_paint((Window*)desktop, (List*)0, 1);

    //Install our handler of mouse events
    fake_os_installMouseCallback(main_mouse_callback);

    //Polling alternative:
    //    while(1) {
    //
    //        fake_os_waitForMouseUpdate(&amp;mouse_x, &amp;mouse_y, &amp;buttons);
    //        Desktop_process_mouse(desktop, mouse_x, mouse_y, buttons);
    //    }

    return 0; 
}

//And, finally, the handler that causes that button to make a new calculator
void spawn_calculator(Button* button, int x, int y) {

    //Create and install a calculator
    Calculator* temp_calc = Calculator_new();
    Window_insert_child((Window*)desktop, (Window*)temp_calc);
    Window_move((Window*)temp_calc, 0, 0);
}
</code></pre>

<p><img src="/web/20180715114827im_/http://trackze.ro/content/images/2016/10/download.png" alt=""/></p>

<p>&nbsp;</p>

<p>Woo! Isn't that exciting? It does stuff. It looks like an application. I guess that makes it time to write my final:</p>

<p>```</p>

<p>&nbsp;</p>

<h2 id="summary">Summary</h2>

<p>Whoooooooh. It has been an adventure. And I may be done writing these on a weekly schedule, but I'm not done working on it, I don't know about you. This isn't an end, but a very nice beginning. I've given you a somewhat jenky but fully serviceable framework off of which you could build any kind of desktop system you could imagine -- and one that could totally, with zero changes, run on bare metal at that. </p>

<p>So that single braindead mouse event is kind of hilarious, just like the 'toggle button' we built around it, is pretty lame. But if you think about it for a second, you should be able to come up with a really easy solution to generate 'real' mouse up, down and click events. And maybe you want to implement a 'WIN_NORAISE' flag so that you won't get buttons overlapping windows. That one's also a pretty easy hack away from being yours. Maybe you want your window titles to be on the side of your windows. I mean, seriously, you do you dude.</p>

<p>So I hope you have fun. I hope this code gets forked and played around with. At the very least I hope somebody learned some design practices or good code formatting or got any sort of ideas out of it -- assuming I had any of that. One can hope.</p>

<p>I may be done with this as a weekly series, but who knows. I'm sure I won't stop writing here, so keep checking back if you like what I have to say. And maybe this will come up again in the future (but I don't promise anything).</p>

<p>At the end of the day, I had fun writing these. I hope you had fun reading. Peace.</p>

<hr/>

<p><a href="https://web.archive.org/web/20180715114827/https://www.github.com/jmarlin/wsbe">The final version of the code can be found at my github, ready to be rolled into your own project or built to run directly in your browser on just about any platform.</a></p>