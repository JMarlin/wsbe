---
layout: post
title:  "3 - I Like To Move It"
author: "Joe Marlin"
---

<p>Hello, and welcome back to WSBE, if you've been following along for the last couple of articles! If you're just joining us, this is a series of articles in which we're exploring the design and implementation of a simple windowing system step-by-step in C (for use, ex: in a hobby OS<sup>[<a href=" " title="or just if you're just weird and like reading about the minutia of the software implementation process">1</a>]</sup>).</p>
<p>Thus far, we've begun by creating a minimal window class along with a drawing abstraction class with which those windows could display themselves. Then, last week, we focused on structure a bit and implemented a 'desktop' class to do the job of organizing all of our child windows and serve as a central hub for window management actions, and also built a simple linked list class which we needed for storing the windows in that desktop.</p>
<p>Some may have found last week a bit boring as we didn't really get a big change in terms of output, but, as we mentioned at the end of that article, it laid a good amount of groundwork that we need in order to implement some core window management functions. So hold onto your butt, because that's what we're going to dive into today.</p>
<p> </p>
<h2 id="theresamouseinthehouse">There's a Mouse in the House</h2>
<p>No more beating around the bush: Today we're going to make our windows raise-able and move-able<sup>[<a href=" " title="And every window you'll see on this desktop is eat-able. Good bye, Gene :'(">2</a>]</sup>. To be able to trigger such actions, we're going to need to bring some input onto the scene in the form of the mouse, at long last. As such, we'll begin by expanding our desktop class with a method that will allow the desktop to react to actions by the mouse device and serve as the source from which all events will flow<sup>[<a href="https://web.archive.org/web/20190617035157/http://wiki.osdev.org/PS/2_Mouse" title="In this series, we're assuming that you've written the core items you would need in order to implement a GUI (input device handling, getting a linear framebuffer set up), but want to know how the actual GUI part might be done. If you don't have the mouse part done yet, clicking this footnote will take you to an article that should get you pointed in the right direction for handling a simple PS/2 device">3</a>]</sup>.</p>
<p>So, the code. We're going to add some properties to the desktop class to facilitate the mouse handling we're about to do by storing the mouse status<sup>[<a href=" " title="Note that we're kind of lazily assuming that our mouse reports one button either being clicked or not. We'll improve on that assumption in the future">4</a>]</sup>.</p>
<pre><code class="language-c">typedef struct Desktop_struct {
    List* children;            //old
    Context* context;          //old
    uint8_t last_button_state; //To track whether the mouse was last down or up
    uint16_t mouse_x;          //Current position of cursor on desktop
    uint16_t mouse_y;          
} Desktop;
</code></pre>
<p>Now to put that into use. For our mouse handling method, we'll start by addressing window raising. For this, we want to detect a button up-&gt;down transition. If we pass that test<sup>[<a href="https://web.archive.org/web/20190617035157/https://www.youtube.com/watch?v=wytqqfzC5E8" title="To be the best, we gotta pass the test. You gotta make it all the way to the top of the mountain. Oh, just click me already. You know you want to.">5</a>]</sup>, we want to find if our mouse is within the bounds of a child. Our desktop's child list ordering represents relative depth on screen, so the easiest way to do this is to iterate backwards<sup>[<a href=" " title="That might seem weird, but our desktop painting method draws the list of windows from first to last, so the item at the end of the list becomes the topmost window. We could draw it backwards instead and do this forwards, but it's arbitrary and I don't give a damn.">6</a>]</sup> through that list until the current child's bounds encompass the current mouse location. If the mouse is within the bounds of multiple windows it doesn't matter because we'll detect the higher window first, break the loop and never encounter the lower window. Cheap occlusion is best occlusion.</p>
<pre><code class="language-c">//Interface method between windowing system and mouse device
void Desktop_process_mouse(Desktop* desktop, uint16_t mouse_x,
                           uint16_t mouse_y, uint8_t mouse_buttons) {

    //For window iterating
    int i;
    Window* child;

    //Capture the reported mouse coordinates
    desktop-&gt;mouse_x = mouse_x;
    desktop-&gt;mouse_y = mouse_y;

    //Check to see if mouse button has been depressed since last mouse update
    if(mouse_buttons) {
        
        //If so, check for a button up -&gt; down transition
        if(!desktop-&gt;last_button_state) {

            //Here's that window bounds-checking loop we were talking about
            for(i = desktop-&gt;children-&gt;count - 1; i &gt;= 0; i--) {

                child = (Window*)List_get_at(desktop-&gt;children, i);

                //Bounds check on the current window
                if(mouse_x &gt;= child-&gt;x &amp;&amp; mouse_x &lt; (child-&gt;x + child-&gt;width) &amp;&amp;
                   mouse_y &gt;= child-&gt;y &amp;&amp; mouse_y &lt; (child-&gt;y + child-&gt;height)) {

                    //Mouse was depressed on this window, so do the raising
                    List_remove_at(desktop-&gt;children, i); //Pull window out of list
                    List_add(desktop-&gt;children, (void*)child); //Insert at the top 

                    //Since we hit a window, we can stop looking
                    break;
                }
            }
        } 
    }

    //Now that we've handled any changes the mouse may have caused, we need to
    //update the screen to reflect those changes 
    Desktop_paint(desktop);

    //Update the stored mouse button state to match the current state 
    desktop-&gt;last_button_state = mouse_buttons;
}
</code></pre>
<p>So, pretty much what we said. Oh, and we also implemented window raising, almost by accident it was that easy. I told you that the groundwork in making this desktop object would pay off for this stuff, and it did. Since we're just drawing windows from back to front right now, to raise a window we just have to pop it out of the list and stash it back at the end of the list. When we redraw, it's now the last thing drawn.</p>
<p>Oh, okay, so yeah our list class doesn't actually <em>have</em> a <code>List_remove_at()</code> function. But come on, we already made a lookup function, so we already know how to find an element in the list by index. To implement removal at an index, we just do the same thing but then repoint the nodes on either side of that list node:</p>
<pre><code class="language-c">void* List_remove_at(List* list, unsigned int index) {

    //The same damn lookup as List_get_at
    void* payload; 

    if(list-&gt;count == 0 || index &gt;= list-&gt;count) 
        return (void*)0;

    ListNode* current_node = list-&gt;root_node;

    for(unsigned int current_index = 0; (current_index &lt; index) &amp;&amp; current_node; current_index++)
        current_node = current_node-&gt;next;

    //This is where we differ from List_get_at by stashing the payload,
    //re-pointing the current node's neighbors to each other and 
    //freeing the removed node 

    //Return early if we got a null node somehow
    if(!current_node)
        return (void*)0;

    //Stash the payload so we don't lose it when we delete the node     
    payload =  current_node-&gt;payload;
 
    //Re-point neighbors to each other 
    if(current_node-&gt;prev)
        current_node-&gt;prev-&gt;next = current_node-&gt;next;

    if(current_node-&gt;next)
        current_node-&gt;next-&gt;prev = current_node-&gt;prev;

    //If the item was the root item, we need to make
    //the node following it the new root
    if(index == 0)
        list-&gt;root_node = current_node-&gt;next;

    //Now that we've clipped the node out of the list, we should free its memory
    free(current_node); 

    //Make sure the count of items is up-to-date
    list-&gt;count--; 

    //Finally, return the payload
    return payload;
}
</code></pre>
<p>It looks a little long, but it's that easy. We're literally now completely finished with the core implementation of window raising. Done<sup>[<a href=" " title="Haaaaa, you believed me for a second, there. Yes, we're still doing things in a very naive fashion that will be addressed in the future. But for now, we're good.">7</a>]</sup>. I guess... move on to moving things? It <em>is</em> kind of the title of the article.</p>
<p> </p>
<h2 id="everythinginitsrightplace">Everything in Its Right Place</h2>
<p>Basic window movement is going to be just about as trivial to implement as raising was, and is going to be a simple extension of that code. That code already provides us a mechanism for determining what window was under the mouse when the button was pressed.</p>
<p>All we need to do is capture what window that was and assume that we're dragging it until we detect that the the mouse button has been released. Finally, if we're in the middle of a drag we just have to update the dragged window's location before we redraw. First, we need some dragging status info properties, then we can write the logic:</p>
<pre><code class="language-c">typedef struct Desktop_struct {
    //[All of the old properties here]
    Window* drag_child;  //-The window being dragged or null if no drag
    uint16_t drag_off_x; //-Offset between the corner of the window and
    uint16_t drag_off_y; // where the mouse button went down
} Desktop;
</code></pre>
<pre><code class="language-c">//Now, the updates to the mouse handler
void Desktop_process_mouse(Desktop* desktop, uint16_t mouse_x,
                           uint16_t mouse_y, uint8_t mouse_buttons) {

    //[Var declaration and mouse coordinate capture]

    if(mouse_buttons) {
        if(!desktop-&gt;last_button_state) {
            for(i = desktop-&gt;children-&gt;count - 1; i &gt;= 0; i--) {

                child = (Window*)List_get_at(desktop-&gt;children, i);

                if(mouse_x &gt;= child-&gt;x &amp;&amp; mouse_x &lt; (child-&gt;x + child-&gt;width) &amp;&amp;
                   mouse_y &gt;= child-&gt;y &amp;&amp; mouse_y &lt; (child-&gt;y + child-&gt;height)) {

                    //[Raise window in list]

                    //Get the offset and the dragged window
                    desktop-&gt;drag_off_x = mouse_x - child-&gt;x;
                    desktop-&gt;drag_off_y = mouse_y - child-&gt;y;
                    desktop-&gt;drag_child = child;

                    //Since we hit a window, we can stop looking
                    break;
                }
            }
        } 
    } else { //We add an else to the button being down to cancel the drag

        desktop-&gt;drag_child = (Window*)0;
    }

    //Update drag window to match the mouse if we have an active drag window
    if(desktop-&gt;drag_child) {

        //Applying the offset makes sure that the corner of the
        //window doesn't awkwardly suddenly snap to the mouse location
        desktop-&gt;drag_child-&gt;x = mouse_x - desktop-&gt;drag_off_x;
        desktop-&gt;drag_child-&gt;y = mouse_y - desktop-&gt;drag_off_y;
    }
    
    //[Paint changes and update captured button status]
}
</code></pre>
<p>There. Moving done. Granted, this will make the windows move no matter where we press the mouse down on them whereas your standard window manager will just have a draggable titlebar or something, but we'll address that when we start adding window chrome.</p>
<p>Now we should be all good to wire this stuff up to our application's entry point, but there's a couple of exceedingly minor things we're missing<sup>[<a href=" " title="and it's not a couple of screws in the noggin, that's implied.">8</a>]</sup>.</p>
<p> </p>
<h2 id="awholelotofflashfornothing">A Whole Lot of Flash for Nothing</h2>
<p>As it stands, we've been setting the 'window' color in the <code>Window_paint()</code> method, which worked fine because we only painted once. But we need a stable place to set that now that we're redrawing on every mouse event, or the windows are going to all change color every time we move the mouse<sup>[<a href=" " title="Feel free to implement this change last and see how things work before changing it, though. It's trippy as hell. PSA: Always trip with a buddy, kids.">9</a>]</sup></p>
<pre><code class="language-c">//Update the window properties to keep stable track of the drawing color
typedef struct Window_struct {  
    //[All of the old properties...]
    uint32_t fill_color; //What it says on the box
} Window;
</code></pre>
<p>Now to assign that value in a more stable place. The <code>Window</code> constructor will do fine:</p>
<pre><code class="language-c">//Window constructor
Window* Window_new(uint16_t x, uint16_t y,  
                   uint16_t width, uint16_t height, Context* context) {

    //[Window allocation...]
    //[Init other properties...]

    //Moving the color assignment to the window constructor
    //so that we don't get a different color on every redraw
    window-&gt;fill_color = 0xFF000000 |            //Opacity
                         pseudo_rand_8() &lt;&lt; 16 | //B
                         pseudo_rand_8() &lt;&lt; 8  | //G
                         pseudo_rand_8();        //R

    return window;
}

//And make sure we update the paint method to match
void Window_paint(Window* window) {

    Context_fillRect(window-&gt;context, window-&gt;x, window-&gt;y,
                     window-&gt;width, window-&gt;height, window-&gt;fill_color);
}
</code></pre>
<p>Now our desktop won't look like the accouterments to some sweet 90s warehouse party.</p>
<p> </p>
<h2 id="butwaittheresmore">But Wait, There's More</h2>
<p>Will the minutia never end? There's one more thing it would be nice to have if we're going to be using the mouse to click on things and that would be... a mouse. An actual mouse cursor, if we're going to be specific.</p>
<p>Some day, we're going to draw a really cool and unique cursor that will put all UI design of the last 30 years to shame<sup>[<a href=" " title="some OSdev hobbyists actually think like this completely unironically, to which I say keep the dream alive, I guess.">10</a>]</sup>. But right now, we don't even have line drawing going on much less bitmaps, so we're going to keep it ultra-simple and just use those mouse coordinates we stashed to draw a small black rectangle at the end of our desktop painting loop:</p>
<pre><code class="language-c"> void Desktop_paint(Desktop* desktop) {
  
    //[Var declaration...]

    //[Clear desktop...]
    //[Draw list of children...]
    
    //Simple ugly mouse. As usual color is ABGR so adjust to your system
    Context_fillRect(desktop-&gt;context, desktop-&gt;mouse_x,
                     desktop-&gt;mouse_y, 10, 10, 0xFF000000);
}
</code></pre>
<p> </p>
<h2 id="abettermousetrap">A Better Mousetrap</h2>
<p>As always, our final task is to tie everything together by using what we've built in our entry code. This time, we don't have to do much except for query the OS for mouse activity and then feed that data to the desktop's mouse handler as it comes.</p>
<p>The standard way you would probably get the mouse activity in most environments would be to call a function that waits for the mouse driver to detect activity within a loop, but my code has to compile down to JavaScript and JS is not a huge fan of polling loops so my 'fake_os' calls used in the code below take a callback instead. Since that's kind of an unreasonable assumption for most other uses, I'm providing an example of the polling implementation as well.</p>
<pre><code class="language-c">//Our desktop object needs to be sharable by our main function
//as well as our mouse event callback. You wouldn't need to worry
//about this if you're polling instead.
Desktop* desktop;

//The callback that our mouse device will trigger on mouse updates
void main_mouse_callback(uint16_t mouse_x, uint16_t mouse_y, uint8_t buttons) {

    Desktop_process_mouse(desktop, mouse_x, mouse_y, buttons);
}

//The new and improved entry point
int main(int argc, char* argv[]) {

    //Init the context, desktop, and some windows as usual
    Context context = { 0, 0, 0 };
    context.buffer = fake_os_getActiveVesaBuffer(&amp;context.width, &amp;context.height);
    desktop = Desktop_new(&amp;context);
    Desktop_create_window(desktop, 10, 10, 300, 200);
    Desktop_create_window(desktop, 100, 150, 400, 400);
    Desktop_create_window(desktop, 200, 100, 200, 600);

    //Do an initial paint
    Desktop_paint(desktop);

    //Install the desktop mouse handler callback into the mouse driver
    fake_os_installMouseCallback(main_mouse_callback);

    //If you were doing a more standard top level event loop
    //(eg: weren't dealing with the quirks of making this thing
    //compile to JS), it would look more like this:
    //    while(1) {
    //
    //        fake_os_waitForMouseUpdate(&amp;mouse_x, &amp;mouse_y, &amp;buttons);
    //        Desktop_process_mouse(desktop, mouse_x, mouse_y, buttons);
    //    }

    //In a real OS, since we wouldn't want this thread unloaded and 
    //thereby lose our callback code, you would probably want to
    //hang here or something if using a callback model
    return 0; 
}
</code></pre>
<p>Now we're all plumbed up and ready to go. Build it. Try it out. My god, it's actually starting to look like a... sort-of window... thingy.</p>
<p><img src="/web/20190617035157im_/http://www.trackze.ro/content/images/2016/08/output_3.png" alt=""></p>
<p> </p>
<h2 id="sweetsweetpayoff">Sweet, Sweet Payoff</h2>
<p>And so your patience from last week has been rewarded! You move your mouse on the table, it moves a thingy on the screen. You click the mouse on a rectangle, the rectangle pops up for action. You hold the mouse button down on a rectangle and you can fling it all over the place. This is progress. This is tangible.</p>
<p>But I have bad news, buckaroo. This still sucks<sup>[<a href=" " title="no, really">11</a>]</sup>. And there's one reason in particular. It's slow as shit.</p>
<p><em>I don't know</em>, you say to yourself. <em>Seems pretty snappy to me.</em></p>
<p>But get this: Every time the screen gets redrawn, we're redrawing all of the desktop, then all of window 1, all of window 2, all of window 3 and the mouse rectangle. So every time we move the mouse, we do the work of writing (1024x768 + 300x200 + 400x400 + 200x600 + 10x10) = <em><strong>over 1.1 million pixels</strong></em> into the framebuffer. Into a framebuffer that, in its entirety<sup>[<a href=" " title="in my implementation, anyway">12</a>]</sup>, only contains about 2/3 that number of pixels.</p>
<p>Imagine it takes ten average CPU instruction times for us to set the value of a pixel in framebuffer RAM. If all we did was move the cursor two pixels to the left, all we <em>need</em> to update are the 100 pixels of the cursor and the strip of 20 pixels we exposed of the desktop or window that was below it. But instead of the 1,200 instructions that would've taken, we took over 11,000,000 and wasted at least 10,998,800 instructions that could've been used to handle actual useful background processes.</p>
<p>And that's only for drawing a flat desktop, three flat-colored window rectangles, and the mouse. Imagine how bad those numbers are going to get when you have 30 windows that are painting themselves full of widgets on the screen.</p>
<p>So, be happy. We're most assuredly making progress. But next week, we're going to go even further and investigate the use of clipping to make our drawing more realistically efficient. See you then!</p>
<p><em>Fun Time Programmin Puzzles: If you play around with dragging the windows, you'll notice that they 'disappear' when they cross the top or left of the screen. If you want to solve an easy challenge, see if you can find any sign of why this happens and fix it.</em></p>
<hr>
<p><a href="https://web.archive.org/web/20190617035157/https://github.com/JMarlin/wsbe">As always, the source, ready for easy immediate use in your browser thanks to the black magic that is emscripten, can be found here at my github</a>. You'll notice that we've started splitting up the code by class to keep our expanding code more manageable. All we did was put the core structs and method declarations for each class into their own headers and the implementations of those methods into separate source files, so it shouldn't be too bad to follow.</p>