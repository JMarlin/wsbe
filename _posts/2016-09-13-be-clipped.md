---
layout: post
title:  "5 - Be Clipped"
author: "Joe Marlin"
---

<p>Hey, all you groovy fellas and dames, welcome to yet another week of WSBE. I'll lay it down quick: As it stands, we have a desktop object that holds some window objects and a drawing context object<sup>[<a href=" " title="capable of drawing only the bitchinest of rectangles, and now even horizontal and vertical lines! Whoa!">1</a>]</sup>. We set up our desktop object to handle mouse events and do a redraw of its windows whenever it gets one. Then, yesterday, we decided to get a little smarter and begin implementing a simple clipping system made out of rectangles and temporarily replaced our old desktop drawing code to visualize how our function for splitting rectangles into the collection of clipping rectangles works. </p>

<p>Today, we'll finish the clipping framework we started in part 4<sup>[<a href=" " title="as if you couldn't already tell they were companion pieces by my exceedingly clever reference to that classic pair of Elmore Leonard books/movies">2</a>]</sup> by updating our basic drawing functions to actually take the clipping rectangles into account, extending our clipping rectangle code a little so that we can both add <em>and</em> remove rectangular regions from the clipping rectangles, and finally use both of those functions to do what we'd originally come here for in part 4: draw our desktop without wasting time drawing bits of windows that won't be visible. And, as a cherry on top, we'll start adding just a touch of chrome to our windows. I'm sure you're already warmed up from last time, so let's shut my yap and dive on in already.</p>

<p>&nbsp;</p>

<h2 id="part1rectangleinarectangle">Part 1: Rectangle in a Rectangle</h2>

<p>Our context has this list of clipping rectangles now and we can add rectangles to it and remove rectangles from it. Whoopee, what good does that actually do us? The idea, here, is that when we call our drawing functions like <code>Context_fill_rect()</code>, we want them to only render those pixels that are inside of any of the rectangles in <code>context-&gt;clip rects</code>. In that way, by setting up the clipping region to only include those areas of a window that aren't going to be covered up by something else<sup>[<a href=" " title="don't worry about the specifics of that part yet, we'll be addressing that in part 2 below">3</a>]</sup> we will only end up painting visible pixels when we call on the window to paint itself.</p>

<p>The most naive approach here would be to do our standard drawing but with a check added to test, when we're about to put a pixel in the framebuffer, if that pixel lands within one of the clipping rectangles and skip putting it on screen if it doesn't. This, however, isn't great since we still have to do some work for every single pixel in the shape, including those that are potentially completely invisible. Instead<sup>[<a href=" " title="and this can get way, way more complex for realistically advanced implementations that actually do full arbitrary polygon clipping, though it does generalize better">4</a>]</sup>, we're going to modify our drawing algorithms slightly so that, for each clipping rectangle, we will pre-calculate the shape that results from trimming the bigger shape by the current clip rectangle and then finally draw that sub-shape.</p>

<p>Thankfully, this is crazy easy for drawing rectangles since to calculate the bit of a rectangle that sits inside of another rectangle you just have to clamp the edges of the one the the edges of the other -- just like we already do to make sure none our rectangle gets drawn offscreen<sup>[<a href="https://web.archive.org/web/20180715114707/http://onlinelibrary.wiley.com/doi/10.1111/1467-8659.1450275/pdf" title="We're keeping things simple and rectangle-based because we want to linger on clipping just long enough to understand its role in a windowing system. If you want to start getting more complex, though, and are willing to pay to get over a $6 paywall, the article you can find by clicking on this footnote describes an algorithm for clipped bresenham line drawing that you can challenge yourself to implement in our framework. As a bonus, it actually includes a C implementation in the text.">5</a>]</sup>. Every drawing function we have so far -- rectangle outlines, horizontal lines, vertical lines -- rests on our basic filled rectangle function. Therefore, if we make a new, secondary filled rectangle function that will draw things as limited by a provided rectangle and then use that new function at the core of an updated version of our old rectangle drawing function that calls it for each clipping rect, we can quickly make our drawing functions clip the way we want:</p>

<pre><code class="language-C">//This is our new function. It basically acts exactly like the original 
//Context_fill_rect() except that it limits its drawing to the bounds
//of a Rect instead of to the bounds of the screen.
void Context_clipped_rect(Context* context, int x, int y, unsigned int width,  
                          unsigned int height, Rect* clip_area, uint32_t color) {

    int cur_x;
    int max_x = x + width;
    int max_y = y + height;

    //Make sure we don't go outside of the clip region:
    if(x &lt; clip_area-&gt;left)
        x = clip_area-&gt;left;

    if(y &lt; clip_area-&gt;top)
        y = clip_area-&gt;top;

    if(max_x &gt; clip_area-&gt;right + 1)
        max_x = clip_area-&gt;right + 1;

    if(max_y &gt; clip_area-&gt;bottom + 1)
        max_y = clip_area-&gt;bottom + 1;

    //Draw the rectangle into the framebuffer line-by line
    //just as we've always done
    for(; y &lt; max_y; y++) 
        for(cur_x = x; cur_x &lt; max_x; cur_x++) 
            context-&gt;buffer[y * context-&gt;width + cur_x] = color;
}

//And here is the heavily updated Context_fill_rect that calls on the new
//Context_clipped_rect() above for each Rect in the context-&gt;clip_rects
void Context_fill_rect(Context* context, int x, int y,  
                      unsigned int width, unsigned int height, uint32_t color) {

    int start_x, cur_x, cur_y, end_x, end_y;
    int max_x = x + width;
    int max_y = y + height;
    int i;
    Rect* clip_area;
    Rect screen_area;

    //If there are clipping rects, draw the rect clipped to
    //each of them. Otherwise, draw unclipped (clipped to the screen)
    if(context-&gt;clip_rects-&gt;count) {

        for(i = 0; i &lt; context-&gt;clip_rects-&gt;count; i++) {    

            clip_area = (Rect*)List_get_at(context-&gt;clip_rects, i);
            Context_clipped_rect(context, x, y, width, height, clip_area, color);
        }
    } else {

        //Since we have no rects, pass a fake 'screen' one
        screen_area.top = 0;
        screen_area.left = 0;
        screen_area.bottom = context-&gt;height - 1;
        screen_area.right = context-&gt;width - 1;
        Context_clipped_rect(context, x, y, width, height, &amp;screen_area, color);
    }
}
</code></pre>

<p>And that's really it. Now setting up/clearing clipping will actually have an effect on our drawing. Since all of our other drawing primitives ultimately call on <code>Context_fill_rect()</code> to do their work, they'll all end up being clipped.</p>

<p>There's one more quick change to our <code>Context</code> class that I had talked about making at the end of last week's article, though. It's really cool that we can, without overlapping, add rectangles to the clipping rectangle collection. But, in the process of building the visibility clipping for a window as I had described it in passing last week<sup>[<a href=" " title="that is: clear the clipping, add a rectangle for our window's bounds, subtract the bounds of any windows above it.">6</a>]</sup>, we need to be able to also <em>remove</em> rectangular regions from our clipping. But, as I also mentioned last time, making that happen is going to be pretty trivial. Since we already punch out an area for a rectangle in our <code>Context_add_clip_rect()</code> function, we just need to update it to <em>not</em> add the new rectangle to the collection when it's done clipping and it suddenly becomes a subtraction method:</p>

<pre><code class="language-C">//This is literally the exact same function but renamed and with a minor change at
//the very very end
//split all existing clip rectangles against the passed rect
void Context_subtract_clip_rect(Context* context, Rect* subtracted_rect) {

    //Check each item already in the list to see if it overlaps with
    //the new rectangle
    int i, j;
    Rect* cur_rect;
    List* split_rects;

    for(i = 0; i &lt; context-&gt;clip_rects-&gt;count; ) {

        cur_rect = List_get_at(context-&gt;clip_rects, i);

        //Standard rect intersect test (if no intersect, skip to next)
        //see here for an example of why this works:
        //http://stackoverflow.com/questions/306316/determine-if-two-rectangles-overlap-each-other#tab-top
        if(!(cur_rect-&gt;left &lt;= subtracted_rect-&gt;right &amp;&amp;
           cur_rect-&gt;right &gt;= subtracted_rect-&gt;left &amp;&amp;
           cur_rect-&gt;top &lt;= subtracted_rect-&gt;bottom &amp;&amp;
           cur_rect-&gt;bottom &gt;= subtracted_rect-&gt;top)) {

            i++;
            continue;
        }

        //If this rectangle does intersect with the new rectangle, 
        //we need to split it
        List_remove_at(context-&gt;clip_rects, i); //Original will be replaced w/splits
        split_rects = Rect_split(cur_rect, subtracted_rect); //Do the split
        free(cur_rect); //We can throw this away now, we're done with it

        //Copy the split, non-overlapping result rectangles into the list 
        while(split_rects-&gt;count) {

            cur_rect = (Rect*)List_remove_at(split_rects, 0);
            List_add(context-&gt;clip_rects, cur_rect);
        }

        //Free the empty split_rect list 
        free(split_rects);

        //Since we removed an item from the list, we need to start counting over again 
        //In this way, we'll only exit this loop once nothing in the list overlaps 
        i = 0;    
    }

    //[~!~] Here we removed List_add();
}

//And with that, our original Context_add_clip_rect() function gets replaced
//by a function that just calls the 'new' subtraction function and then adds
//the passed rectangle into the collection
void Context_add_clip_rect(Context* context, Rect* added_rect) {

    Context_subtract_clip_rect(context, added_rect);

    //Now that we have made sure none of the existing rectangles overlap
    //with the new rectangle, we can finally insert it 
    List_add(context-&gt;clip_rects, added_rect);
}
</code></pre>

<p>Really absurdly simple, I don't know why we didn't just do that in the first place<sup>[<a href=" " title="I should really foreshadow this kind of glossing-over, so in this case I will: We actually still have one more kind of boolean operation to add to our clipping region before this project is finished, that being intersection. But for the moment we don't need it.">7</a>]</sup>.</p>

<p>To finish this section up, I just want to make some changes to the desktop drawing function so that you can really see how this new clipping ends up working, and that it <em>is</em> in fact working. To do that, I'm going to continue to forgo actually painting the windows. Instead, we're going to draw a loud image to the screen, then add a rectangle for the desktop to the clipping region, use our 'new' subtraction function to subtract each of our windows from the clipping region, and finally do our usual call to <code>Context_fill_rect()</code> to draw the desktop color. This way, we'll get a good example of our clipping at work in the form of the first image showing through the desktop where our windows would be:</p>

<pre><code class="language-C">//Paint the desktop 
void Desktop_paint(Desktop* desktop) {

    //Loop through all of the children and call paint on each of them 
    unsigned int i;
    Window* current_window;
    Rect* temp_rect;

    //Clear the screen quadrants each to a different bright color
    Context_fill_rect(desktop-&gt;context, 0, 0, desktop-&gt;context-&gt;width/2,
                      desktop-&gt;context-&gt;height/2, 0xFF0000FF);
    Context_fill_rect(desktop-&gt;context, desktop-&gt;context-&gt;width/2, 0, desktop-&gt;context-&gt;width/2,
                      desktop-&gt;context-&gt;height/2, 0xFF00FF00); 
    Context_fill_rect(desktop-&gt;context, 0, desktop-&gt;context-&gt;height/2, desktop-&gt;context-&gt;width/2,
                      desktop-&gt;context-&gt;height/2, 0xFF00FFFF);
    Context_fill_rect(desktop-&gt;context, desktop-&gt;context-&gt;width/2, desktop-&gt;context-&gt;height/2,
                      desktop-&gt;context-&gt;width/2, desktop-&gt;context-&gt;height/2, 0xFFFF00FF);                                            

    //Add a rect for the desktop to the context clipping region
    temp_rect = Rect_new(0, 0, desktop-&gt;context-&gt;height - 1, desktop-&gt;context-&gt;width - 1);
    Context_add_clip_rect(desktop-&gt;context, temp_rect);

    //Now subtract each of the window rects from the desktop rect
    for(i = 0; (current_window = (Window*)List_get_at(desktop-&gt;children, i)); i++) {

        temp_rect = Rect_new(current_window-&gt;y, current_window-&gt;x,
                             current_window-&gt;y + current_window-&gt;height - 1,
                             current_window-&gt;x + current_window-&gt;width - 1);
        Context_subtract_clip_rect(desktop-&gt;context, temp_rect);
        free(temp_rect); //In an add, the allocated rectangle object gets pushed into the
                         //clipping collection and then eventually freed when we clear it. 
                         //We specifically don't add the rectangle to the collection on a 
                         //subtract, though, so we have to make sure to free it ourselves.
    }

    //Fill the desktop (shows the clipping)
    Context_fill_rect(desktop-&gt;context, 0, 0, desktop-&gt;context-&gt;width,
                      desktop-&gt;context-&gt;height, 0xFFFF9933);

    //Reset the context clipping for next render
    Context_clear_clip_rects(desktop-&gt;context);

    //simple rectangle for the mouse
    Context_fill_rect(desktop-&gt;context, desktop-&gt;mouse_x, 
                      desktop-&gt;mouse_y, 10, 10, 0xFF000000);
}
</code></pre>

<p>To understand everything a little better, here's a visual representation of what exactly is happening to the clipping rectangles during the desktop drawing function: <br/>
<img src="/web/20180715114707im_/http://trackze.ro/content/images/2016/09/gif2.gif" alt=""/></p>

<p>There, you can very clearly see how, after the desktop rectangle gets added into the blank clipping region, each window in turn punches a hole into the existing clipping rectangles using our simple rectangle splitting algorithm, just as I described it at the end of the last article.</p>

<p>Today's code is built in two parts that are completely buildable each on their own, because I wanted to do this demonstration of clipped drawing before we use it to result drawing all of our windows so you can more clearly see the new clipping magic at play. So feel free to fire up the compiler and give this bad boy a spin. For even more clarity, here's a super slowed-down depiction of how the filled rect is getting drawn on each desktop draw:  </p>

<p><img src="/web/20180715114707im_/http://trackze.ro/content/images/2016/09/gif1.gif" alt=""/></p>

<p>Okay, that's fun and all, but it's time to cover up all of our hard work and use this new clipped drawing functionality to actually paint our windows<sup>[<a href=" " title="Enjoy this little novelty while you can because, spoiler, drawing our windows with clipping is going to end up looking exactly the same as it did two articles ago.">8</a>]</sup></p>

<p>&nbsp;</p>

<h2 id="part2hidingtheartwork">Part 2: Hiding the Artwork</h2>

<p>Well, we got the fun spectacle of our working rudimentary clipping out of the way, so I guess it's time to put the hood back over the engine. We're going to focus on taking what we've just built, and rewriting our window drawing to take advantage of it. For double-fun<sup>[<a href=" " title="and frankly because I can understand the impatience of spending a month and a half looking at colored squares">9</a>]</sup>, we're going to also update our window painting to look a little more window-like.</p>

<p>Basically, we just need to update the drawing function in our Desktop class to do the use clipping in the same way we just did to punch window holes out of the background, but for each window. However, with the windows there's also one minor twist: Ordering.</p>

<p>Here's the thing: The desktop was super trivial because we know that all of the windows will already be above it and therefore occlude it. But with the windows it's important to note that we only want to subtract from a clipping rectangle where the window is occluded. And a window can only become occluded by the windows <em>above</em> it, not below. In our case, that means the windows further into the list than the window being drawn. So, before we start updating our desktop painting we'll write a quick function to get a list of windows above and overlapping a given window<sup>[<a href=" " title="You may wonder why we're doing this separately when we're already going through the list in order in our desktop's window painting loop and are clearly dealing with depth ordering there. Why the added complexity of re-looking up the window ordering? As a hint at things to come, I'll tell you that the whole-screen update in the desktop draw function is eventually going to go bye-bye.">10</a>]</sup>:</p>

<pre><code class="language-C">//Used to get a list of windows overlapping the passed window
List* Desktop_get_windows_above(Desktop* desktop, Window* window) {

    int i;
    Window* current_window;
    List* return_list;

    //Attempt to allocate the output list
    if(!(return_list = List_new()))
        return return_list;

    //We just need to get a list of all items in the
    //child list at higher indexes than the passed window
    //We start by finding the passed child in the list
    for(i = 0; i &lt; desktop-&gt;children-&gt;count; i++)
        if(window == (Window*)List_get_at(desktop-&gt;children, i))
            break;

    //Now we just need to add the remaining items in the list
    //to the output (IF they overlap, of course)
    //NOTE: As a bonus, this will also automatically fall through
    //if the window wasn't found
    for(; i &lt; desktop-&gt;children-&gt;count; i++) {

        current_window = (Window*)List_get_at(desktop-&gt;children, i);

        //Our good old rectangle intersection logic
        if(current_window-&gt;x &lt;= (window-&gt;x + window-&gt;width - 1) &amp;&amp;
           (current_window-&gt;x + current_window-&gt;width - 1) &gt;= window-&gt;x &amp;&amp;
           current_window-&gt;y &lt;= (window-&gt;y + window-&gt;height - 1) &amp;&amp;
           (window-&gt;y + window-&gt;height - 1) &gt;= window-&gt;y)
            List_add(return_list, current_window); //Insert the overlapping window
    }

    return return_list; 
}
</code></pre>

<p>As far as preparing for our window clipping, that's about that. We're ready to update the main paint routine in <code>Desktop</code>. It's been stated before, but I'll reiterate: Our process is going to be first to do exactly what we did to the desktop in part 1 (add desktop rect, subtract all window rects, paint). Then we're going to go through just about the same process to clip and draw each window using the <code>Window_paint()</code> call, but instead of adding the rect of the current window and then subtracting the rect of <em>every</em> window we're going to only subtract the rectangles of the windows returned by the <code>Desktop_get_windows_above()</code> function we just wrote.</p>

<p>This is going to be another really long one, but it's nothing you haven't seen before. It's just rather verbose:</p>

<pre><code class="language-C">//Paint the desktop 
void Desktop_paint(Desktop* desktop) {

    //Loop through all of the children and call paint on each of them 
    unsigned int i, j;
    Window *current_window, *clipping_window;
    Rect* temp_rect;     
    List* clip_windows;                              

    //Do the clipping for the desktop just like before
    //Add a rect for the desktop
    temp_rect = Rect_new(0, 0, desktop-&gt;context-&gt;height - 1, desktop-&gt;context-&gt;width - 1);
    Context_add_clip_rect(desktop-&gt;context, temp_rect);

    //Now subtract each of the window rects from the desktop rect
    for(i = 0; i &lt; desktop-&gt;children-&gt;count; i++) {

        current_window = (Window*)List_get_at(desktop-&gt;children, i);

        temp_rect = Rect_new(current_window-&gt;y, current_window-&gt;x,
                             current_window-&gt;y + current_window-&gt;height - 1,
                             current_window-&gt;x + current_window-&gt;width - 1);
        Context_subtract_clip_rect(desktop-&gt;context, temp_rect);
        free(temp_rect); //Rect doesn't end up in the clipping list
                         //during a subtract, so we need to get rid of it
    }

    //Fill the desktop
    Context_fill_rect(desktop-&gt;context, 0, 0, desktop-&gt;context-&gt;width, desktop-&gt;context-&gt;height, 0xFFFF9933);

    //Reset the context clipping 
    Context_clear_clip_rects(desktop-&gt;context);

    //Now we do a similar process to draw each window
    for(i = 0; i &lt; desktop-&gt;children-&gt;count; i++) {

        current_window = (Window*)List_get_at(desktop-&gt;children, i);

        //Create and add a base rectangle for the current window
        temp_rect = Rect_new(current_window-&gt;y, current_window-&gt;x,
                             current_window-&gt;y + current_window-&gt;height - 1,
                             current_window-&gt;x + current_window-&gt;width - 1);
        Context_add_clip_rect(desktop-&gt;context, temp_rect);

        //Now, we need to get and clip any windows overlapping this one
        clip_windows = Desktop_get_windows_above(desktop, current_window);

        while(clip_windows-&gt;count) {

           //We do the different loop above and use List_remove_at because
           //we want to empty and destroy the list of clipping widows
           clipping_window = (Window*)List_remove_at(clip_windows, 0);

           //Make sure we don't try and clip the window from itself
           if(clipping_window == current_window)
               continue;

           //Get a rectangle from the window, subtract it from the clipping 
           //region, and dispose of it
           temp_rect = Rect_new(clipping_window-&gt;y, clipping_window-&gt;x,
                                clipping_window-&gt;y + clipping_window-&gt;height - 1,
                                clipping_window-&gt;x + clipping_window-&gt;width - 1);
           Context_subtract_clip_rect(desktop-&gt;context, temp_rect);
           free(temp_rect);
        }

        //Now that we've set up the clipping, we can do the
        //normal (but now clipped) window painting
        Window_paint(current_window);

        //Dispose of the used-up list and clear the clipping we used to draw the window
        free(clip_windows);
        Context_clear_clip_rects(desktop-&gt;context);
    }

    //Simple rectangle for the mouse
    Context_fill_rect(desktop-&gt;context, desktop-&gt;mouse_x, desktop-&gt;mouse_y, 10, 10, 0xFF000000);
}
</code></pre>

<p>And now we're painting our windows with clipping! You might think we need to change something about the way <code>Window_paint()</code> does its work, but since we already modified the graphics methods it uses for drawing everything that it paints when we call it in our window drawing loop will already automatically be affected by the clipping that was applied to the context prior.</p>

<p>We could call it there, but I know it's kind of irksome that we've been at this for so long, have written hundreds of lines of code, and yet still have nothing especially resembling a window. So, since we're mucking around with putting window painting back into our screen updates, why don't we spend a quick minute and spruce it up a bit?</p>

<p>Firstly, let's centralize some of our window colors<sup>[<a href=" " title="In a really rather gross and cheap way. We can and may look into more flexible theming waaay on down the road.">11</a>]</sup>. And since we're not painting random-colored rects anymore, we can go ahead and remove the <code>fill_color</code> property from the <code>Window</code> class while we're at it.</p>

<pre><code class="language-C">//We're going to centrally define our window colors here
//Feel free to play with this 'theme'
#define WIN_BGCOLOR     0xFFBBBBBB //A generic grey
#define WIN_TITLECOLOR  0xFFBE9270 //A nice subtle blue
#define WIN_BORDERCOLOR 0xFF000000 //Straight-up black

//We're going to draw all of the windows the same, now,
//so we can remove the window color property
typedef struct Window_struct {  
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    Context* context;
} Window;
</code></pre>

<p>To match the fact that we're not using the <code>fill_color</code> property anymore, we also have to remove its assignment from the <code>Window</code> constructor while we're at it:</p>

<pre><code class="language-C">//Only change to the window constructor is that we don't need 
//to generate the background color anymore
//Window constructor
Window* Window_new(int16_t x, int16_t y,  
                   uint16_t width, uint16_t height, Context* context) {

    //Try to allocate space for a new WindowObj and fail through if malloc fails
    Window* window;
    if(!(window = (Window*)malloc(sizeof(Window))))
        return window;

    //Assign the property values
    window-&gt;x = x;
    window-&gt;y = y;
    window-&gt;width = width;
    window-&gt;height = height;
    window-&gt;context = context;

    return window;
}
</code></pre>

<p>Now that we've got that minor housekeeping out of the way, we can move on to overhauling our window painting method. Really, you can do whatever you want here and certainly feel free to play with the style. But I'm going to just do a simple 3px border in <code>WIN_BORDERCOLOR</code> around the edge of the window and along the bottom of a 25px tall <code>WIN_TITLECOLOR</code> colored titlebar rectangle, and finally finish it off by filling the body of the window in with <code>WIN_BGCOLOR</code>. The code should be pretty self-explanatory<sup>[<a href=" " title="Feel free to also implement a Context_draw_rect_thick() function that takes a border thickness in pixels to do the border drawing below a little more concisely">12</a>]</sup>:   </p>

<pre><code class="language-Language-C">//Let's start making things look like an actual window
void Window_paint(Window* window) {

    //Draw a 3px border around the window 
    Context_draw_rect(window-&gt;context, window-&gt;x, window-&gt;y,
                      window-&gt;width, window-&gt;height, WIN_BORDERCOLOR);
    Context_draw_rect(window-&gt;context, window-&gt;x + 1, window-&gt;y + 1,
                      window-&gt;width - 2, window-&gt;height - 2, WIN_BORDERCOLOR);
    Context_draw_rect(window-&gt;context, window-&gt;x + 2, window-&gt;y + 2,
                      window-&gt;width - 4, window-&gt;height - 4, WIN_BORDERCOLOR);

    //Draw a 3px border line under the titlebar
    Context_horizontal_line(window-&gt;context, window-&gt;x + 3, window-&gt;y + 28,
                            window-&gt;width - 6, WIN_BORDERCOLOR);
    Context_horizontal_line(window-&gt;context, window-&gt;x + 3, window-&gt;y + 29,
                            window-&gt;width - 6, WIN_BORDERCOLOR);
    Context_horizontal_line(window-&gt;context, window-&gt;x + 3, window-&gt;y + 30,
                            window-&gt;width - 6, WIN_BORDERCOLOR);

    //Fill in the titlebar background
    Context_fill_rect(window-&gt;context, window-&gt;x + 3, window-&gt;y + 3,
                      window-&gt;width - 6, 25, WIN_TITLECOLOR);

    //Fill in the window background
    Context_fill_rect(window-&gt;context, window-&gt;x + 3, window-&gt;y + 31,
                      window-&gt;width - 6, window-&gt;height - 34, WIN_BGCOLOR);
}
</code></pre>

<p>Finally, as an added touch, I'm going to change a teensy tiny minor section of our mouse handling code so that, more like a traditional window manager, our windows are only draggable by their titlebars:</p>

<pre><code class="language-C">void Desktop_process_mouse(Desktop* desktop, uint16_t mouse_x,  
                           uint16_t mouse_y, uint8_t mouse_buttons) {

    //[Variable declarations, mouse coordinate capture...]

    if(mouse_buttons) {
        if(!desktop-&gt;last_button_state) { 
            for(i = desktop-&gt;children-&gt;count - 1; i &gt;= 0; i--) {

                child = (Window*)List_get_at(desktop-&gt;children, i);

                //Here's our change:
                //See if the mouse position lies within the bounds of the current
                //window's 31 px tall titlebar by replacing 
                //    mouse_y &lt; (child-&gt;y + child-&gt;height)
                //with
                //    mouse_y &lt; (child-&gt;y + 31)   
                if(mouse_x &gt;= child-&gt;x &amp;&amp; mouse_x &lt; (child-&gt;x + child-&gt;width) &amp;&amp;
                mouse_y &gt;= child-&gt;y &amp;&amp; mouse_y &lt; (child-&gt;y + 31)) {

                    //[Etcetera...]
</code></pre>

<p>Such a minor little tweak that I decided to just elide as much of that function as I could because typing it all out wasn't even worth it. But that little change should make this bad boy feel a little more like you're used to.</p>

<p>And that's it for today. Compile that bad boy up and you'll get... well. Some windows. Shocker.</p>

<p><img src="/web/20180715114707im_/http://trackze.ro/content/images/2016/09/gif3.gif" alt=""/></p>

<p>&nbsp;</p>

<h2 id="epilogue">Epilogue</h2>

<p>These last two installments have most certainly been, if anything, a good case study of the idea that there is often much more to software than meets the eyes. While a simple painter's algorithm might seem like an easy way to go, and visually gives the exact same results at the end of the day<sup>[<a href=" " title="and, let's be frank, with a modern system full of basically unlimited scratch RAM and with processor speeds that rival the supercomputers of the 90s -- when these kinds of clipping techniques were in their heyday -- there's realistically no reason you can't get away with it, especially in a hobby project.">13</a>]</sup>, the reality of making a practical stacking window manager that's actually performant<sup>[<a href=" " title="and obviously the code provided could still be very very much optimized at that">14</a>]</sup> involves a lot more that isn't especially obvious. </p>

<p>And even then, we have some improvements to make down the road. Consider this: Even now that we're limiting our drawing to those bits of the windows and/or desktop actually visible on the screen, why are we wasting time redrawing every single window on every single mouse event when, for instance, we might've just moved one window two pixels to the left that didn't affect the visibility of any other window or even just moved the mouse and nothing else?</p>

<p>But at this point, our framework is beginning to get robust enough that we could go off in a lot of different directions from here in terms of what feature we want to tackle next. We could definitely dive into those further improvements to screen update efficiency -- and we will -- but I'm going to finally take a little mercy in recognition of you folks' patience since we can save that for another time. So tune in next week when we start talking about the much more interesting -- not to mention visually rewarding -- topic of implementing controls!</p>

<p><strong>Note:</strong> <em>You may notice that you get some wonky-ass issues related to memory sanity <sup>[<a href=" " title="emscripten throwing a `SEGFAULT`, possibly, if you're using it there, your OS either throwing a GPF, page fault, or just doing real fucky things">15</a>]</sup> if you try to drag a window so it extends past the top or sides<sup>[<a href=" " title="this is more noticeable because you'll experience the window wrapping around of the page if it goes past the right">16</a>]</sup> of the screen. It's an issue with our new fill rect function, try and solve it!</em>    </p>

<hr/>

<p><a href="https://web.archive.org/web/20180715114707/https://github.com/JMarlin/wsbe">As always, the source, ready for easy immediate use in your browser thanks to the black magic that is emscripten, can be found here at my github.</a></p>