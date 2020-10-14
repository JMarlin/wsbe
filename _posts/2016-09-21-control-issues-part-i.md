---
layout: post
title:  "6 - Control Issues Part I"
author: "Joe Marlin"
---

<p>So here's where our window manager starts to actually get really exciting: We have a desktop, and the desktop contains windows. But the windows themselves contain bupkis. What the hell good to anyone is a window that doesn't actually have anything in it? It's time to change that. It's time to implement controls.</p>

<p>There's a really obvious thought staring us in the face that you might not even have realized: What if our windows could have windows in them? I mean, what's so special about a desktop? It's a rectangular region with windows inside of it. Why couldn't we do the same with a window itself? </p>

<p>So your immediate thought might be: 'Well, that's not so weird, but I thought we were going to be talking about controls, not nested windows'. But think about it this way: What if our controls <em>were</em> windows?</p>

<p>A window's defining feature is that it has a location, has a size, displays visible content within its area, can contain other objects (controls or child windows), and responds when you interact with it. A button has a location, has a size, displays visible content within its area, can contain other objects (say an image) and responds when you interact with it. A desktop has a location -- (0, 0) --, a size, displays visible content within its area, can contain other objects (windows), and responds when you click it. These are all basically the same thing, with minor differences in the details. So why not unify them?</p>

<p><strong>WARNING:</strong> This week, we will not end up with functioning code. We have a lot of work to do to get controls happening, hence why this is being broken into two parts. Today, we'll modify our classes so that everything is a window and we'll update our <code>Context</code> to support our nested drawing. Next week we'll implement the actual drawing and interception of mouse events and end up with a working system. </p>

<p>&nbsp;</p>

<h2 id="thedesktopshallinheritthewindow">The Desktop Shall Inherit the Window</h2>

<p>The takeaway: Everything's going to be a window. Most importantly, <code>Windows</code> need to be able to do a lot of the stuff that the <code>Desktop</code> has been doing that we mentioned above, namely containing other windows, drawing them and passing mouse events to them.</p>

<p>But we still have some stuff that our <code>Desktop</code> is going to do that <code>Windows</code>, in general, don't. Like keeping track of and drawing the mouse cursor. If you're familiar with an OOP language like C++, you'd probably recognize this as a good candidate for using inheritance. But is there a way to do that in C?</p>

<p>There is, and to show you how we do it, I'll show you how we're going to rewrite our <code>Desktop</code> struct:  </p>

<pre><code class="language-Language-C">typedef struct Desktop_struct {  
    Window window; //Inherits window class
    uint16_t mouse_x;
    uint16_t mouse_y;
} Desktop;
</code></pre>

<p>If you've never dealt with this design before, it's one of the cooler legal hacks we can use in C to allow for a type of inheritance. According to the C spec, the address of the first element of a struct and the address of the struct itself will always be equivalent. Therefore, by putting a <code>Window</code> at the beginning of the desktop struct we can cast any <code>Desktop</code> pointer into a <code>Window</code> pointer and pass it to any <code>Window_...</code> function without the function being any the wiser.</p>

<p>&nbsp;</p>

<p>So, we've almost completely eviscerated our desktop struct. To make up for that, we're going to continue our work by inserting the properties we just stripped out of <code>Desktop</code> into the <code>Window</code> class. As mentioned, most importantly this means giving <code>Window</code>s each a list of child windows.</p>

<p>However, while we're at it, we're also going to throw in three new things: The first is a reference to a parent window -- since we're going to be nesting windows inside windows, we're going to want this to traverse the tree of windows in our code. The second is an unsigned int called 'flags' that we're going to use as a bit field. For now, we'll just be using it to store whether our window is going to be decorated or not since buttons don't have titlebars. The final addition is going to be a function pointer called 'paint_function' -- just put it in for now, and I'll explain what that's for down below.  </p>

<pre><code class="language-Language-C">//Forward struct declaration for function type declarations
struct Window_struct;

//Callback function type declarations
typedef void (*WindowPaintHandler)(struct Window_struct*);

typedef struct Window_struct {  
    struct Window_struct* parent; //New
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    uint16_t flags; //New
    Context* context; 
    List* children; //New
    uint8_t last_button_state; //New
    WindowPaintHandler paint_function; //New
    struct Window_struct* drag_child; //New
    int16_t drag_off_x; //New
    int16_t drag_off_y; //New
} Window;
</code></pre>

<p>&nbsp;</p>

<p>While we're screwing around with our class declaration stuff, we're going to add a few more defines that will help us out later. To help us do math involving the window border (or lack thereof) later on, we'll centralize its dimensions, and we'll also define the flag bit for turning a window's decoration off.</p>

<pre><code class="language-Language-C">#define WIN_TITLEHEIGHT 31 
#define WIN_BORDERWIDTH 3

//Some flags to define our window behavior
#define WIN_NODECORATION 0x1
</code></pre>

<p>&nbsp;</p>

<p>Now, what's the story with that <code>paint_function</code> function pointer? </p>

<p>So far, all of our windows have used the exact same function in the exact same way to draw the exact same thing. It may be obvious: This is completely untenable for a real system. Different kinds of controls need to draw themselves differently, and custom user controls need to give the user complete control over how they're drawn. Also, now that our desktop is just a kind of window we certainly can't draw the same thing there that we do in the rest of our windows. </p>

<p><code>paint_function</code> is our way of handling that. What we'll do is split window painting into two concerns: Context setup and actual drawing. The setup will always be the same for every window, so we'll bake that into a fixed function later on. But once the setup is finished, to let the window draw whatever it needs to, we'll call whatever its <code>paint_function</code> points to. Then that function can be set or overridden in a constructor or elsewhere.</p>

<p>For now, let's throw together a 'default' window painting function so that we have something to assign that pointer to when we first initialize a vanilla window:  </p>

<pre><code class="language-Language-C">//This is the default paint method for a new window
void Window_paint_handler(Window* window) {

    //Fill in the window background
    Context_fill_rect(window-&gt;context, 0, 0,
                      window-&gt;width, window-&gt;height, WIN_BGCOLOR);
}
</code></pre>

<p>In the same way, we can make a similar function that we can use later to draw the desktop:  </p>

<pre><code class="language-Language-C">//Paint the desktop 
void Desktop_paint_handler(Window* desktop_window) {

    //Fill the desktop
    Context_fill_rect(desktop_window-&gt;context, 0, 0, desktop_window-&gt;context-&gt;width, desktop_window-&gt;context-&gt;height, 0xFFFF9933);
}
</code></pre>

<p>&nbsp;</p>

<p>Now that we've changed all of its members, we need to look into modifying the window constructor a bit. To begin with, of course, we need to update its arguments to take the initial window flags, and we also need to add the initialization of our new properties. But beyond that, we're also going to split out the <em>initialization</em> of the class from the <em>allocation</em> of the class.</p>

<p>This has to do with the way our inheritance works for the <code>Desktop</code> class. The <code>Window</code> part of the desktop initialization will be exactly the same as for any other window, so for that we'll just have our <code>Desktop</code> constructor use the exact same initialization code that normal <code>Windows</code> use. But a <code>Desktop</code> structure is bigger than a <code>Window</code> structure, so the <code>malloc(sizeof(Window))</code> we use in <code>Window_new()</code> is no good for allocating the memory needed for a <code>Desktop</code>. If we split out the initialization of a <code>Window</code> from our <code>Window</code> constructor, we can let <em>both</em> constructors use it and deal with setting up the right allocation size independently.  </p>

<pre><code class="language-Language-C">//Updated window constructor
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
    if(!(window-&gt;children = List_new()))
        return 0;

    //Assign the property values
    window-&gt;x = x;
    window-&gt;y = y;
    window-&gt;width = width;
    window-&gt;height = height;
    window-&gt;context = context;
    window-&gt;flags = flags;
    window-&gt;parent = (Window*)0; //We'll assign a parent later -- or not
    window-&gt;last_button_state = 0;
    window-&gt;paint_function = Window_paint_handler;
    window-&gt;drag_child = (Window*)0;
    window-&gt;drag_off_x = 0;
    window-&gt;drag_off_y = 0;

    return 1;
}
</code></pre>

<p>&nbsp;</p>

<p>Now that we've modified our <code>Window</code> constructor to reflect our merging of <code>Desktop</code> functionalities, let's do the <code>Desktop</code> constructor as well. For the most part, we're going to now let <code>Window_init()</code> handle most of our initialization now that <code>Desktop</code> is just a glorified, undecorated <code>Window</code>. But still after that we still have to do the minor housekeeping of updating the <code>paint_function</code> to the desktop painting handler we wrote earlier and then initializing the desktop-unique properties:</p>

<pre><code class="language-Language-C">Desktop* Desktop_new(Context* context) {

    //Malloc or fail 
    Desktop* desktop;
    if(!(desktop = (Desktop*)malloc(sizeof(Desktop))))
        return desktop;

    //Initialize the Window bits of our desktop (just an
    //undecorated window the size of the drawing context)
    if(!Window_init((Window*)desktop, 0, 0, context-&gt;width, context-&gt;height, WIN_NODECORATION, context)) {

        free(desktop);
        return (Desktop*)0;
    }

    //Here's where we override that paint function to draw the
    //desktop background instead of the default window background
    desktop-&gt;window.paint_function = Desktop_paint_handler;

    //Now continue by filling out the desktop-unique properties 
    desktop-&gt;mouse_x = desktop-&gt;window.context-&gt;width / 2;
    desktop-&gt;mouse_y = desktop-&gt;window.context-&gt;height / 2;

    return desktop;
}
</code></pre>

<p>&nbsp;</p>

<p>Next thing we have to do in order to integrate the common functions of <code>Window</code> and <code>Desktop</code> is move a few functions we want to apply to both over from the desktop class into the window class. Namely:</p>

<ul>
<li><code>List* Desktop_get_windows_above(Desktop* desktop, Window* child)</code></li>
<li><code>void Desktop_process_mouse(Desktop* desktop, uint16_t mouse_x, uint16_t mouse_y, uint8_t mouse_buttons)</code></li>
<li><code>Window* Desktop_create_window(Desktop* desktop, int16_t x, int16_t y, uint16_t width, int16_t height, uint16_t flags)</code></li>
</ul>

<p>Literally all you have to do is go through those functions, rename them all to <code>Window_...</code> instead of <code>Desktop_...</code> and replace all instances of <code>Desktop* desktop</code> with <code>Window* window</code>. Not even going to cover that much, because it's pretty trivial. But when you're done with that, we have some minor changes to make.</p>

<p>We'll be completely deleting <code>Desktop_paint()</code> as well, but we won't be moving it over to the <code>Window</code> class. We'll be completely rewriting <code>Window_paint()</code> later and we're going to use that to handle both the desktop drawing and all of the window drawing. </p>

<p>The first minor change we need to make to the functions we just moved: The way we're currently handling top-down drawing using the desktop. As it was, we were handling all mouse input in the <code>Desktop_process_mouse()</code> and, at the end of that routine, forcing a redraw of all our children which was always finalized by drawing the mouse over everything.</p>

<p>For now, we still want that to be the case, but unfortunately we moved that function over to the <code>Window</code> class. That much is fine, because we want our windows eventually to handle their own mouse events no matter how deeply nested. But if we keep drawing the desktop in that function, the desktop redraw is going to get called on every single window that gets a mouse event, which is certainly not what we want. So, to start with, let's clip this bit of code out of <code>Window_process_mouse()</code>:  </p>

<pre><code class="language-Language-C">//Tail end of Window_process_mouse():

    //Desktop_paint(desktop); &lt;- Remove this bit
                                 (this function doesn't even exist anymore)

    //Update the stored mouse button state to match the current state 
    desktop-&gt;last_button_state = mouse_buttons;
}
</code></pre>

<p>There, now window mouse events won't wrongfully trigger a full-screen redraw. But we do need to do that somewhere. So we'll write a small function for the <code>Desktop</code> which we'll be able to call in our <code>main()</code> that will call <code>Window_process_mouse()</code> to do the normal <code>Desktop</code> mouse handling, but, when that's complete, do the screen redraw and then draw that mouse we stripped out:</p>

<pre><code class="language-Language-C">//Our overload of the Window_process_mouse function used to capture the screen mouse position 
void Desktop_process_mouse(Desktop* desktop, uint16_t mouse_x,  
                           uint16_t mouse_y, uint8_t mouse_buttons) {

    //Capture the mouse location in order to draw it later
    desktop-&gt;mouse_x = mouse_x;
    desktop-&gt;mouse_y = mouse_y;

    //Do the old generic mouse handling
    Window_process_mouse((Window*)desktop, mouse_x, mouse_y, mouse_buttons);

    //Now that we've handled any changes the mouse may have caused, we need to
    //update the screen to reflect those changes 
    Window_paint((Window*)desktop);

    //And finally draw the hacky mouse, as usual
    Context_fill_rect(desktop-&gt;window.context, desktop-&gt;mouse_x, desktop-&gt;mouse_y, 10, 10, 0xFF000000);
}
</code></pre>

<p>&nbsp;</p>

<p>One more small change before we're done with the basic task of making windows nestable: We need to set that <code>window-&gt;parent</code> property at some point so that we can navigate the tree of windows in our code. Easy enough, this is just a quick change to our <code>Window_create_window()</code> function:</p>

<pre><code class="language-Language-C">//The tail end of Window_create_window():

    //Set the new child's parent 
    new_window-&gt;parent = window;

    return new_window;
}
</code></pre>

<p>&nbsp;</p>

<p>Okay! So that's that! We've moved the bulk of our desktop properties and functions over to windows and made desktops a subclass of window, so now windows can contain windows can contain windows. Now we just have to tackle those minor <code>Context</code> changes.</p>

<p>&nbsp;</p>

<h2 id="actuallygettingthisjunkonthescreen">Actually Getting This Junk on the Screen</h2>

<p>We still have a  bunch of work to do when it comes to window painting -- and even then, we won't actually start painting anything until Part II. The thing is, now that we're allowing for child windows, we have to change the way we're doing our drawing yet again. And before we can do that, we have to do a bit of surgery on our <code>Context</code> to support those changes.</p>

<p>First and easiest: When we call those <code>paint_function</code>s on nested child windows, we want them to not have to know anything about where they are on the screen. We only want the window manager to care about that -- as far as our windows will be concerned their top left corner should be (0, 0). To do this, we can add simple support to our <code>Context</code> for changing the location of the drawing origin. To do that, let's start by adding a <code>translate_x</code> and <code>translate_y</code> property to the context:</p>

<pre><code class="language-Language-C">typedef struct Context_struct {  
    uint32_t* buffer; 
    uint16_t width; 
    uint16_t height; 
    int translate_x; //Our new translation values
    int translate_y;
    List* clip_rects;
} Context;
</code></pre>

<p>&nbsp;</p>

<p>The idea here is that before we call the <code>paint_function</code> on a window, we can set the context's <code>translate_x</code> and <code>translate_y</code> to the top-left corner of that window and then the <code>paint_function</code> doesn't have to do any weird calculation themselves to determine where things go. To make that actually happen, we just need to use those values to shift things when we draw our clipped rectangles:</p>

<pre><code class="language-Language-C">void Context_clipped_rect(Context* context, int x, int y, unsigned int width,  
                          unsigned int height, Rect* clip_area, uint32_t color) {

    int cur_x;
    int max_x = x + width;
    int max_y = y + height;

    //NEW: Translate the rectangle coordinates by the context translation values
    x += context-&gt;translate_x;
    y += context-&gt;translate_y;
    max_x += context-&gt;translate_x;
    max_y += context-&gt;translate_y;

    //And then the usual stuff:
    if(x &lt; clip_area-&gt;left)
        x = clip_area-&gt;left;

    if(y &lt; clip_area-&gt;top)
        y = clip_area-&gt;top;

    if(max_x &gt; clip_area-&gt;right + 1)
        max_x = clip_area-&gt;right + 1;

    if(max_y &gt; clip_area-&gt;bottom + 1)
        max_y = clip_area-&gt;bottom + 1;

    for(; y &lt; max_y; y++) 
        for(cur_x = x; cur_x &lt; max_x; cur_x++) 
            context-&gt;buffer[y * context-&gt;width + cur_x] = color;
}
</code></pre>

<p>With that, since all of our current drawing ultimately calls down to that function, everything we draw will be properly offset now.</p>

<p>&nbsp;</p>

<p>We have another little bit of graphics groundwork to cover before I can let you go for the day, though. To do what we need to do when we actually start getting into putting pixels on the screen in Part II, we need to add one more boolean operation to the context's clipping rectangles: Intersection.</p>

<p>Just like subtraction and addition, we'll start out by implementing a simple function that does intersection just between two rectangles. We'll do this by basically doing the same thing as our <code>Rect_split()</code> code, but this time we'll use the shrinking rectangle as our output:  </p>

<pre><code class="language-Language-C">/*
 ___a
|  _|_b      _c
|_|_| | --&gt; |_| 
  |___|
*/
Rect* Rect_intersect(Rect* rect_a, Rect* rect_b) {

    Rect* result_rect;

    //If the two rectangles don't overlap, there's no result
    if(!(rect_a-&gt;left &lt;= rect_b-&gt;right &amp;&amp;
       rect_a-&gt;right &gt;= rect_b-&gt;left &amp;&amp;
       rect_a-&gt;top &lt;= rect_b-&gt;bottom &amp;&amp;
       rect_a-&gt;bottom &gt;= rect_b-&gt;top))
        return (Rect*)0;

    //The result rectangle starts out as a copy of the first input rect
    if(!(result_rect = Rect_new(rect_a-&gt;top, rect_a-&gt;left,
                                rect_a-&gt;bottom, rect_a-&gt;right)))
        return (Rect*)0;

    //Shrink to the right-most left edge
    if(rect_b-&gt;left &gt;= result_rect-&gt;left &amp;&amp; rect_b-&gt;left &lt;= result_rect-&gt;right) 
        result_rect-&gt;left = rect_b-&gt;left;

    //Shrink to the bottom-most top edge
    if(rect_b-&gt;top &gt;= result_rect-&gt;top &amp;&amp; rect_b-&gt;top &lt;= result_rect-&gt;bottom) 
        result_rect-&gt;top = rect_b-&gt;top;

    //Shrink to the left-most right edge
    if(rect_b-&gt;right &gt;= result_rect-&gt;left &amp;&amp; rect_b-&gt;right &lt;= result_rect-&gt;right)
        result_rect-&gt;right = rect_b-&gt;right;

    //Shrink to the top-most bottom edge
    if(rect_b-&gt;bottom &gt;= result_rect-&gt;top &amp;&amp; rect_b-&gt;bottom &lt;= result_rect-&gt;bottom)
        result_rect-&gt;bottom = rect_b-&gt;bottom;

    return result_rect;
}
</code></pre>

<p>&nbsp;</p>

<p>Now, to use it to create an intersection against our <code>Context</code>'s whole clipping rectangle collection. The intersection of the whole set of clipping rectangles is just the collective results of the intersections of the input rectangle and each clipping rectangle in turn:  </p>

<pre><code class="language-Language-C">//Update the clipping rectangles to only include those areas within both the
//existing clipping region AND the passed Rect
void Context_intersect_clip_rect(Context* context, Rect* rect) {

    int i;
    List* output_rects;
    Rect* current_rect;
    Rect* intersect_rect;

    //Allocate a new list of rectangles into which we'll put
    //our intersection results
    if(!(output_rects = List_new()))
        return;

    //Do an intersection against the passed rectangle for each 
    //rectangle in clip_rects
    for(i = 0; i &lt; context-&gt;clip_rects-&gt;count; i++) {

        //Get the next clip_rect and do the intersection
        current_rect = (Rect*)List_get_at(context-&gt;clip_rects, i);
        intersect_rect = Rect_intersect(current_rect, rect);

        //If there was a result, put it in the output list
        if(intersect_rect)
            List_add(output_rects, intersect_rect);
    }

    //Now that we're done, we can delete the original list of 
    //clipping rectangles
    while(context-&gt;clip_rects-&gt;count)
        List_remove_at(context-&gt;clip_rects, 0);
    free(context-&gt;clip_rects);

    //And replace it with the intersection results
    context-&gt;clip_rects = output_rects;

    //Now that we're done with the input rect, we'll free it
    free(rect);
}
</code></pre>

<p>&nbsp;</p>

<h2 id="theunsatisfyingend">The Unsatisfying End</h2>

<p>That's all for today. A whole lot of infrastructure and nothing to show for it. It's a little frustrating, I know, but there's a lot of changes that need to be made for this particular subject. By moving a lot of the window management functionality from the desktop into the windows themselves, we've got the structural framework now to manage and interact with nested windows and controls. And with the small changes we've made to our <code>Context</code> we'll be able to properly clip and draw those objects to any arbitrary nesting.</p>

<p>I tried to put all of the changes into one article at first, but it would've been twice as long as the longest article I've done so far, and I don't want to kill you. Part of the reason I was a bit late this week, too.</p>

<p>But still, feel free to let your excitement build, because you have a lot to look forward to in Part II. By the end of Part II we'll have implemented the drawing and mouse event handling of sub-windows/controls and written some controls that, like we've done with <code>Desktop</code> in this article, inherit from <code>Window</code> to get the bulk of their functionality done. And it's not long from there that our windowing system will be, for most intents and purposes, finished! </p>

<hr/>

<p><strong>AUTHOR'S NOTE:</strong> Normally, this is the part where I link to the code. But frankly, the code isn't quite ready and won't be until I post Part II. Bummer, I know. That said, you probably already know where the repo is if you've been reading these, so you're probably going to look at it anyway. Whatever. </p>