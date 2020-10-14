---
layout: post
title:  "4 - Get Clippy"
author: "Joe Marlin"
---

<p>So where are we at, now? We have a desktop object that holds window objects and a drawing context object with which it can draw those window objects. We've strapped the mouse into the desktop object in such a way that we can display a simplistic mouse cursor and raise and move the simple rectangular representations of our windows. Where do we go from here?</p>

<p>Well, not redrawing the entire goddamn screen and every single bit of every single window -- occluded by other windows or not -- is the direction I've chosen. We certainly could've just dived directly into building window decoration and widgets right away and then worried about having reasonable drawing times later. But, to me, solving the problem of only redrawing changed portions of the screen really sits at the core of the mysteries of windowing -- especially if you're curious about how things used to be done back when you had to figure out how to do a GUI with only 16 megs of system RAM (ie: no room for window buffers) and a 133MHz CPU (ie: no time to push millions and millions of pixels).</p>

<p>So today is going to be part 1 of a two-part series on window clipping. We won't get as far as drawing clipped geometry until next week, but we <em>will</em> develop a super simple way to implement a clipping region in our <code>Context</code> that will allow us to do so and get a super cool visualization of how it works.</p>

<p>&nbsp;</p>

<h2 id="iseeyouretryingtocomposesomewindows">I See You're Trying to Compose Some Windows</h2>

<p>Buckle in, because today's going to be a bit of a long one, but the result is going to be somewhat neat<sup>[<a href=" " title="Definitely more visually interesting than the last couple of articles">1</a>]</sup>. Our first task is answering the question of what it is, exactly, we're trying to accomplish. When we say we want to only redraw those things on the screen that have changed, what do we mean precisely?</p>

<p>As it stands, when we move a window the following process occurs:</p>

<p><img src="/web/20180715114647im_/http://trackze.ro/content/images/2016/09/seq1.gif" alt=""/></p>

<p>Obviously, there's a lot of waste here. For one, the lowest window isn't even in the finished image, so why are we drawing it at all? How much time could we save in drawing if we figured out what things <em>don't</em> need to be drawn and skipped them entirely? This is a problem that can be solved with clipping.</p>

<p>Here's the general idea of clipping in an image: <br/>
<img src="/web/20180715114647im_/http://trackze.ro/content/images/2016/09/clip.png" alt=""/></p>

<p>You have a subject polygon and a 'cutting' polygon, and you want to produce a third polygon which is, rather intuitively, the result of 'cutting' the first polygon with the second in some way. Seems straightforward, but how does that help us exactly?</p>

<p>The first thing we can do is use clipping to 'subtract' a higher window's bounding rectangle from a lower window's bounding rectangle:</p>

<p><img src="/web/20180715114647im_/http://trackze.ro/content/images/2016/09/rectclip.png" alt=""/></p>

<p>Doing that gives us a polygon that represents the area of the lower window that will actually be visible on screen. The second thing we can then do is use polygon clipping between this visibility polygon and the geometry we want to draw on our window. By doing that, suddenly we've achieved our goal of restricting our drawing to what's not occluded on screen:</p>

<p><img src="/web/20180715114647im_/http://trackze.ro/content/images/2016/09/winclip.png" alt=""/></p>

<p>If we were to use the lowest window in our first example for something like this, its visibility polygon would end up being completely empty and so any drawing operations to that window would end up being skipped entirely!</p>

<p>So here's our goal: Give our drawing context a way to set up a clipping region to which any drawing operations will be restricted.</p>

<p>&nbsp;</p>

<h2 id="thebestkindofcorrect">The Best Kind of Correct</h2>

<p>So that makes some sense, I think. But here's our big challenge: We're building this thing from scratch so that we know exactly how everything works, and polygon clipping is suuuuuper complicated. That said, the problem has been solved in several ways academically and it should be possible to take one of those existing algorithms and attempt to implement it in our own code.</p>

<p>But here's the thing: Polygon clipping is complex because it has a bajillion edge cases since the problem involves solving for shapes that can come in a basically infinite set of configurations. But we have a nice out here: Everything we're dealing with is just a rectangle. A rectangle clipped by another rectangle will always result in a right polygon of some sort. And the fun part is that you can, in turn, break any right polygon down into a collection of rectangles:</p>

<p><img src="/web/20180715114647im_/http://trackze.ro/content/images/2016/09/polyrect.png" alt=""/></p>

<p>So it's rectangles all the way down.</p>

<p>So, here's our approach: Instead of a more generic 'visibilty polygon', we're going to keep track of a collection of 'visibility rectangles'. Then we'll limit any drawing operations to the interior of those rectangles.</p>

<p>So all of our problems eventually boil down to being able to take two overlapping rectangles and split one by the other.</p>

<p>&nbsp;</p>

<h2 id="arectanglesplitterthatsalmostasstupidasclippyhimself">A Rectangle Splitter That's Almost as Stupid as Clippy Himself</h2>

<p>I'll jump straight to the chase: Here's my solution to removing one rectangle from another and producing a set of rectangles that make up the difference. The idea is to iteratively slice the subject rectangle by each edge of the cutting rectangle: </p>

<p><img src="/web/20180715114647im_/http://trackze.ro/content/images/2016/09/split.png" alt=""/></p>

<p>I think this makes a lot of sense visually, but let me break it down:</p>

<ul>
<li>For each edge of the cutting rect (red):
<ol><li>If that edge is between the parallel edges of the subject:</li>
<li>Create a new output rectangle spanning from the outside edge of the subject to the cutting edge of the cutter</li>
<li>Reduce the extents of the subject rectangle by the cutter edge</li></ol></li>
</ul>

<p>After all of this dry writing, you're probably ready to start doing some actual code. And the above idea might make more sense if you were to see it in code, so let's get started on that by making a <code>Rect</code> class that we can apply the above algorithm to:</p>

<pre><code class="language-C">//It'll have the obvious properties
typedef struct Rect_struct {  
    int top;
    int left;
    int bottom;
    int right;
} Rect;

//And an obvious constructor
Rect* Rect_new(int top, int left, int bottom, int right) {

    //Attempt to allocate the object
    Rect* rect;
    if(!(rect = (Rect*)malloc(sizeof(Rect))))
        return rect;

    //Assign intial values
    rect-&gt;top = top;
    rect-&gt;left = left;
    rect-&gt;bottom = bottom;
    rect-&gt;right = right;

    return rect;
}
</code></pre>

<p>The above pretty much went without saying. But you'd best bust out your reading glasses, because we're about to implement our rectangle splitter as a member function of this new class:</p>

<pre><code class="language-C">//Explode subject_rect into a list of contiguous rects which are
//not occluded by cutting_rect         
List* Rect_split(Rect* subject_rect, Rect* cutting_rect) {

    //Allocate the list of result rectangles
    List* output_rects;
    if(!(output_rects = List_new()))
        return output_rects;

    //We're going to modify the subject rect as we go,
    //so we'll clone it so as to not upset the object 
    //we were passed
    Rect subject_copy;
    subject_copy.top = subject_rect-&gt;top;
    subject_copy.left = subject_rect-&gt;left;
    subject_copy.bottom = subject_rect-&gt;bottom;
    subject_copy.right = subject_rect-&gt;right;

    //We need a rectangle to hold new rectangles before
    //they get pushed into the output list
    Rect* temp_rect;

    //Begin splitting
    //1 -Split by left edge if that edge is between the subject's left and right edges 
    if(cutting_rect-&gt;left &gt;= subject_copy.left &amp;&amp; cutting_rect-&gt;left &lt;= subject_copy.right) {

        //Try to make a new rectangle spanning from the subject rectangle's left and stopping before 
        //the cutting rectangle's left
        if(!(temp_rect = Rect_new(subject_copy.top, subject_copy.left,
                                  subject_copy.bottom, cutting_rect-&gt;left - 1))) {

            //If the object creation failed, we need to delete the list and exit failed
            free(output_rects);

            return (List*)0;
        }

        //Add the new rectangle to the output list
        List_add(output_rects, temp_rect);

        //Shrink the subject rectangle to exclude the split portion
        subject_copy.left = cutting_rect-&gt;left;
    }

    //2 -Split by top edge if that edge is between the subject's top and bottom edges 
    if(cutting_rect-&gt;top &gt;= subject_copy.top &amp;&amp; cutting_rect-&gt;top &lt;= subject_copy.bottom) {

        //Try to make a new rectangle spanning from the subject rectangle's top and stopping before 
        //the cutting rectangle's top
        if(!(temp_rect = Rect_new(subject_copy.top, subject_copy.left,
                                  cutting_rect-&gt;top - 1, subject_copy.right))) {

            //If the object creation failed, we need to delete the list and exit failed
            //This time, also delete any previously allocated rectangles
            while(output_rects-&gt;count) {

                temp_rect = List_remove_at(output_rects, 0)
                free(temp_rect);
            }

            free(output_rects);

            return (List*)0;
        }

        //Add the new rectangle to the output list
        List_add(output_rects, temp_rect);

        //Shrink the subject rectangle to exclude the split portion
        subject_copy.top = cutting_rect-&gt;top;
    }

    //3 -Split by right edge if that edge is between the subject's left and right edges 
    if(cutting_rect-&gt;right &gt;= subject_copy.left &amp;&amp; cutting_rect-&gt;right &lt;= subject_copy.right) {

        //Try to make a new rectangle spanning from the subject rectangle's right and stopping before 
        //the cutting rectangle's right
        if(!(temp_rect = Rect_new(subject_copy.top, cutting_rect-&gt;right + 1,
                                  subject_copy.bottom, subject_copy.right))) {

            //Free on fail
            while(output_rects-&gt;count) {

                temp_rect = List_remove_at(output_rects, 0)
                free(temp_rect);
            }

            free(output_rects);

            return (List*)0;
        }

        //Add the new rectangle to the output list
        List_add(output_rects, temp_rect);

        //Shrink the subject rectangle to exclude the split portion
        subject_copy.right = cutting_rect-&gt;right;
    }

    //4 -Split by bottom edge if that edge is between the subject's top and bottom edges 
    if(cutting_rect-&gt;bottom &gt;= subject_copy.top &amp;&amp; cutting_rect-&gt;bottom &lt;= subject_copy.bottom) {

        //Try to make a new rectangle spanning from the subject rectangle's bottom and stopping before 
        //the cutting rectangle's bottom
        if(!(temp_rect = Rect_new(cutting_rect-&gt;bottom + 1, subject_copy.left,
                                  subject_copy.bottom, subject_copy.right))) {

            //Free on fail
            while(output_rects-&gt;count) {

                temp_rect = List_remove_at(output_rects, 0)
                free(temp_rect);
            }

            free(output_rects);

            return (List*)0;
        }

        //Add the new rectangle to the output list
        List_add(output_rects, temp_rect);

        //Shrink the subject rectangle to exclude the split portion
        subject_copy.bottom = cutting_rect-&gt;bottom;
    }

    //Finally, after all that, we can return the output rectangles 
    return output_rects;
}
</code></pre>

<p>It's actually not that bad for how long it is. Most of the bulk of that thing up there is just safeguarding against allocation failures when we create the list object that will hold the output rectangles and when we create each of those new output rectangles.</p>

<p>All that's really happening up there is the same thing four times with the difference in each being the edge that's doing the cutting. We just check to see if a cutting edge is between the edges of the subject and, if so, create a new output rectangle to the outside of it and shrink the subject rectangle to the inside of it.</p>

<p>It was a bit of a journey, but we have rectangle-by-rectangle clipping now which we'll be able to use for all of our future clipping needs.</p>

<p>&nbsp;</p>

<h2 id="puttingthingsintocontext">Putting Things Into Context</h2>

<p>What we ultimately want to do with these rectangles is hold a collection of them representing the screen area our drawing operations are going to be clipped to and, strangely enough, clip our drawing operations to them. Since this clipping is going to be eventually applied to context drawing operations, we're going to need to add a list of <code>Rect</code> clipping regions to our <code>Context</code> properties:</p>

<pre><code class="language-C">typedef struct Context_struct {  
    //[Same 'ol props...]
    List* clip_rects; //What it says on the box
} Context;
</code></pre>

<p>Now that we have something we definitely need to initialize, we're going to have to finally get around to giving this context thing an actual constructor:</p>

<pre><code class="language-C">//Constructor for our context, at long last
Context* Context_new(uint16_t width, uint16_t height, uint32_t* buffer) {

    //Attempt to allocate
    Context* context;
    if(!(context = (Context*)malloc(sizeof(Context))))
        return context; 

    //Attempt to allocate new rect list 
    if(!(context-&gt;clip_rects = List_new())) {

        free(context);
        return (Context*)0;
    }

    //Finish assignments
    context-&gt;width = width; 
    context-&gt;height = height; 
    context-&gt;buffer = buffer;

    return context;
}
</code></pre>

<p>Nothing big, there, just allocating a <code>Context</code> object, allocating a <code>List</code> for the clipping rectangles and doing the initial property assignments.</p>

<p>Now what are we going to <em>do</em> with this list of rectangles? Well, for starters, we need a way to add more rectangles to it. At first blush, this might seem as simple as calling <code>List_add()</code> on <code>clip_rects</code>. But what we're trying to express with this list of rectangles is the collection of areas on the screen that we're going to be allowed to draw to, and as such we want those areas to be non-overlapping. But we just made a function that splits one rectangle by another, so we can just use that to snip out an area where the new rectangle will fit:</p>

<pre><code class="language-C">//Insert the passed rectangle into the clip list, splitting all
//existing clip rectangles against it to prevent overlap
void Context_add_clip_rect(Context* context, Rect* added_rect) {

    int i, j;
    Rect* cur_rect;
    List* split_rects;

    //Check each item already in the list to see if it overlaps with
    //the new rectangle
    for(i = 0; i &lt; context-&gt;clip_rects-&gt;count; ) {

        cur_rect = List_get_at(context-&gt;clip_rects, i);

        //Standard rect intersect test
        //see here for an example of why this works:
        //http://stackoverflow.com/questions/306316/determine-if-two-rectangles-overlap-each-other#tab-top
        if(!(cur_rect-&gt;left &lt;= added_rect-&gt;right &amp;&amp;
           cur_rect-&gt;right &gt;= added_rect-&gt;left &amp;&amp;
           cur_rect-&gt;top &lt;= added_rect-&gt;bottom &amp;&amp;
           cur_rect-&gt;bottom &gt;= added_rect-&gt;top)) {

            //If this rect doesn't intersect with the added_rect
            //then we can just move on to the next one
            i++;
            continue;
        }

        //If this rectangle *does* intersect with the new rectangle, 
        //we need to split it
        List_remove_at(context-&gt;clip_rects, i); //Original will be replaced w/splits
        split_rects = Rect_split(cur_rect, added_rect); //Do the split
        free(cur_rect); //We can throw this away now, we're done with it

        //Copy the split, non-overlapping result rectangles into the list 
        while(split_rects-&gt;count) {

            cur_rect = (Rect*)List_remove_at(split_rects, 0); //Pull from A
            List_add(context-&gt;clip_rects, cur_rect); //Push to B
        }

        //Free the split_rect list that we just emptied
        free(split_rects);

        //Since we removed an item from the list, we need to start counting over again 
        //In this way, we'll only exit this loop once nothing in the list overlaps 
        i = 0;    
    }

    //Now that we have made sure none of the existing rectangles overlap
    //with the new rectangle, we can finally insert it into the hole
    //we just created
    List_add(context-&gt;clip_rects, added_rect);
}
</code></pre>

<p>So, if we want to add a new rectangle we first punch a hole for it out of the existing rectangles. This is good for adding new rects into the clipping region, but what about when we're done setting up our current clipping? Here's a much simpler tool for clearing our clipping information when we're ready to set up a new clipping:</p>

<pre><code class="language-C">//Remove all of the clipping rects from the passed context object
void Context_clear_clip_rects(Context* context) {

    Rect* cur_rect;

    //Remove and free until the list is empty
    while(context-&gt;clip_rects-&gt;count) {

        cur_rect = (Rect*)List_remove_at(context-&gt;clip_rects, 0);
        free(cur_rect);
    }
}
</code></pre>

<p>&nbsp;</p>

<h2 id="makingsomethingofnothing">Making Something of Nothing</h2>

<p>This whole thing may be seeming a little black-magicy, and we've already added a good chunk of comparatively arcane code compared to our previous articles, so what I want to do today is take what we've made so far and make it visible so you can get an idea of how our rectangle splitting is working.</p>

<p>To do this, I temporarily want to replace window drawing by instead inserting each of the window bounds into the context's clipping rectangles and then, once they're all added, drawing each of the result rectangles to the screen so we can see the way our splitting is working firsthand.</p>

<p>So let's update the desktop paint method:</p>

<pre><code class="language-C">//Paint the desktop 
void Desktop_paint(Desktop* desktop) {

    //Loop through all of the children and call paint on each of them 
    unsigned int i;
    Window* current_window;
    Rect* temp_rect;

    //Start by clearing the desktop background
    Context_fill_rect(desktop-&gt;context, 0, 0, desktop-&gt;context-&gt;width,
                      desktop-&gt;context-&gt;height, 0xFF000000); //Change pixel format if needed 
                                                            //Currently: ABGR

    //Instead of painting the windows, for now we'll add their dimensions to the context
    //clip rects and then draw those rects to show how our splitting algorithm works
    //Clear the old rects 
    Context_clear_clip_rects(desktop-&gt;context);

    //Add a clipping rect for each window
    for(i = 0; (current_window = (Window*)List_get_at(desktop-&gt;children, i)); i++) {

        temp_rect = Rect_new(current_window-&gt;y, current_window-&gt;x,
                             current_window-&gt;y + current_window-&gt;height - 1,
                             current_window-&gt;x + current_window-&gt;width - 1);
        Context_add_clip_rect(desktop-&gt;context, temp_rect);
    }

    //Draw the resultant clipping regions
    for(i = 0; i &lt; desktop-&gt;context-&gt;clip_rects-&gt;count; i++) {

        temp_rect = (Rect*)List_get_at(desktop-&gt;context-&gt;clip_rects, i);
        Context_draw_rect(desktop-&gt;context, temp_rect-&gt;left, temp_rect-&gt;top,
                          temp_rect-&gt;right - temp_rect-&gt;left + 1,
                          temp_rect-&gt;bottom - temp_rect-&gt;top + 1,
                          0xFF00FF00);
    }

    //Draw the mouse
    Context_fill_rect(desktop-&gt;context, desktop-&gt;mouse_x, desktop-&gt;mouse_y, 10, 10, 0xFFFF0000);
}
</code></pre>

<p>Add all of the windows, draw all of the rects. Only problem here is that I used one method we don't have yet: <code>Context_draw_rect()</code>. We want to see the clipping regions as something clearer than a blob of one color, and it's about time we started adding some more drawing methods to our context anyhow:</p>

<pre><code class="language-C">//Drawing a rectangle as two horizontal and two vertical lines
void Context_draw_rect(Context* context, int x, int y,  
                       unsigned int width, unsigned int height, uint32_t color) {

    Context_horizontal_line(context, x, y, width, color); //top
    Context_vertical_line(context, x, y + 1, height - 2, color); //left 
    Context_horizontal_line(context, x, y + height - 1, width, color); //bottom
    Context_vertical_line(context, x + width - 1, y + 1, height - 2, color); //right
}
</code></pre>

<p>And since we don't have those functions yet either, let's write some super simple code to draw horizontal and vertical lines:</p>

<pre><code class="language-C">//A horizontal line as a filled rect of height 1
void Context_horizontal_line(Context* context, int x, int y,  
                             unsigned int length, uint32_t color) {

    Context_fill_rect(context, x, y, length, 1, color);
}

//A vertical line as a filled rect of width 1
void Context_vertical_line(Context* context, int x, int y,  
                           unsigned int length, uint32_t color) {

    Context_fill_rect(context, x, y, 1, length, color);
}
</code></pre>

<p>&nbsp;</p>

<h2 id="mangledmain">Mangled Main</h2>

<p>To wrap up the entry code for today's work, the only thing we really have to change is the initialization of our Context, since we have to use the new constructor we just made now:</p>

<pre><code class="language-C">int main(int argc, char* argv[]) {

    //Fill this in with the info particular to your project
    Context* context = Context_new(0, 0, 0); //Using the constructor now
    context-&gt;buffer = fake_os_getActiveVesaBuffer(&amp;context-&gt;width, &amp;context-&gt;height);

    //And now all of the usual stuff
    desktop = Desktop_new(context);
    Desktop_create_window(desktop, 10, 10, 300, 200);
    Desktop_create_window(desktop, 100, 150, 400, 400);
    Desktop_create_window(desktop, 200, 100, 200, 600);
    Desktop_paint(desktop);
    fake_os_installMouseCallback(main_mouse_callback); //Or polling loop, of course

    return 0; 
}
</code></pre>

<p>So what did all of the weirdness above get us? Ladies and gentlemen may I present: </p>

<p><img src="/web/20180715114647im_/http://trackze.ro/content/images/2016/09/seq2.gif" alt=""/></p>

<p>What are we looking at? Abusing the clipping region tools we just wrote like this, we can see how our relatively simple rectangle splitting adds up to give us a group of non-overlapping rects that effectively act as an x-ray into how our windows are going to clip each other. Since we're adding the topmost window to the clipping region last it splits everything else without getting split itself, and so we can see how we might be able to use this splitting to break up lower windows into their visible clipping regions when we need to redraw them.</p>

<p>&nbsp;</p>

<h2 id="itsover">It's Over</h2>

<p>So today, in a bid to start talking about using clipping to speed up our drawing and start discussing the more complex functions of a nontrivial windowing system, we introduced and implemented a pretty simple rectangle splitting algorithm that we'll be able to use to generate the clipping regions that our future context drawing will be limited to. </p>

<p>Our method for adding a new rectangle to our context's clipping regions isn't actually going to be used as much as it is here, because next week we'll look into how it can be quickly modified to allow for subtracting a rectangular area from the clipping regions instead. Once we have this, creating the 'window visibility rects' for a given window will be as easy as clearing the context clipping, adding the rect of the window to draw, and then subtracting the rects for each window above it. Then all we have to do is update our context drawing functions to limit their effects to the current clipping regions and our windowing system will finally be drawing itself without re-rendering the entirety of every window.</p>

<p>We're doing all of this craziness because it cannot be overstated how important clipping is to a non-compositing windowing system in order for it to actually be performant. It may really not end up being super critical to your particular project depending on how advanced it is, but if you're doing an OS project for the same reason that I do -- because you want to really know how the internals of things work -- then you would be remiss to ignore the way it's handled in the real world. Of course, commercial systems like GDI in Windows implement a <em>far</em> more complex clipping polygon infrastructure than what we've built here, and you're free to try implementing a 'true' polygon clipper yourself -- just be warned that that would end up being about as deep an endeavor as the rest of this blog series. I'd like to think that we've presented a good example of creatively using the domain of a problem to your advantage.</p>

<hr/>

<p><a href="https://web.archive.org/web/20180715114647/https://github.com/JMarlin/wsbe">As always, the source, ready for easy immediate use in your browser thanks to the black magic that is emscripten, can be found here at my github.</a></p>