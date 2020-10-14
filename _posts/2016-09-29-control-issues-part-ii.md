---
layout: post
title:  "7 - Control Issues Part II"
author: "Joe Marlin"
---

<p>Sorry to leave you dudes and dudettes on a cliffhanger, there. Apparently building a robust, recursive framework for window controls is relatively nontrivial. I was shocked. </p>

<p>Last week saw us moving a lot of the core components of the <code>Desktop</code> class over into the <code>Window</code> class -- most critically the list of child windows which we're going to be using to implement a tree of controls -- subsequently using some C-style inheritance in order to update the <code>Desktop</code> class to be a sub-class of <code>Window</code>. Finally, we also added rectangle intersection to our <code>Contex</code>'s clipping area tools in order to do the clipping we're going to need to do when drawing these child windows.</p>

<p>Today, we're going to update our window drawing logic in order to make that child drawing happen. Then we're going to update our mouse handling so that we can forward mouse actions to the window that the mouse was actually on top of and finally tie everything together by implementing and using a simple <code>Window</code>-derived toggle button control.</p>

<p>&nbsp;</p>

<h2 id="recursionseerecursion">Recursion: See 'Recursion'.</h2>

<p>A small consideration before we dive into drawing our windows: Now that our windows can potentially be nested, their <code>x</code> and <code>y</code> location properties will no longer be absolute values, but instead relative to their parent. As such, to do our clipping we'll need a simple way to get the absolute screen position of a window. And we can do that by just adding up the offsets of each of a window's parents until we hit the desktop. So here's a nice simple thing to start us off with:</p>

<pre><code class="language-C">//Recursively get the absolute on-screen x-coordinate of this window
int Window_screen_x(Window* window) {

    if(window-&gt;parent)
        return window-&gt;x + Window_screen_x(window-&gt;parent);

    return window-&gt;x;
}

//Recursively get the absolute on-screen y-coordinate of this window
int Window_screen_y(Window* window) {

    if(window-&gt;parent)
        return window-&gt;y + Window_screen_y(window-&gt;parent);

    return window-&gt;y;
}
</code></pre>

<p>&nbsp;</p>

<p>But now it comes to the nitty-gritty. We can finally get to the real clipping-and-drawing logic that our updated <code>Window_paint()</code> function is going to use to poop our new arbitrarily-nested tree of windows onto the screen. As a starting point, this is just going to be an extension of what we were already doing in <code>Desktop_paint()</code>: Set the clipping to the window boundaries, subtract the rectangles for any sibling windows that are overlapping us, and then paint the window. </p>

<p>We now have this caveat, though: We need to also restrict our clipping to the clipping of our parent window. If we have a button inside a window and it's slightly too wide, we don't want it dangling off the side of the window. As I mentioned before in part I: The visible area of any window is going to be the intersection of that window's bounding rectangle with the visibility clipping of its parent.</p>

<p>As such, we're going to split out the clipping bit from the rest of the paint function, both because it's going to get quite a bit more complex, but more importantly because we're going to need to use it as recursive function. In order to get a window's clipping, we need to get its parent's clipping. And in order to get that parent's clipping, we need to get <em>its</em> parent's clipping. And so on and so on. The thing that saves this from going on forever is the desktop -- the desktop has no parent, so we have nothing to call the function on.</p>

<p>And so, for the case of the desktop, we simply add the desktop bound rectangle to the drawing context's clipping area and return. Then, for any other window, once we've gotten the parent clipping we can basically proceed by doing the same thing we were previously doing in <code>Desktop_paint()</code>, except that instead of <em>adding</em> the window rectangle to the clipping we're going to <em>intersect</em> it. Then we can go ahead and subtract any overlapping siblings as usual.</p>

<p>If you're A little lost at this point, here's a rough example of what we're going to have going on. The gif below shows how we need things to proceed if we needed to set up the clipping area for a button-window inside of a window on the desktop: <br/>
<img src="/web/20180715114750im_/http://trackze.ro/content/images/2016/09/giphy-1.gif" alt=""/></p>

<p>So, without further ado, our new window clipping region function:</p>

<pre><code class="language-C">//Apply clipping for window bounds
//(We use the in_recursion variable to tell us if we were called by
//another function or if we were called by ourself -- explained more below)
void Window_apply_bound_clipping(Window* window, int in_recursion) {

    Rect* temp_rect;
    int screen_x, screen_y;
    List* clip_windows;
    Window* clipping_window;

    //Use our new functions to get the window's absolute position
    screen_x = Window_screen_x(window);
    screen_y = Window_screen_y(window);

    //Build the visibility rectangle for this window
    //If the window is decorated and we're recursing, we want to limit
    //the window's clipping area to the area inside the window decoration
    //so that child windows don't get drawn over the window decorations.
    if((!(window-&gt;flags &amp; WIN_NODECORATION)) &amp;&amp; in_recursion) {

        //Limit client drawable area 
        screen_x += WIN_BORDERWIDTH;
        screen_y += WIN_TITLEHEIGHT;
        temp_rect = Rect_new(screen_y, screen_x,
                             screen_y + window-&gt;height - WIN_TITLEHEIGHT - WIN_BORDERWIDTH - 1, 
                             screen_x + window-&gt;width - (2*WIN_BORDERWIDTH) - 1);
    } else {

        //If we're not decorated, the entire area of the window is drawable.
        //If we're not recursing, it means we're about to do window drawing
        //when we return from this function, so we want to leave the border
        //area unclipped so we can paint the window decorations
        temp_rect = Rect_new(screen_y, screen_x, screen_y + window-&gt;height - 1, 
                             screen_x + window-&gt;width - 1);
    }

    //If there's no parent (meaning we're at the top of the window tree)
    //then we just add our rectangle and exit
    //This is where our recursions will finally halt 
    if(!window-&gt;parent) {

        Context_add_clip_rect(window-&gt;context, temp_rect);
        return;
    }

    //Now, here's the recursive part. For anything but the top-level-window/desktop, 
    //we must recursively call this function again on our parent in order to limit
    //the clipping area we're working with to the visible area of any windows we're 
    //nested inside of
    Window_apply_bound_clipping(window-&gt;parent, 1);

    //Now that we've gotten our parent's clipping area, we can intersect our own
    //window bound rectangle against the existing clipping area we got from our
    //parent to just the area of the current window  
    Context_intersect_clip_rect(window-&gt;context, temp_rect);

    //And finally, we subtract the rectangles of any siblings that are occluding us
    //This part is *exactly* the same as the sibling subtraction we were previously
    //doing in the now-defunct Desktop_paint(), with the change that, since we
    //moved it over into the Window class in Part I, our get_windows_above function
    //has a slightly different name now 
    clip_windows = Window_get_windows_above(window-&gt;parent, window);
    while(clip_windows-&gt;count) {

        clipping_window = (Window*)List_remove_at(clip_windows, 0);

        //Make sure we don't try and clip the window from itself
        if(clipping_window == window)
            continue;

        //Get a rectangle from the window, subtract it from the clipping 
        //region, and dispose of it
        screen_x = Window_screen_x(clipping_window);
        screen_y = Window_screen_y(clipping_window);

        temp_rect = Rect_new(screen_y, screen_x,
                             screen_y + clipping_window-&gt;height - 1,
                             screen_x + clipping_window-&gt;width - 1);
        Context_subtract_clip_rect(window-&gt;context, temp_rect);
        free(temp_rect);
    }

    //Finally, dispose of the used-up Window_get_windows_above() list and return
    free(clip_windows);
}
</code></pre>

<p>&nbsp;</p>

<p>Cool. With the clipping out of the way, we're almost ready to finally get to <code>Window_paint()</code>. But as long as we're spinning off window-drawing things into their own functions, let's also also spin off the window border drawing to make the main painting routine a little more concise. While we're at it, we can also change it slightly to use those new <code>WIN_</code> constants since we're using them everywhere else:  </p>

<pre><code class="language-C">void Window_draw_border(Window* window) {

    int i;
    int screen_x = Window_screen_x(window);
    int screen_y = Window_screen_y(window);

    //Draw a border around the window 
    Context_draw_rect(window-&gt;context, screen_x, screen_y,
                      window-&gt;width, window-&gt;height, WIN_BORDERCOLOR);
    Context_draw_rect(window-&gt;context, screen_x + 1, screen_y + 1,
                      window-&gt;width - 2, window-&gt;height - 2, WIN_BORDERCOLOR);
    Context_draw_rect(window-&gt;context, screen_x + 2, screen_y + 2,
                      window-&gt;width - 4, window-&gt;height - 4, WIN_BORDERCOLOR);

    //Draw a border line under the titlebar
    for(i = 0; i &lt; WIN_BORDERWIDTH; i++)
        Context_horizontal_line(window-&gt;context, screen_x + WIN_BORDERWIDTH,
                                screen_y + i + WIN_TITLEHEIGHT - WIN_BORDERWIDTH,
                                window-&gt;width - (2*WIN_BORDERWIDTH),
                                WIN_BORDERCOLOR);

    //Fill in the titlebar background
    Context_fill_rect(window-&gt;context, screen_x + WIN_BORDERWIDTH, 
                      screen_y + WIN_BORDERWIDTH,
                      window-&gt;width - 6, 25, WIN_TITLECOLOR);
}
</code></pre>

<p>&nbsp;</p>

<p>Now. It's time. We can finally get to our window painting routine. The updated version will be now used for both desktop drawing as well as window drawing, and because of the recursive/nested nature of our new child-window scheme we also need to set it up to call itself on each of its children when done</p>

<pre><code class="language-C">//The paint function -- yet another function we moved 
//from Desktop to Window last round
void Window_paint(Window* window) {

    int i, screen_x, screen_y, child_screen_x, child_screen_y;
    Window* current_child;
    Rect* temp_rect;

    //Start by limiting painting to the window's visible area
    //using that shiny new clipping area calculator we just whipped up
    Window_apply_bound_clipping(window, 0);

    //Use those new functions to get the absolute window location
    screen_x = Window_screen_x(window);
    screen_y = Window_screen_y(window);

    //If we have window decorations turned on, first use the border drawing
    //function we just spun off in order to draw those window decorations,
    //and then limit the clipping area further so that our painting handler 
    //callback will only be able to affect the inner drawable area of the window 
    if(!(window-&gt;flags &amp; WIN_NODECORATION)) {

        //Draw border
        Window_draw_border(window);

        //Limit client drawable area 
        screen_x += WIN_BORDERWIDTH;
        screen_y += WIN_TITLEHEIGHT;
        temp_rect = Rect_new(screen_y, screen_x,
                             screen_y + window-&gt;height - WIN_TITLEHEIGHT - WIN_BORDERWIDTH - 1, 
                             screen_x + window-&gt;width - (2*WIN_BORDERWIDTH) - 1);
        Context_intersect_clip_rect(window-&gt;context, temp_rect);
    }

    //Then subtract the screen rectangles of all children 
    //NOTE: We don't do this in Window_apply_bound_clipping because, due to 
    //its recursive nature, it would cause the screen rectangles of all of 
    //our parent's children to be subtracted from the clipping area -- which
    //would eliminate this window. 
    for(i = 0; i &lt; window-&gt;children-&gt;count; i++) {

        current_child = (Window*)List_get_at(window-&gt;children, i);

        child_screen_x = Window_screen_x(current_child);
        child_screen_y = Window_screen_y(current_child);

        temp_rect = Rect_new(child_screen_y, child_screen_x,
                             child_screen_y + current_child-&gt;height - 1,
                             child_screen_x + current_child-&gt;width - 1);
        Context_subtract_clip_rect(window-&gt;context, temp_rect);
        free(temp_rect);
    }

    //Finally, with all the clipping set up, we can set the context's 0,0 to the top-left corner
    //of the window's drawable area using the context translation parameters we added in Part I
    //and ultimately call the window's paint handler function 
    window-&gt;context-&gt;translate_x = screen_x;
    window-&gt;context-&gt;translate_y = screen_y;
    window-&gt;paint_function(window); //Paint it!

    //Now that we're done drawing this window, we can clear the changes we made to the context
    Context_clear_clip_rects(window-&gt;context);
    window-&gt;context-&gt;translate_x = 0;
    window-&gt;context-&gt;translate_y = 0;

    //Since we're still painting the whole screen whenever anything changes, we must also 
    //call on all of our children to paint themselves 
    for(i = 0; i &lt; window-&gt;children-&gt;count; i++) {

        current_child = (Window*)List_get_at(window-&gt;children, i);
        Window_paint(current_child);
    }
}
</code></pre>

<p>&nbsp;</p>

<p>And now, with those changes taken care of, we can now paint nested children.</p>

<p>&nbsp;</p>

<h2 id="pointandclick">Point and Click</h2>

<p>Cool enough, but what good is the capacity of displaying controls if we can't actually interact with them? A control that can't control anything is hardly a control at all, it would seem to me. As such, we're going to take a moment and fudge with our mouse handling just a bit.</p>

<p>We did end up moving our <code>Desktop_process_mouse()</code> over to <code>Window_process_mouse()</code> and slightly modifying it last time around, but the core problem with it as it stands is that it currently does a bit of processing to see if we're dragging the titlebar of any of the passed window's immediate children, but it then does nothing to go down the tree of children to see if we're interacting with any children of our children. But, more concerning, we have no mechanism by which a window can actually <em>do</em> anything if we find that it's been interacted with.</p>

<p>We're going to start right there, then. Taking a page out of our window-painting callback playbook, we're going to add another updateable function pointer to our <code>Window</code> class that will allow us to attach an action of some sort if we've detected that the mouse button has gone down on a window:</p>

<pre><code class="language-C">//Forward struct declaration for function type declarations
struct Window_struct;

//Callback function type declarations
typedef void (*WindowPaintHandler)(struct Window_struct*);  
//New function pointer type, takes the pointer to the affected window and
//the x and y coordinates it happened at
typedef void (*WindowMousedownHandler)(struct Window_struct*, int, int);

typedef struct Window_struct {  
    struct Window_struct* parent;
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    uint16_t flags;
    Context* context;
    struct Window_struct* drag_child;
    List* children;
    uint16_t drag_off_x;
    uint16_t drag_off_y;
    uint8_t last_button_state;
    WindowPaintHandler paint_function;
    WindowMousedownHandler mousedown_function; //And add one to the struct
} Window;
</code></pre>

<p>&nbsp;</p>

<p>It should probably go without saying that we'll have to make a default function for this that all windows get on initialization (which doesn't need to actually do anything), and that we'll have to assign that value in <code>Window_init()</code>. Just the same as the window paint callback:</p>

<pre><code class="language-C">//The default handler for window mouse events doesn't do anything
void Window_mousedown_handler(Window* window, int x, int y) {

    return;
}
</code></pre>

<pre><code class="language-C">//Here's the new tail-end of Window_init()

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

    return 1;
}
</code></pre>

<p>&nbsp;</p>

<p>Now that we've got the means, let's overhaul <code>Window_process_mouse()</code> so that 1) it calls down into the mouse handler of a child windows if the mouse is found to be over it and 2) if we found a mouse button up-&gt;down transition, and we didn't find that we were over any children (meaning we must be over ourself), it fires off whatever mousedown function is tied to the current window:</p>

<pre><code class="language-C">//Overhauling the window mouse handler to work with our nested children model
void Window_process_mouse(Window* window, uint16_t mouse_x,  
                          uint16_t mouse_y, uint8_t mouse_buttons) {

    int i, inner_x1, inner_y1, inner_x2, inner_y2;
    Window* child;

    //If we had a button depressed, then we need to see if the mouse was
    //over any of the child windows
    //We go front-to-back in terms of the window stack for free occlusion
    for(i = window-&gt;children-&gt;count - 1; i &gt;= 0; i--) {

        child = (Window*)List_get_at(window-&gt;children, i);

        //If mouse isn't window bounds, we can't possibly be interacting with it 
        if(!(mouse_x &gt;= child-&gt;x &amp;&amp; mouse_x &lt; (child-&gt;x + child-&gt;width) &amp;&amp;
           mouse_y &gt;= child-&gt;y &amp;&amp; mouse_y &lt; (child-&gt;y + child-&gt;height))) 
            continue;

        //Now we'll check to see if we're dragging a titlebar
        if(mouse_buttons &amp;&amp; !window-&gt;last_button_state) {

            //While we're at it, let's adjust things so that a raise happens
            //not just on the titlebar, but whenever we click inside a 
            //child, to be more consistent with most other GUIs
            List_remove_at(window-&gt;children, i); //Pull window out of list
            List_add(window-&gt;children, (void*)child); //Insert at the top

            //See if the mouse position lies specifically within the bounds of
            //the current window's titlebar
            //We check the decoration flag because we can't drag with no titlebar
            if(!(child-&gt;flags &amp; WIN_NODECORATION) &amp;&amp; 
                mouse_y &gt;= child-&gt;y &amp;&amp; mouse_y &lt; (child-&gt;y + WIN_TITLEHEIGHT)) {

                //We'll set this window as the window being dragged
                //until such a time as the mouse is released
                window-&gt;drag_off_x = mouse_x - child-&gt;x;
                window-&gt;drag_off_y = mouse_y - child-&gt;y;
                window-&gt;drag_child = child;

                //We break without setting target_child if we're doing a drag since
                //that shouldn't trigger a mouse event in the child 
                break;
            }
        }

        //Found a target, so forward the mouse event to that window and quit looking
        //We subtract the window offset since we're making all coordinates inside of
        //a window relative to its upper-lefthand corner
        Window_process_mouse(child, mouse_x - child-&gt;x, mouse_y - child-&gt;y, mouse_buttons); 
        break;
    }

    //Moving this outside of the mouse-in-child detection since it doesn't really
    //have anything to do with it. But still cancelling any drags in the same
    //way as always
    if(!mouse_buttons)
        window-&gt;drag_child = (Window*)0;

    //Update drag window to match the mouse if we have an active drag window
    if(window-&gt;drag_child) {

        window-&gt;drag_child-&gt;x = mouse_x - window-&gt;drag_off_x;
        window-&gt;drag_child-&gt;y = mouse_y - window-&gt;drag_off_y;
    }

    //If we didn't find a target in the search, then we ourselves are the target of any clicks
    //This is where our arbitrary callback finally comes into play
    if(window-&gt;mousedown_function &amp;&amp; mouse_buttons &amp;&amp; !window-&gt;last_button_state) 
        window-&gt;mousedown_function(window, mouse_x, mouse_y);

    //Update the stored mouse button state to match the current state, as usual
    window-&gt;last_button_state = mouse_buttons;
}
</code></pre>

<p>&nbsp;</p>

<p>As you can see, the biggest change above is that the function will now cascade down to call the same function on the highest child that's found under the mouse. This will keep happening until we can't find a child to drill down to anymore, and finally fires it's handler. Now, we're keeping things simple for now for simpleness's sake -- we're not even trying to handle a full mouse click cycle, instead opting for a single, much simpler mouse-down trigger. And, if you try it out, dragging a decorated window inside of a window with this code is kind of broken as is, for maybe not so obvious reasons. But our goal was to be able to make controls, and this'll let us do it until we want to fancy it up.</p>

<p>&nbsp;</p>

<h2 id="letsbuttonthingsup">Let's Button Things Up</h2>

<p>We did all of the hard stuff already. Maybe it's time for some actual results. Let's take what we've built and use it to build a really simple toggle button control that we can throw into one of our windows. We'll keep things easy to show how the infrastructure we just built works, but you should be able to take this example and build a lot off of it.</p>

<p>We're going to make this button control a sub-class of <code>Window</code>, just like we did with the desktop, since that core class already handles most of the basic stuff that a button would need to do now. The button just needs a custom paint handler so that it can make itself actually look like a button instead of a window and it also needs a custom mousedown handler so that it can figure out when it's been toggled. Our window already provides both of those, so the only thing we need to add to it is a place to keep track of our toggled state:</p>

<pre><code class="language-C">typedef struct Button_struct {  
    Window window;
    uint8_t color_toggle;
} Button;
</code></pre>

<p>&nbsp;</p>

<p>With that defined, we need to make a simple constructor that acquires the memory needed to store a new <code>Button</code> object, initialize its window-bits with <code>Window_init()</code>, and then sets the custom functions (which we'll write in a moment).</p>

<pre><code class="language-C">Button* Button_new(int x, int y, int w, int h) {

    //Normal allocation and initialization
    //Like a Desktop, this is just a special kind of window 
    Button* button;
    if(!(button = (Button*)malloc(sizeof(Button))))
        return button;

    if(!Window_init((Window*)button, x, y, w, h, WIN_NODECORATION, (Context*)0)) {

        free(button);
        return (Button*)0;
    }

    //Override default window callbacks
    button-&gt;window.paint_function = Button_paint;
    button-&gt;window.mousedown_function = Button_mousedown_handler;

    //And clear the toggle value
    button-&gt;color_toggle = 0;

    return button;
}
</code></pre>

<p>&nbsp;</p>

<p>On to the handlers. Let's start with the mousedown handler, because that's the simplest. It just needs to flip the button's toggle value on and off:</p>

<pre><code class="language-C">//This just sets and resets the toggle
void Button_mousedown_handler(Window* button_window, int x, int y) {

    //Cast from Window* (because that's what the core window mechanisms think
    //we are) to Button* so that we can access our toggle state
    Button* button = (Button*)button_window;

    //And then toggle it
    button-&gt;color_toggle = !button-&gt;color_toggle;
}
</code></pre>

<p>&nbsp;</p>

<p>And for our paint function, we'll just draw a thin outer border rectangle and put a thicker rectangle on the inside to show the toggle state. We check if the toggle state has been set and base the color that that inner rectangle is going to be drawn based on that information:</p>

<pre><code class="language-C">void Button_paint(Window* button_window) {

    //Do the casting to get access to our Button-centric properties
    Button* button = (Button*)button_window;

    //Decide what the inner rectangle color is going to be
    uint32_t border_color;
    if(button-&gt;color_toggle)
        border_color = WIN_TITLECOLOR;
    else
        border_color = WIN_BGCOLOR - 0x101010;

    //Fill the button background
    Context_fill_rect(button_window-&gt;context, 1, 1, button_window-&gt;width - 1,
                      button_window-&gt;height - 1, WIN_BGCOLOR);

    //Draw the button border
    Context_draw_rect(button_window-&gt;context, 0, 0, button_window-&gt;width,
                      button_window-&gt;height, 0xFF000000);

    //Draw the inner toggle-status indicator
    Context_draw_rect(button_window-&gt;context, 3, 3, button_window-&gt;width - 6,
                      button_window-&gt;height - 6, border_color);
    Context_draw_rect(button_window-&gt;context, 4, 4, button_window-&gt;width - 8,
                      button_window-&gt;height - 8, border_color);                                        
}
</code></pre>

<p>&nbsp;</p>

<p>And our button class is done, because all of the stuff we wrote into the window class already handles the bulk of the gruntwork of figuring out the basics (where to draw us, what our visible area is, if we got clicked on, etc.). All that's left to do is to pop it into our entry function. Before we get there, though, we're going to need one more thing: We need to be able to put the new button into the window.</p>

<p>Yes, we already have a function for spawning a window inside of another window, but it's useless to us since it only knows how to make windows and can't make us anything that derives off of a window. We could make a similar function that makes a button instead of a window, but do we really want to have to write that kind of a function again and again for every sub-type of control and window we might dream up in the future? Instead, let's give windows a simple function that can take a Window* -- which could really be a re-cast pointer to any of our types which derive from the Window class -- and insert it into the window list:</p>

<pre><code class="language-C">//Quick wrapper for shoving a new entry into the child list
void Window_insert_child(Window* window, Window* child) {

    //It's important for pretty much every bit of functionality that the
    //new child be assigned a parent and a drawing context
    child-&gt;parent = window;
    child-&gt;context = window-&gt;context;

    //And then we just shove it into the list
    List_add(window-&gt;children, child);
}
</code></pre>

<p>&nbsp;</p>

<p>Using that, we'll be able to simply spawn a button with <code>Button_new()</code> -- or any other thing we might come up with in the future -- and then install it into another window. And that's precisely what we're about to do next.</p>

<p>&nbsp;</p>

<h2 id="cometogetherrightnow">Come Together Right Now</h2>

<p>Without further ado, the final keystone of the work we've been hammering on for the last two weeks, our updated entry function:</p>

<pre><code class="language-C">//Create and draw a few rectangles and exit
int main(int argc, char* argv[]) {

    //Fill this in with the info particular to your project
    Context* context = Context_new(0, 0, 0);
    context-&gt;buffer = fake_os_getActiveVesaBuffer(&amp;context-&gt;width, &amp;context-&gt;height);

    //Create the desktop 
    desktop = Desktop_new(context);

    //Sprinkle it with windows 
    //This time, we're going to hold on to the pointer of one of them
    //so that we can install the button into it below
    Window* window = Window_create_window((Window*)desktop, 100, 150, 400, 400, 0);
    Window_create_window((Window*)desktop, 10, 10, 300, 200, 0);
    Window_create_window((Window*)desktop, 200, 100, 200, 600, 0);

    //Here's the bit that ties all of the new stuff together:
    //Create and install the button
    Button* button = Button_new(307, 357, 80, 30);
    Window_insert_child(window, (Window*)button);

    //Do the initial draw
    Window_paint((Window*)desktop);

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
</code></pre>

<p>&nbsp;</p>

<p>Compile it up and, if you didn't make any typos, you should be greeted with our old trio of windows joined by a new friend: Our simple little toggle button, which you can click on and off to your heart's content:</p>

<p><img src="/web/20180715114750im_/http://trackze.ro/content/images/2016/09/download.png" alt=""/></p>

<p>&nbsp;</p>

<h2 id="finalthoughts">Final Thoughts</h2>

<p>It took a little bit of doin', but we managed to implement a framework for controls by harnessing the power of inheritance and recursive structures. If the section above wherein we created the button class is trying to point out anything, it's that, with the way we've created a base system that abstracts the lower-level details that a lot of things have in common, we now have the power to build a lot of different functionalities while only adding a minimal amount of code. I mean, really, look at the button. There is an incredible amount we can do with just one hook into a mouse event and a custom paint handler.</p>

<p>So if there's one thing I would suggest, besides reviewing the code from these past two weeks very thoroughly since we wrote an appreciable amount of it, it would be digging into the code to write yet another new control or two. Maybe a checkbox. Or a grouping frame. You could even do a progress meter pretty easily. But with the power of this kind of design, it all just becomes building blocks. I mean, a checkbox could even inherit from the toggle button class we just made. Why not? The only thing that's different there is a minor change to the way it draws itself.</p>

<p>One final thing to note that we haven't really discussed at all in this series, which is frankly a little surprising since this is mainly aimed at OS hobbyists: How would we make all of this work as a service for user applications? It didn't matter too much before, but now that we've gotten as far as building up windows with controls and events it's something to think about. Now, if you were going to do some kind of Windows-type model where this service gets mapped into each process's address space and can be called into directly then you'd pretty much be done with it. But if you're doing something more walled-off and IPC-focused, you might use the window pointers as handles to pass back and forth in messages between your user process and our window manager. And in your standard paint functions, for example, at the end of the default draw you could send a 'paint' message to the process that created the window, sending the window's context pointer as a handle that could be passed back to the context functions through message wrappers. Unfortunately, though, this is just a rough thought. The way you integrate that is going to be really dependent on how your particular OS handles IPC.</p>

<p>But really, look at that, we basically have a windowing system! It needs some spit and shine here and there, a few nice functions (bitmap blitting and/or text, anyone?). But, if you've been following along, congratulations, you're basically there! And really, I'm pretty much going to call it at that. We're going to have one last entry tomorrow to cover a final efficiency improvement to our screen updates and maybe clean up some of the low-hanging fruit of minor features. But after that, I'm afraid to tell you, you're on your own to take the framework I've given you and make it into something your very own.</p>

<p>&nbsp;</p>

<hr/>

<p><a href="https://web.archive.org/web/20180715114750/https://www.github.com/jmarlin/wsbe">Code is at my github, you know what to do.</a> I actually got a pull request from a certain Badel2 the other day, and that was pretty cool to get. Definitely feel free to fork the project and start fixing all of the things you think I did wrong and evolve the code into your own work of GUI art.</p>