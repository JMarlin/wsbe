---
layout: post
title:  "1 - Complicated Rectangles"
author: "Joe Marlin"
---

<p><em>[<strong>Note For Returning Readers</strong>: After a bit of feedback, I decided to drop the JavaScript from this post to focus completely on the C implementation. I was worried that that would alienate people who want to play with the code without having to write a whole OS around it, so I've also updated the repo for this project using Emscripten so that you can run a simple build script and have the example code running on a canvas in the web browser of your choice right away. We now return you to your regularly scheduled updated article]</em></p>

<hr/>

<p>&nbsp;</p>

<p>Okay, we’ve had the rant. It’s over with. Breathe a sigh of relief<sup>[<a href=" " title="Or let me know if you liked that last one, I could do that alllll day. Ask my girlfriend">1</a>]</sup>. Today, we're going to start writing some code.</p>

<p>To accomplish our goal for today, we're going to be deciding on a core element of the windowing system to begin with, and from thence our design will flow. We won't make it too fancy, but we <em>will</em> make it work. Note that I'm targeting this at OS-dev hobbyists -- I'm assuming we have nothing except access to a flat 32-bit framebuffer and an implementation of malloc and free. If you aren't quite there yet, you should check out <a href="https://web.archive.org/web/20180715114612/http://wiki.osdev.org/Drawing_In_Protected_Mode">this article</a> for help on the framebuffer and/or <a href="https://web.archive.org/web/20180715114612/http://wiki.osdev.org/Memory_Allocation">this article</a> for malloc<sup>[<a href=" " title="Good luck on getting through that and making it back here. Seriously. You'll probably need it.">2</a>]</sup>.</p>

<p>&nbsp;</p>

<h2 id="letswritesomecode">Let's Write Some Code</h2>

<p>It's in the name: A windowing system deals with windows. So it seems like that would be a good place to start, since the core operation of this whole piece of software is going to be the creation and the modification of said windows.</p>

<p>This is a textbook example of a good application for OO. We need to spawn a bunch of the same basic thing (windows) and each of them will have a list of intrinsic properties describing them (location, size, appearance) and a set of actions which can be applied to them (move, resize, raise). But if we're working in C, aren't we kind of boned in that? </p>

<p>You might say: <em>just solve that problem by using C++ or D or something instead!</em><sup>[<a href=" " title="If you said 'maybe like Scala or something', I would remind you that the ultimate target here would be to get this happening on a hobby OS, so anything that high-level is automatically out. If you said Haskell then [A]: You're really fucking weird and I like it and [B]: It's actually been done, look up the House project.">3</a>]</sup> And you could totally do that. The bulk of code in, for example, <a href="https://web.archive.org/web/20180715114612/https://cgit.haiku-os.org/haiku/tree/">the Haiku source</a> is written in C++<sup>[<a href=" " title="Taking after, in that regard, its pappy BeOS.">4</a>]</sup>. The one drawback is that while C can be compiled to run directly on bare hardware just like it were assembly, C++ requires a runtime to handle things like the <code>new</code> and <code>destroy</code> keyword, for which you need to go to the effort of implementing your <code>malloc</code> and <code>free</code> methods<sup>[<a href=" " title="in C anyhow, look at that">5</a>]</sup> and then binding them into the runtime. </p>

<p>Personally, I don't feel like screwing with that. And, as I'll show you, there's totally a way to make an OO-approach work in C anyway<sup>[<a href=" " title="To be fair, there's a lot of approaches, BUT THIS IS MY WAY, BUSTER.">6</a>]</sup>. Let's check it out by seeing how we would use C to make a really basic window object with a constructor the C way:</p>

<pre><code class="language-C">//#include &lt;memory management &amp; inttype headers as appropriate&gt;

//Instead of declaring our properties within a class, we just
//declare them in a struct
//DON'T WORRY, we *will* expand on this later
typedef struct Window_struct {  
    uint16_t x; //64K ought to be enough for anybody.
    uint16_t y;
    uint16_t width;
    uint16_t height;
} Window;

//For our constructor, we just write a function that does the
//memory allocation and then the other normal constructor things. 
//So instead of
//    Window* window = new Window(x, y, w, h);
//We would do
//    Window* window = Window_new(x, y, w, h);
//Not so bad. Heck, it's the same damn number of characters.
Window* Window_new(unsigned int x, unsigned int y,  
                   unsigned int width, unsigned int height) {

    //Try to allocate space for a new Window and fail through if malloc fails
    Window* window;
    if(!(window = (Window*)malloc(sizeof(Window))))
        return window;

    //Assign the property values
    window-&gt;x = x;
    window-&gt;y = y;
    window-&gt;width = width;
    window-&gt;height = height;

    return window;
}
</code></pre>

<p>Okay, that makes sense, right?. Now, even though they're basically just rectangle objects (<strong>we will get there man</strong>), we can poop out <code>Window</code>s all day! But what good is a <del>rectangle</del> window object if it's just an intangible concept in memory? </p>

<p>&nbsp;</p>

<h2 id="thisisntinterestinguntilwedrawsomething">This Isn't Interesting Until We Draw Something</h2>

<p>Before I call this a day, I know you're not going to be happy until you see something on the screen. You're here, after all, because you want windows on your screen. Well, we're not going to quite get there today, but I'll give you <em>something</em>.</p>

<p>Since we're writing this in our crazy low-level C, all we have is our framebuffer without any concept of a graphics library to simplify things<sup>[<a href=" " title="We will kind of end up writing one, though. You really could *start* by putting one into your code, like cairo if you're a badass like Kevin Lange, but we're making the arbitrary choice, in this series, to BELIEVE IN NOTHING LEBOWSKI.">7</a>]</sup>. Without any intermediating force, we are going to have to do some legwork in abstracting out our own tools for drawing into the framebuffer.</p>

<pre><code class="language-c">//We'll continue our design pattern of declaring struct 'objects' and 
//methods to operate on them by declaring a 'context' object that, for
//now, simply keeps track of the pointer to our framebuffer and its 
//dimensions: 
typedef struct Context_struct {  
    uint32_t* buffer; //A pointer to our framebuffer
    unsigned int width; //The dimensions of the framebuffer
    unsigned int height; 
} Context;

//Now, we can use that info to create a simple method for drawing a
//filled rectangle into the context
void Context_fillRect(Context* context, unsigned int x, unsigned int y,  
                      unsigned int width, unsigned int height, uint32_t color) {

    unsigned int cur_x;
    unsigned int max_x = x + width;
    unsigned int max_y = y + height;

    //Make sure we don't try to draw outside of the framebuffer:
    if(max_x &gt; context-&gt;width)
        max_x = context-&gt;width;    

    if(max_y &gt; context-&gt;height)
        max_y = context-&gt;height;

    //Draw the rectangle into the framebuffer line-by line
    //(bonus points if you write an assembly routine to do it faster)
    for( ; y &lt; max_y; y++)
        for(cur_x = x; cur_x &lt; max_x; cur_x++)
            context-&gt;buffer[y * context-&gt;width + cur_x] = color;
}
</code></pre>

<p>So we have a way to keep track of our graphics area and pass it around and we have a way to paint rectangular regions into it. Let's tie it all together by adding a reference to the graphics area to our window objects and a 'method' by which the windows can draw themselves:</p>

<pre><code class="language-C">//Updated struct and constructor:
typedef struct Window_struct {  
    //Existing properties go here...
    Context* context; //The drawing context
} Window;

Window* Window_new(unsigned int x, unsigned int y,  
                   unsigned int width, unsigned int height, Context* context) {

    //Existing allocation and assignment stuff goes here...

    window-&gt;context = context;

    return window;
}

//Here's a quick, crappy pseudo-RNG since you probably don't have one
//(you don't need to know about this):
uint8_t pseudo_rand_8() {

    static uint16_t seed = 0;
    return (uint8_t)(seed = (12657 * seed + 12345) % 256);
}

//And the actual paint 'method' for windows:
void Window_paint(Window* window) {

    //Note that you may have to change this around based on the
    //pixel format of your framebuffer (we can and will be
    //smarter about this, but ours is ABGR)
    uint32_t fill_color = 0xFF000000 |            //Opacity
                          pseudo_rand_8() &lt;&lt; 16 | //B
                          pseudo_rand_8() &lt;&lt; 8  | //G
                          pseudo_rand_8();        //R

    //Use the window's new context member to draw a rect
    //representing the window area
    Context_fillRect(window-&gt;context, window-&gt;x, window-&gt;y
                     window-&gt;width, window-&gt;height, fill_color);
}
</code></pre>

<p>Okay, take a deep breath -- <em>we did it</em>. As mentioned, definitely have to fiddle around if your framebuffer's pixel format isn't the same as ours and whatnot<sup>[<a href=" " title="If you're curious, the reason our pixel format is a little weird is because that's the way it ends up being for canvas image data in JS, and the provided source is written to build and run against the Emscripten framework provided in the git repo">8</a>]</sup>, but that gives a nice structured way to ask a window to draw itself once we've created it. Now let's do something with it already!</p>

<p>&nbsp;</p>

<h2 id="letthereberects">Let There Be Rects</h2>

<p>We made windows. We made a screen context. We gave windows the ability to paint themselves to their screen context. So let's put it all together at long last and actually do something with them by writing some main code to get a context, create a couple of windows, and draw them:</p>

<pre><code class="language-C">int main(int argc, char* argv[]) {

    //Fill this in with the info particular to your project
    //For our purposes, we're using the 'fake_os' interface 
    //that I provide in the git repo's emscripten harness
    Context context = { 0, 0, 0 };
    context.buffer = fake_os_getActiveVesaBuffer(&amp;context.width, &amp;context.height);

    //Create a few windows
    Window* win1 = Window_new(10, 10, 300, 200, &amp;context);
    Window* win2 = Window_new(100, 150, 400, 400, &amp;context);
    Window* win3 = Window_new(200, 100, 200, 600, &amp;context);

    //And draw them
    Window_paint(win1);
    Window_paint(win2);
    Window_paint(win3);    

    return 0;
}
</code></pre>

<p>And there you are. You have created a modernist masterpiece.</p>

<p><img src="/web/20180715114612im_/http://trackze.ro/content/images/2016/08/rects-1.png" alt=""/></p>

<p>&nbsp;</p>

<h2 id="isthatseriouslyit">Is That Seriously It</h2>

<p>Okay, so I'll be the first to admit that a screen with a few rectangles on it isn't super exciting<sup>[<a href=" " title="Though if you think that's bad, you should've seen my first pass at this where I spent so much time hung up fussing over early design considerations that it only got as far as defining the window objects">9</a>]</sup>. But the important thing is that we've made a foothold from which we can make progress in our future installments. Diving in like this and at least getting <em>something</em> happening can be an invaluable way to start a project; It gets the motivation juices flowing.</p>

<p>That said, even with the tiny amount of code we've written, we've already baked in a lot of poor design decisions. For one, we're definitely not going to be spawning window objects manually in our main function when we're done here. We're going to want some way of centrally keeping track of our windows. And our window painting is insanely braindead.</p>

<p>We have a lot of stuff still to consider coming up. How are we going to focus a window? What will we do when we want to <em>really</em> draw into a window? Will we keep track of controls, and how will we do it? Do we really need to be drawing parts of one window that are just going to get covered up by another window?</p>

<p>So stay tuned and we'll start answering those questions as we fix and refine our way towards something that actually looks like a windowing system. Next week, we'll be getting some proper organization to our windows.</p>

<p><a href="https://web.archive.org/web/20180715114612/https://github.com/JMarlin/wsbe">Find full source to this article here, which I've set up to be easily compiled and played with on just about any platform.</a></p>