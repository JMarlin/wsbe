---
layout: post
title:  "8 - Getting Dirty"
author: "Joe Marlin"
---

<p>Screw introductions today. If you've made it this far, it means you already know what we're up to, here. And if you haven't, then... well, frankly, you probably have some reading to do if you really want to grok every detail of this article -- in short: Hi, nice to meet you, we're elbow-deep into writing a windowing system, find a seat in the back somewhere.</p>

<p>No. In lieu of telling you explicitly what our goals are going to be this week, I'll just show you the first change we're going to make today:</p>

<pre><code class="language-C">//The desktop overload for process_mouse
void Desktop_process_mouse(Desktop* desktop, uint16_t mouse_x,  
                           uint16_t mouse_y, uint8_t mouse_buttons) {

    //Capture the mouse location in order to draw it later
    desktop-&gt;mouse_x = mouse_x;
    desktop-&gt;mouse_y = mouse_y;

    //Do the old generic mouse handling
    Window_process_mouse((Window*)desktop, mouse_x, mouse_y, mouse_buttons);

    //Hint: The change is here.

    //And finally draw the hacky mouse, as usual
    Context_fill_rect(desktop-&gt;window.context, desktop-&gt;mouse_x,
                      desktop-&gt;mouse_y, 10, 10, 0xFF000000);
}
</code></pre>

<p>&nbsp;</p>

<p>This is what used to be there: <code>Window_paint((Window*)desktop);</code></p>

<p>Having the architecture of the system to this point in your mind, this should be at least a little perplexing, and, if not, I'll help you out: That one call is what's been causing our screen to redraw itself at all. Our desktop is intercepting the mouse, the mouse handlers were bouncing events down through the window hierarchy and causing state changes in the windows, and then we were finally reflecting those changes by forcing a full redraw of every window in that statement we just removed.</p>

<p>What the fuck. How you gonna draw, dude? </p>

<p>&nbsp;</p>

<h2 id="simplifysimplify">Simplify, Simplify</h2>

<p>Yes. Yes, obviously, we are going to still draw stuff to the screen. That's kind of our whole deal, here. And this may seem like madness, but this is to accomplish an end that we had already touched on way back: <em>We're drawing</em> <strong><em>way</em></strong> <em>too much junk at a time</em></p>

<p>"But wait!" I hear you say. "Didn't we already handle that when we spent all of that time implementing clipping?"</p>

<p>Oh, sweet summer child. That was only half of the battle. We may be limiting our drawing to only those things visible on screen, but we're still drawing everything any time we move the mouse. Every single window. Every single time. Specifically, we're even redrawing -- potentially -- whole swaths of screen that have absolutely zero changes in them. Literally just executing a time-consuming and complicated <code>v_mem[i] = v_mem[i];</code>, and doing so thousands of times.</p>

<p>That sounds like massively wasted cycles, kids. I don't want that. You don't want that. Or maybe you do. I don't really care, we're doing it my way regardless.</p>

<p>So how do we start fixing this problem? Well, we can start by putting our draw calls in the right place. This is the first and easiest fix for this wastage; We took the <code>Window_paint()</code> out of the desktop mouse handler because it doesn't make sense to do a whole redraw just because the mouse moved. Instead, we only want drawing to happen if something actually changed. And the only time things change are when we make them change. With the mouse. More specifically, when we raise or move a window (and other things, but we'll get there). </p>

<p>We've been doing all of our window raising and window moving rather trivially by just toggling things right inside of our <code>Window_process_mouse()</code> code. But we need the actions of raising and moving to start doing some heavier lifting, so we're going to start by spinning them out into their own <code>Window</code> methods. Let's start with the simpler of the two: Raising. To begin with, we're going to make things a little smarter by keeping track of a child's active window so that we don't have to pull it from the list to find it every time:</p>

<pre><code class="language-C">//The definition:
typedef struct Window_struct {  
    struct Window_struct* parent;
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    uint16_t flags;
    Context* context;
    struct Window_struct* drag_child;
    struct Window_struct* active_child; //This is new
    List* children;
    uint16_t drag_off_x;
    uint16_t drag_off_y;
    uint8_t last_button_state;
    WindowPaintHandler paint_function;
    WindowMousedownHandler mousedown_function;
} Window;

//And the tail end of the Window_init() method:

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
    window-&gt;active_child = (Window*)0; //Blank that thang

    return 1;
}
</code></pre>

<p>&nbsp;</p>

<p>With that out of the way, let's start on the actual raise method:</p>

<pre><code class="language-C">//A method to handle the raising of the window
void Window_raise(Window* window, uint8_t do_draw) {

    int i;
    Window *parent, *last_active;

    //If we don't have a parent, we don't have siblings,
    //we can't be drawn, we can't be raised
    if(!window-&gt;parent)
        return;

    //You can't raise the window that's already active
    parent = window-&gt;parent;

    if(parent-&gt;active_child == window)
        return;

    //Get a reference to the active window we're replacing
    last_active = parent-&gt;active_child;

    //Find this window in the list of its siblings to get its index
    for(i = 0; i &lt; parent-&gt;children-&gt;count; i++)
        if((Window*)List_get_at(parent-&gt;children, i) == window)
            break;

    //This is the exact same thing we were doing in the Window_process_mouse()
    //method to move the window to the top of the list
    List_remove_at(parent-&gt;children, i); //Pull window out of list
    List_add(parent-&gt;children, (void*)window); //Insert at the top

    //Make sure the parent knows this window is now the active one 
    parent-&gt;active_child = window;

    //Do a redraw if it was requested
    if(!do_draw)
        return;

    Window_paint(window, (List*)0, 1);

    //Make sure the old active window gets an updated title color 
    Window_update_title(last_active);
}
</code></pre>

<p>&nbsp;</p>

<p>Okay, a lot of that is pretty trivial, but then there's some weird shit at the end. It might not be surprising, in light of what I've been telling you, that we'd call a paint on the raised window when all is said and done. But: <strong>a)</strong> why are we only doing it optionally, <strong>b)</strong> what's with all of the extra arguments to <code>Window_paint()</code>, and <strong>c)</strong> we don't have any such method as <code>Window_update_title(last_active)</code>, buster.</p>

<p><strong>a)</strong> Not obvious unless you already knew where I was going, but when we get to implementing our move routine it's going to assume that moving a window is going to force the window to the top of the screen as well. Therefore, we'll be using this function at the beginning of the move function. But we're going to draw in the move function as well, and it would be wasteful to do it twice.</p>

<p><strong>b)</strong> We're going to be modifying the <code>Window_paint()</code> method a little further on to support some of the changes we're making. For the moment, rest assured that that's just causing our window to get redrawn the same way it would've worked last week. Important to this week's direction, though: Note that, since our <code>Window_paint()</code> calls, as of last week, draw the specified window and all of its children, that's all that's going to happen here. When a window gets raised, it pops to the top of the list and the only visual change it causes to the screen is that it needs to be drawn in front of anything that was occluding it, so we just redraw that window and everything in it. With that, we're already going in a good direction with the limited screen updates here. </p>

<p><strong>c)</strong> I want to have a little cheap fun this week and, since we're dealing with window raising, make active and inactive titlebar colors. And this here is the whole reason we saved the name of the old active window in the first place: If we're not going to be refreshing the whole screen, then we damn sure better at least make sure the border of the old window gets redrawn to show it's inactivity status. </p>

<p>C is fun and easy, so let's head there next.</p>

<pre><code class="language-C">//All this function does is redraw the specified window's border
//It acts a lot like Window_paint() in that it gets the window
//visibility clipping with Window_apply_bound_clipping(), but 
//then it only does the border draw and returns
void Window_update_title(Window* window) {

    int screen_x, screen_y;

    //Can't update the border of a window with no border
    if(window-&gt;flags &amp; WIN_NODECORATION)
        return;

    //Start by limiting painting to the window's visible area
    Window_apply_bound_clipping(window, 0, (List*)0);

    //Draw border
    Window_draw_border(window);

    Context_clear_clip_rects(window-&gt;context);
}
</code></pre>

<p>&nbsp;</p>

<p>To make that actually do anything, we need to first define an actual window inactive titlebar color:</p>

<pre><code class="language-C">//Feel free to play with this 'theme'
#define WIN_BGCOLOR     0xFFBBBBBB //A generic grey
#define WIN_TITLECOLOR  0xFFD09070 //A nice subtle blue
#define WIN_TITLECOLOR_INACTIVE 0xFF908080 //This is new, a darker shade 
#define WIN_BORDERCOLOR 0xFF000000 //Straight-up black
#define WIN_TITLEHEIGHT 31 
#define WIN_BORDERWIDTH 3
</code></pre>

<p>&nbsp;</p>

<p>And then update our border drawing function to be aware of it:</p>

<pre><code class="language-C">void Window_draw_border(Window* window) {

    //All of the other border drawing crapola
    //...

    //Fill in the titlebar background
    Context_fill_rect(window-&gt;context, screen_x + 3, screen_y + 3,
                      window-&gt;width - 6, 25,
                      window-&gt;parent-&gt;active_child == window ? 
                          WIN_TITLECOLOR : WIN_TITLECOLOR_INACTIVE);
                          //^That's our change
}
</code></pre>

<p>&nbsp;</p>

<p>And now we have active and inactive titlebar colors that will be updated. Wasn't that fun?</p>

<p>Back to real business: Let's ignore that <code>Window_paint()</code> method that clearly needs to be updated for a moment and look to the second event which would cause a visible change on the screen: Moving a window.</p>

<p>Let's stop for a moment and consider what actually changes when we move a window. Let's say we moved it twenty pixels down and ten pixels to the right. Clearly the window's x and y values get updated and subsequently we need to call a paint on that window, just like in the raise, to completely redraw it and its children at the new location. That's all well and good, but -- keeping in mind that since we're not doing whole-screen updates anymore -- what kind of havoc is left over on the screen once the window is redrawn at the new location and how do we deal with it?</p>

<p>It should be pretty easy to visualize the above in your head, but here's a quick visualization of what's happened in the above description -- we've updated the window coordinates and called paint on it, but have done no other updates to the screen:</p>

<p><img src="/web/20180715114810im_/http://trackze.ro/content/images/2016/10/test.png" alt=""/></p>

<p>Yeah. Look at that junk. As in any other screen drawing, if we don't clean up in some way, moving an object across the screen is just going to leave dirty copies in its wake. We don't want that. So what do you do when your screen is</p>

<p>&nbsp;</p>

<h2 id="dirtyandrekt">Dirty and Rekt</h2>

<p>The above was all just preparing you for this moment, the meat of today's discussion, the introduction of the <em>dirty rect</em>.</p>

<p>Basically, the idea is this: If we move a window or otherwise change something on screen (we'll see more examples later), we end up trashing a bit of the display which needs to be updated -- in this case the little L-shaped chunk of screen making up the difference of where our window was and where our window now is. That much is just stating the obvious.</p>

<p>To deal with this, we introduce the idea of a dirty region: Something we can pass to our window painting algorithm that says "okay, dude, this is the spot that was messed up, can you do me a favor and update just that spot?". Since we're in love with lists and rectangles a-la our list of clipping rectangles, we're going to implement this dirty region the same way. We'll start at the building blocks we need to get this dirty rect thing working for us in the window move method we're going to be implementing.</p>

<p>That means starting with those changes I mentioned to the <code>Window_paint()</code> method. Not a huge amount is going to be changing here, we're just going to be adding a list of dirty rectangles to the arguments and then passing it directly to <code>Window_apply_bound_clipping()</code></p>

<pre><code class="language-C">//Small updates to window paint
//Note that we have one other new argument. We'll get into that in a second
void Window_paint(Window* window, List* dirty_regions, uint8_t paint_children) {

    int i, j, screen_x, screen_y, child_screen_x, child_screen_y;
    Window* current_child;
    Rect* temp_rect;

    //Start by limiting painting to the window's visible area
    //NEW: We're now passing the list of dirty regions
    //apply_bound_clipping is going to do the real work with
    //them when we update it momentarily
    Window_apply_bound_clipping(window, 0, dirty_regions);

    //Set the context translation [OLD]
    screen_x = Window_screen_x(window);
    screen_y = Window_screen_y(window);

    //Border stuff [OLD]
    if(!(window-&gt;flags &amp; WIN_NODECORATION)) {

        Window_draw_border(window);
        screen_x += WIN_BORDERWIDTH;
        screen_y += WIN_TITLEHEIGHT;
        temp_rect = Rect_new(screen_y, screen_x,
                             screen_y + window-&gt;height - WIN_TITLEHEIGHT - WIN_BORDERWIDTH - 1, 
                             screen_x + window-&gt;width - (2*WIN_BORDERWIDTH) - 1);
        Context_intersect_clip_rect(window-&gt;context, temp_rect);
    }

    //Subtract children windows from clipping [OLD]
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

    //Translate and call paint callback [OLD]
    window-&gt;context-&gt;translate_x = screen_x;
    window-&gt;context-&gt;translate_y = screen_y;
    window-&gt;paint_function(window);

    //Reset context [OLD]
    Context_clear_clip_rects(window-&gt;context);
    window-&gt;context-&gt;translate_x = 0;
    window-&gt;context-&gt;translate_y = 0;

    //WHAT. I DIDN'T TELL YOU GUYS ABOUT THIS AT ALL
    if(!paint_children)
        return;

    //Alright, we were already painting children here. But this, too, 
    //is getting affected by this addition of dirty rects
    //If we're being asked to also redraw any children (don't ask why we
    //wouldn't be for now) we want to check if any of the dirty rects
    //actually include any portion of a child before we redraw it.
    //Because if none of the child was dirtied, why waste the time
    //even contemplating redrawing it?
    for(i = 0; i &lt; window-&gt;children-&gt;count; i++) {

        current_child = (Window*)List_get_at(window-&gt;children, i);

        //If we didn't pass dirty regions, then we just skip this 
        //and draw the child every time
        if(dirty_regions) {

            //Check to see if the child is affected by any of the
            //dirty region rectangles
            for(j = 0; j &lt; dirty_regions-&gt;count; j++) {

                temp_rect = (Rect*)List_get_at(dirty_regions, j);

                //Get the screen location of the child
                screen_x = Window_screen_x(current_child);
                screen_y = Window_screen_y(current_child);

                //Good 'ol intersect test
                if(temp_rect-&gt;left &lt;= (screen_x + current_child-&gt;width - 1) &amp;&amp;
                   temp_rect-&gt;right &gt;= screen_x &amp;&amp;
                   temp_rect-&gt;top &lt;= (screen_y + current_child-&gt;height - 1) &amp;&amp;
                   temp_rect-&gt;bottom &gt;= screen_y)
                    break;
            }

            //Skip drawing this child if no intersection was found
            if(j == dirty_regions-&gt;count)
                continue;
        }

        //Otherwise, recursively request the child to redraw its dirty areas
        Window_paint(current_child, dirty_regions, 1);
    }
}
</code></pre>

<p>&nbsp;</p>

<p>Yeah, I surprised you a bit there, but it looks scarier than it is. Really all we did was update our draw children loop to <strong>a)</strong> be skipped if our caller wills it (seriously, I <em>will</em> get to that) and <strong>b)</strong> check to see if any of the dirty areas in the list (if we got one) actually intersect a child before calling paint on it so as not to waste time updating anything that doesn't need updating.</p>

<p>But, the real <em>real</em> meat of this was hidden in that one piddling line up top: <code>Window_apply_bound_clipping(window, 0, dirty_regions);</code>. Yup, we have to drill down one level further. But once we do, we'll already have this whole dirty rect system completed and will just have to use it when we make our window move method, so let's get to it:</p>

<pre><code class="language-C">//Note that we added the list of dirty rects to the arguments
void Window_apply_bound_clipping(Window* window, int in_recursion, List* dirty_regions) {

    Rect *temp_rect, *current_dirty_rect, *clone_dirty_rect;
    int screen_x, screen_y, i;
    List* clip_windows;
    Window* clipping_window;

    //Calculate this window's basic bound rectangle [OLD]
    screen_x = Window_screen_x(window);
    screen_y = Window_screen_y(window);

    if((!(window-&gt;flags &amp; WIN_NODECORATION)) &amp;&amp; in_recursion) {

        //Limit client drawable area 
        screen_x += WIN_BORDERWIDTH;
        screen_y += WIN_TITLEHEIGHT;
        temp_rect = Rect_new(screen_y, screen_x,
                             screen_y + window-&gt;height - WIN_TITLEHEIGHT - WIN_BORDERWIDTH - 1, 
                             screen_x + window-&gt;width - (2*WIN_BORDERWIDTH) - 1);
    } else {

        temp_rect = Rect_new(screen_y, screen_x, screen_y + window-&gt;height - 1, 
                             screen_x + window-&gt;width - 1);
    }

    //If there's no parent (meaning we're at the top of the window tree)
    //then we just add our rectangle and exit
    //**HERE'S OUR CHANGE**: If we were passed a dirty region list, we first
    //clone those dirty rects into the clipping region and then intersect
    //the top-level window bounds against it so that we're limited to the
    //dirty region from the outset. Really kind of simple.
    if(!window-&gt;parent) {

        if(dirty_regions) {

            //Clone the dirty regions and put them into the clipping list
            for(i = 0; i &lt; dirty_regions-&gt;count; i++) {

                //Clone
                current_dirty_rect = (Rect*)List_get_at(dirty_regions, i);
                clone_dirty_rect = Rect_new(current_dirty_rect-&gt;top,
                                            current_dirty_rect-&gt;left,
                                            current_dirty_rect-&gt;bottom,
                                            current_dirty_rect-&gt;right);

                //Add
                Context_add_clip_rect(window-&gt;context, clone_dirty_rect);
            }

            //Finally, intersect this top level window against them
            Context_intersect_clip_rect(window-&gt;context, temp_rect);

        } else {

            //Otherwise, we just add the bounds of this top level window
            //to the (empty) visibility clipping area as we were doing before 
            Context_add_clip_rect(window-&gt;context, temp_rect);
        }

        return;
    }

    //The rest of the function is exactly the same as it was before
</code></pre>

<p>&nbsp;</p>

<p>That little chunk of code is really all we need to make this dirty rect thing work. To start with, if we didn't pass in a list of dirty rects, everything acts exactly the same when calling <code>Window_paint()</code> as it did before. Calculate what's visible of the window using <code>Window_apply_bound_clipping()</code>, then call the window's paint callback. The only change we're making now is, if passed a dirty rect list, we further limit our clipping to not just what's <em>visible</em> but what's both visible <em>and lies within the passed dirty rects</em>.</p>

<p>It might throw you that we're making a copy of each dirty rect in the list instead of passing them each directly into the clipping list, but because we're working in C we need to make sure we don't free objects that we're using. We might not be done with the rectangles in the dirty rect list, but they will be deleted when we clear the clipping rect after the painting is finished. So we copy them, because if the dirty rect list tried to access them after that we would have problems. Now you know.</p>

<p>But enough of my yapping. We have the dirty rect system implemented, let's see how we're going to actually use it by finally getting around to doing this window move function:</p>

<pre><code class="language-C">//We're wrapping this action so that we can handle any needed redraw when it happens
void Window_move(Window* window, int new_x, int new_y) {

    int i;
    int old_x = window-&gt;x;
    int old_y = window-&gt;y;
    Rect new_window_rect;
    List *replacement_list, *dirty_list, *dirty_windows;

    //We'll make the not-unreasonable rule that if a window is moved, it
    //must become the top-most window
    Window_raise(window, 0); //Raise it, but don't repaint it yet

    //To calculate the actual dirty region, we're going to just hijack the existing
    //clipping region modification operations. First we'll get the visible regions
    //of the original window position
    Window_apply_bound_clipping(window, 0, (List*)0);

    //Temporarily update the window position
    window-&gt;x = new_x;
    window-&gt;y = new_y;

    //Calculate the bounds of the moved window
    new_window_rect.top = Window_screen_y(window);
    new_window_rect.left = Window_screen_x(window);
    new_window_rect.bottom = new_window_rect.top + window-&gt;height - 1;
    new_window_rect.right = new_window_rect.left + window-&gt;width - 1;

    //Reset the window position (just bear with me)
    window-&gt;x = old_x;
    window-&gt;y = old_y;

    //Now, we'll get the *actual* dirty area by subtracting the new location of
    //the window 
    Context_subtract_clip_rect(window-&gt;context, &amp;new_window_rect);

    //Now that the context clipping tools made the list of dirty rects for us,
    //we can go ahead and steal the list it made for our own purposes
    //(yes, it would be cleaner to spin off our boolean rect functions so that
    //they can be used both here and by the clipping region tools, but I ain't 
    //got time for that junk. Feel free to do it yourself, though)
    if(!(replacement_list = List_new())) {

        Context_clear_clip_rects(window-&gt;context);
        return;
    }

    dirty_list = window-&gt;context-&gt;clip_rects;
    window-&gt;context-&gt;clip_rects = replacement_list;

    //Now, let's get all of the siblings that we overlapped before the move,
    //since only those siblings are the ones that would be affected by the
    //move and need to be redrawn
    dirty_windows = Window_get_windows_below(window-&gt;parent, window);

    //Now that we've done all we need with the window in its original
    //position, we can *actually* update its position for real tho
    window-&gt;x = new_x;
    window-&gt;y = new_y;

    //Now, we repaint any of the overlapped siblings we found earlier
    //specifically passing them the dirty list so that they can only
    //redraw those areas of themselves without wasting time on anything else
    //(removing the windows from the list as we go for convenience)
    while(dirty_windows-&gt;count)
        Window_paint((Window*)List_remove_at(dirty_windows, 0), dirty_list, 1);

    //The one thing that might still be dirty is the parent we're inside of
    //Look, that 'paint_children' flag came in really useful! In this case, we
    //already handled updating the parent's dirtied children (because its children
    //are our siblings), so we don't want to redraw them twice
    Window_paint(window-&gt;parent, dirty_list, 0);

    //We're done with the lists, so we can dump them
    while(dirty_list-&gt;count)
        free(List_remove_at(dirty_list, 0));

    free(dirty_list);
    free(dirty_windows);

    //And now, with all of the dirtiness fixed, we can do the actual paint of
    //the window (need to redraw its whole content including children, and
    //don't want to limit the drawing to a dirty list)
    Window_paint(window, (List*)0, 1);
}
</code></pre>

<p>&nbsp;</p>

<p>Ha, you thought I had actually described every method we would need to write to implement this function. As usual, I lied. We don't have <code>Window_get_windows_below()</code>. Thankfully, it's literally just <code>Window_get_windows_above()</code> in reverse, so it's easy: </p>

<pre><code class="language-C">//Used to get a list of windows which the passed window overlaps
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
    //child list at lower indexes than the passed window
    //We start by finding the passed child in the list
    for(i = parent-&gt;children-&gt;count - 1; i &gt; -1; i--)
        if(child == (Window*)List_get_at(parent-&gt;children, i))
            break;

    //Now we just need to add the remaining items in the list
    //to the output (IF they overlap, of course)
    for(; i &gt; -1; i--) {

        current_window = List_get_at(parent-&gt;children, i);

        //Our good old rectangle intersection logic
        if(current_window-&gt;x &lt;= (child-&gt;x + child-&gt;width - 1) &amp;&amp;
           (current_window-&gt;x + current_window-&gt;width - 1) &gt;= child-&gt;x &amp;&amp;
           current_window-&gt;y &lt;= (child-&gt;y + child-&gt;height - 1) &amp;&amp;
           (current_window-&gt;y + current_window-&gt;height - 1) &gt;= child-&gt;y)
            List_add(return_list, current_window); //Insert the overlapping window
    }

    return return_list; 
}
</code></pre>

<p>&nbsp;</p>

<p>Boom. Bam. That's that. We have working (I promise) methods for raising and moving that clean up after themselves without having to redraw every pixel on the screen. There's one more related thing I want to address today after this, but before we get there we need to plug them into the mouse handling method (probably helpful if everything we just wrote doesn't end up as dead code) and do a little cleanup. </p>

<p>The cleanup first: We have a minor bug in our context regarding clipping. Right now our context assumes, when drawing, that if there are no rects in the clipping rect collection that we don't want to do any visibility clipping and just draw everything directly to the screen, unaltered. But that assumption doesn't hold if, when adding, removing and intersecting clipping rects to build a window's visibility clipping, we end up with an empty set. In <em>that</em> case, we would want to actually do the opposite: not draw a damn thing. So, instead of assuming an empty list means no clipping, we'll actually add a flag that indicates whether clipping is on or not. We'll clear it when we clear the clipping rects, and we'll flip it on whenever we add, subtract or intersect a clipping rect:</p>

<pre><code class="language-C">//Start by adding the flag to the object definition:
typedef struct Context_struct {  
    uint32_t* buffer; 
    uint16_t width; 
    uint16_t height; 
    int translate_x; 
    int translate_y;
    List* clip_rects;
    uint8_t clipping_on; //[NEW]
} Context;
</code></pre>

<pre><code class="language-C">//Add the new flag to new context initialization
//Here's the tail end of Context_new():

    //Finish assignments
    context-&gt;width = width; 
    context-&gt;height = height; 
    context-&gt;buffer = buffer;
    context-&gt;clipping_on = 0; [NEW]

    return context;
}
</code></pre>

<pre><code class="language-C">//Update that core rectangle drawing function to use the flag
//This is the tail end of Context_fill_rect():

    //If there are clipping rects, draw the rect clipped to
    //each of them. Otherwise, draw unclipped (clipped to the screen)
    if(context-&gt;clip_rects-&gt;count) {

        for(i = 0; i &lt; context-&gt;clip_rects-&gt;count; i++) {    

            clip_area = (Rect*)List_get_at(context-&gt;clip_rects, i);
            Context_clipped_rect(context, x, y, width, height, clip_area, color);
        }
    } else {

        //Here's the change: Just check to see if clipping is on before drawing
        //the rect unclipped so that, if it is and the list is empty, we draw
        //nothing instead
        if(!context-&gt;clipping_on) {

            screen_area.top = 0;
            screen_area.left = 0;
            screen_area.bottom = context-&gt;height - 1;
            screen_area.right = context-&gt;width - 1;
            Context_clipped_rect(context, x, y, width, height, &amp;screen_area, color);
        }
    }
}
</code></pre>

<p>Now we just have to flip that flag off and on:</p>

<pre><code class="language-C">//Turn clipping on whenever we intersect a new clipping rect:
void Context_intersect_clip_rect(Context* context, Rect* rect) {

    context-&gt;clipping_on = 1;

    //...
</code></pre>

<pre><code class="language-C">//Turn it on when we subtract one
//Since Context_add_clip_rect calls this, we don't need to update that one
void Context_subtract_clip_rect(Context* context, Rect* rect) {

    context-&gt;clipping_on = 1;

    //...
</code></pre>

<p>And flip it off when we clear</p>

<pre><code class="language-C">void Context_clear_clip_rects(Context* context) {

    context-&gt;clipping_on = 0;

    //...
</code></pre>

<p>&nbsp;</p>

<p>Bug fixed.</p>

<p>Moving on, let's get those move and raise functions inserted into <code>Window_process_mouse()</code>:</p>

<pre><code class="language-C">//It's a good chunk of code, but really only two lines out of the whole thing 
//below actually changed
void Window_process_mouse(Window* window, uint16_t mouse_x,  
                          uint16_t mouse_y, uint8_t mouse_buttons) {

    int i, inner_x1, inner_y1, inner_x2, inner_y2;
    Window* child;

    //Look through children [OLD]
    for(i = window-&gt;children-&gt;count - 1; i &gt;= 0; i--) {

        child = (Window*)List_get_at(window-&gt;children, i);

        //Skip if we're not over the window [OLD]
        if(!(mouse_x &gt;= child-&gt;x &amp;&amp; mouse_x &lt; (child-&gt;x + child-&gt;width) &amp;&amp;
           mouse_y &gt;= child-&gt;y &amp;&amp; mouse_y &lt; (child-&gt;y + child-&gt;height))) 
            continue;

        //Dragging titlebar? [OLD]
        if(mouse_buttons &amp;&amp; !window-&gt;last_button_state) {

            //Raise happens whenever we click inside a child
            //HERE'S CHANGE NUMBER ONE, call the method instead of
            //manipulating the list manually
            Window_raise(child, 1);

            //Check for drag [OLD]
            if(!(child-&gt;flags &amp; WIN_NODECORATION) &amp;&amp; 
                mouse_y &gt;= child-&gt;y &amp;&amp; mouse_y &lt; (child-&gt;y + 31)) {

                window-&gt;drag_off_x = mouse_x - child-&gt;x;
                window-&gt;drag_off_y = mouse_y - child-&gt;y;
                window-&gt;drag_child = child;

                break;
            }
        }

        //Found a target, forward the mouse event [OLD]
        Window_process_mouse(child, mouse_x - child-&gt;x, mouse_y - child-&gt;y, mouse_buttons); 
        break;
    }

    //End drag if mouse is up [OLD]
    if(!mouse_buttons)
        window-&gt;drag_child = (Window*)0;

    //Update drag window to match the mouse if we have an active drag window
    if(window-&gt;drag_child) {

        //HERE'S THE SECOND CHANGE calling the move function instead of 
        //manually setting the window coordinates
        Window_move(window-&gt;drag_child, mouse_x - window-&gt;drag_off_x,
                    mouse_y - window-&gt;drag_off_y);
    }

    //If we didn't find a target, then we ourselves are the target [OLD]
    if(window-&gt;mousedown_function &amp;&amp; mouse_buttons &amp;&amp; !window-&gt;last_button_state) 
        window-&gt;mousedown_function(window, mouse_x, mouse_y);

    //Update the stored mouse button state [OLD]
    window-&gt;last_button_state = mouse_buttons;
}
</code></pre>

<p>&nbsp;</p>

<p>There. Now any time we detect a raise or a window move, the screen updates. With that finished, our initial goal is done: we replaced the functionality of that full screen redraw we were firing at the end of every <code>Desktop_process_mouse()</code>. </p>

<p>One thing you might have noticed is that we're not cleaning up after the mouse. I'm leaving that one this week though, because doing that actually makes our work this week really visible, so you can appreciate what's going on a little better. Since the mouse is leaving a trail of dirty pixels behind it, you'll be able to clearly see how only those minimal areas that a window move or raise actually affect will get updated and redrawn instead of the whole screen. Now we're looking pretty pro. So it's time for just one more thing:</p>

<p>&nbsp;</p>

<h2 id="thepointisinvalid">The Point is Invalid</h2>

<p>You might remember, wayyyy up there at the top that I told you there was another case in which we would need to use dirty rects to redraw a portion of the screen. And that case is the generic case. (for now) Raising and moving windows are the only things that our window manager knows of that affect the screen. But once our windows and controls start coming into their own, they're going to need to reflect changes that might happen inside of themselves. So we need a general way for a window to tell the window manager "yo, I made some changes here, can you do me the favor of giving me a repaint?".</p>

<p>We call that invalidating. And we're going to write one more quick window method that we can use to do so. Really all it needs to do is take a rectangular region and turn it into a dirty rect list (we could just always redo the whole window, but it's a little more efficient for the window to be able to say 'I only changed this bit here, so just limit my redrawing to that spot') and then use it to fire a window paint:</p>

<pre><code class="language-C">//Request a repaint of a certain region of a window
void Window_invalidate(Window* window, int top, int left, int bottom, int right) {

    List* dirty_regions;
    Rect* dirty_rect;

    //This function takes coordinates in terms of window coordinates
    //So we need to convert them to screen space 
    int origin_x = Window_screen_x(window);
    int origin_y = Window_screen_y(window);
    top += origin_y;
    bottom += origin_y;
    left += origin_x;
    right += origin_x;

    //Attempt to create a new dirty rect list 
    if(!(dirty_regions = List_new()))
        return;

    //Attempt to create a new rect based on the function input
    if(!(dirty_rect = Rect_new(top, left, bottom, right))) {

        free(dirty_regions);
        return;
    }

    //Attempt to put that rect into the list
    if(!List_add(dirty_regions, dirty_rect)) {

        free(dirty_regions);
        free(dirty_rect);
        return;
    }

    //And finally, just fire a paint
    //Note that we yet again use that 'paint_children' flag, in this
    //case because a change to the body of a window shouldn't and
    //wouldn't affect it's children at all (eg: if we changed the
    //desktop wallpaper, there would be no reason to repaint the windows
    //on the desktop). See, I told you I would tell you why that's there!
    Window_paint(window, dirty_regions, 0);

    //Finally, clean up the dirty rect list
    List_remove_at(dirty_regions, 0);
    free(dirty_regions);
    free(dirty_rect); 
}
</code></pre>

<p>&nbsp;</p>

<p>Yeah, really it's just a convenience function for requesting a specific case of repaint. But it's going to be useful enough that it deserves to exist. For instance, right now whenever we click that button we made last time, we don't see the changes until we drag another window over top of it to force a repaint. Now we can use our invalidate function to force the button to get redrawn whenever its toggle status changes:</p>

<pre><code class="language-C">void Button_mousedown_handler(Window* button_window, int x, int y) {

    Button* button = (Button*)button_window;

    button-&gt;color_toggle = !button-&gt;color_toggle;

    //Since the button has visibly changed state, we need to invalidate the
    //area that needs updating
    Window_invalidate((Window*)button, 0, 0,
                      button-&gt;window.height - 1, button-&gt;window.width - 1);
}
</code></pre>

<p>&nbsp;</p>

<p>And now that we have that going on, except for the nasty trail that the mouse leaves behind it, our windows behave just the way we would expect them to -- but a whole lot more efficiently. </p>

<p>&nbsp;</p>

<h2 id="thepenultimateend">The Penultimate End</h2>

<p>Look, dude. We're really really close to that being that. I mean, you personally have a lot to do when it comes to making what I've been showing you work in your personal project. We're missing a lot of seemingly simple crap like fonts and bitmaps and anything that doesn't involve simple rectangles somehow. But really, that's all icing. And most of it is actually pretty trivial to implement. Doing clipped bitmap drawing is just like clipped rectangle drawing but with blitting rectangular sub-sections of the image into the screen instead of drawing sub-rects. Frankly, I haven't addressed any of that because, seriously, none of that is integral to understanding how an old-school non-composited windowing system operates.  </p>

<p>On the flipside... at this point, we've kind of covered most of the critical bits of that last thing. Like, seriously. We've got a recursive window hierarchy with the option of custom extensible <code>Window</code>-derived windows and controls. We've got (reeeeeallly basic) mouse event dispatching. And we have all of the critical components of efficient asynchronous screen updating using dirty rectangles and visibility clipping that an old-school windowing system needs to do it's thing pretty closely to how the real commercial products used to and sometimes still do do it.</p>

<p>So, we're getting close to the end. Next week we're going to show the possibilities of the system we've spent these last couple of months building by adding a couple of small features (get ready for a real mouse, better mouse event handling and some simple bitmap font rendering!) in order to make the final piece of this thing a possibility: We're going to use everything we've built so far to implement a simple GUI four-banger calculator app!</p>

<p>But even then, this code will just be a starting point. It has an absolute truckload of minor bugs and missing functionalities, and it's up to you to mold this generic clay armature I've given you into something <em>you</em> really want to play with. And there's a lot of potential there.</p>

<p>Can't wait to see you guys next week to wrap this thing up!</p>

<p>&nbsp;</p>

<hr/>

<p>As always, <a href="https://web.archive.org/web/20180715114810/https://github.com/JMarlin/wsbe">you can find all of the code free for you to steal or run in your browser via the magic of emscripten right over here at my github</a>.</p>