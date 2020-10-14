---
layout: post
title:  "2 - Rectangle HQ"
author: "Joe Marlin"
---

<p>Last time, we decided to get started by implementing a basic <code>Window</code> class using our flavor of object-oriented C, in the process also creating a <code>Context</code> class to wrap our framebuffer details and abstract drawing<sup>[<a href=" " title="'drawing'">1</a>]</sup> to our simple video device which we then used to draw some 'window' rectangles to our screen.</p>

<p>When we were done, I touched on some obvious ways in which what we wrote fails entirely at being a window manager. Today, we're going to start tackling one of those failures, namely the fact that our 'window manager' doesn't... you know. Manage windows. Kind of an issue.</p>

<p>&nbsp;</p>

<h2 id="weneedtotalkaboutthechildren">We Need To Talk About the Children</h2>

<p>First, I want to prepare you yet again to be a little let down: This week is going to be yet another instance of a bunch of code without much visual payoff. But I promise that once we have our fancy<sup>[<a href=" " title="and well-architectured, dammit">2</a>]</sup> infrastructure all worked out I promise we'll make up for it by going hog-wild with the ridiculous chrome<sup>[<a href=" " title="If you want the regular kind of chrome, though, you're on your own with porting it.">3</a>]</sup>.</p>

<p>Okay. So what's the plan? We need a way to manage the creation and organization of our windows, or else the core concept of a windowing system isn't really happening. So what we're going to want to do is create some other kind of class that will act as the parent of our windows and handle spawning and deleting them and<sup>[<a href=" " title="VERY eventually">4</a>]</sup> distributing mouse and keyboard events among them.</p>

<p>We also happen to be lacking a desktop, so why not just call the desktop the parent of our windows?</p>

<pre><code class="language-C">//Let's define an object for our desktop/parent class
typedef struct Desktop_struct {  
    List* children;   //For storing a bunch of windows
    Context* context; //A context for drawing the windows and desktop
} Desktop;
</code></pre>

<p>Whoa whoa whoa. Wait up. We know what that second item is, a reference to a drawing context, that makes sense. But what the hell is that first type? <em>Are you getting soft on us and importing libs, kid?</em></p>

<p>I'm not getting soft, we're goddamn <em>doing</em> this thing. We're going to get ourselves sidetracked for a moment and implement a <code>List</code> class for storing our list of windows. Sure, we could've just slapped an array of windows in there, but what if we end up needing to allocate more windows than we had the foresight to in the first place? Nope, this will not do. Let's get to listin'.</p>

<pre><code class="language-C">//A type to encapsulate a basic dynamic list
typedef struct List_struct {  
    unsigned int count;  //The number of items in our list
    ListNode* root_node; //The first element in the list
} List;

//Basic list constructor
List* List_new() {

    //Malloc and/or fail null
    List* list;
    if(!(list = (List*)malloc(sizeof(List))))
        return list;

    //Fill in initial property values
    //(All we know for now is that we start out with no items) 
    list-&gt;count = 0;
    list-&gt;root_node = (ListNode*)0;

    return list;
}
</code></pre>

<p>Now just you wait a <em>goddamned</em> second, I hear you cry. Am I putting you on? WHAT ARE THESE <code>ListNode</code> THINGS.</p>

<p>The plan here is to keep it pretty simple with this list class and just make it an implementation of a doubly-linked list. So basically, this class is just going to wrap a bunch of linked list elements, for which we'll make -- you guessed it -- another class<sup>[<a href=" " title="will the fun NEVER end?">5</a>]</sup>.</p>

<pre><code class="language-C">//A type to encapsulate an individual item in a linked list
typedef struct ListNode_struct {  
    void* payload;                //Generic pointer to an object
    struct ListNode_struct* prev; //Self-referential back-pointer
    struct ListNode_struct* next; //Self-referential forward-pointer
} ListNode;

//Here's the basic ListNode constructor
ListNode* ListNode_new(void* payload) {

    //Malloc and/or fail null (I think you get the pattern at this point)
    ListNode* list_node;
    if(!(list_node = (ListNode*)malloc(sizeof(ListNode))))
        return list_node;

    //Assign initial properties
    list_node-&gt;prev = (ListNode*)0;
    list_node-&gt;next = (ListNode*)0;
    list_node-&gt;payload = payload; 

    return list_node;
}
</code></pre>

<p><strong>Okay</strong>. There we are. We've finally hit the end of our trail and implemented the constructor for the most basic unit of this desktop/manager thing we're trying to build. We can now generate an object which holds a pointer to some other kind of object and which can potentially, but does not yet, point to others of its ilk. </p>

<p>I'm assuming here<sup>[<a href=" " title="and thereby making an ass of you and ming">6</a>]</sup> that you already have a working knowledge of the concepts of this, the most stupidly basic data structure of them all, <a href="https://web.archive.org/web/20180715114631/https://en.wikipedia.org/wiki/Doubly_linked_list">but if not you can take this quick refresher and get back to us</a>.</p>

<p>So we can make our linked list and our list links, but we can't yet make our list links into a linked list. Before we move on back to the desktop class, we know that we're going to need to at least be able use our list to append windows as they're created and also to access them once they're there, so let's get those taken care of starting with the add method, which is pretty straightforward:</p>

<pre><code class="language-C">//Insert a payload at the end of the list
//Zero is fail, one is success
int List_add(List* list, void* payload) {

    //Try to make a new node, exit early on fail 
    ListNode* new_node;
    if(!(new_node = ListNode_new(payload))) 
        return 0;

    //If there aren't any items in the list yet, assign the
    //new item to the root node
    if(!list-&gt;root_node) {

        list-&gt;root_node = new_node;        
    } else {

        //Otherwise, we'll find the last node and add our new node after it
        ListNode* current_node = list-&gt;root_node;

        //Fast forward to the end of the list 
        while(current_node-&gt;next)
            current_node = current_node-&gt;next;

        //Make the last node and first node point to each other
        current_node-&gt;next = new_node;
        new_node-&gt;prev = current_node; 
    }

    //Update the number of items in the list and return success
    list-&gt;count++;

    return 1;
}
</code></pre>

<p>Easy. Make a node with the provided payload, then either stick it in the root position or use the existing root node to iterate to the end of the chain and stick the new node there, then increase our count. </p>

<p>Now we'll do the method to retrieve the payload of the linked-list-link for a given index. This way, among other things, we can throw this into a for-loop with an index variable to iterate its contents (in this case, our windows). <sup>[<a href=" " title="There would certainly be more efficient ways to do so, since by doing it that way we end up spending time rewinding the list and then getting back to the index-th item for each subsequent access whereas we could definitely implement an iterator that just gets the subsequent link on each access. But here I'm trading off maximal efficiency for simplicity of implementation. If you want to one-up me, feel free to implement a binary tree and a bevy of sexy access methods, it's your right as a citizen of the world.">7</a>]</sup>  </p>

<pre><code class="language-C">//Get the payload of the list item at the given index
//Indices are zero-based
void* List_get_at(List* list, unsigned int index) {

    //If there's nothing in the list or we're requesting beyond the end of
    //the list, return nothing
    if(list-&gt;count == 0 || index &gt;= list-&gt;count) 
        return (void*)0;

    //Iterate through the items in the list until we hit our index
    ListNode* current_node = list-&gt;root_node;

    //Iteration, making sure we don't hang on malformed lists
    for(unsigned int current_index = 0; (current_index &lt; index) &amp;&amp; current_node; current_index++)
        current_node = current_node-&gt;next;

    //Return the payload, guarding against malformed lists
    return current_node ? current_node-&gt;payload : (void*)0;
}
</code></pre>

<p>Okay, we can manage lists of things, now. Core concept: check. Let's put it in practice.</p>

<p>&nbsp;</p>

<h2 id="gatherroundchildren">Gather 'Round, Children</h2>

<p>So let's get back to that desktop class we were wanting to implement this week. As you remember, we're beginning its life as a pretty simple structure, just that list of windows we just implemented the data structure for and a context reference for drawing the child windows to. For now, all we're doing is spawning windows and drawing them, so that's all we need. Now that we have the missing link<sup>[<a href=" " title="ed list">8</a>]</sup> implemented, let's implement that desktop constructor.</p>

<pre><code class="language-C">Desktop* Desktop_new(Context* context) {

    //Malloc or fail 
    Desktop* desktop;
    if(!(desktop = (Desktop*)malloc(sizeof(Desktop))))
        return desktop;

    //Use the new List constructor to create the child list
    //(or clean up and fail otherwise, of course)
    if(!(desktop-&gt;children = List_new())) {

        //Delete the new desktop object and return null 
        free(desktop);
        return (Desktop*)0;
    }

    //Fill out other properties 
    desktop-&gt;context = context;

    return desktop;
}
</code></pre>

<p>Nothing we haven't seen before. But we said we want to spawn child windows and draw them, so let's make that happen. For the window spawning method, all we're really doing is plumbing together the <code>Window</code> constructor and the <code>List_add</code> function we just made above:</p>

<pre><code class="language-C">//A method to automatically create a new window in the provided desktop 
Window* Desktop_create_window(Desktop* desktop, unsigned int x, unsigned int y,  
                              unsigned int width, unsigned int height) {

    //Attempt to create the window instance
    Window* window;
    if(!(window = Window_new(x, y, width, height, desktop-&gt;context)))
        return window;

    //Attempt to add the window to the end of the desktop's children list
    //If we fail, make sure to clean up all of our allocations to this point
    if(!List_add(desktop-&gt;children, (void*)window)) {

        free(window);
        return (Window*)0;
    }

    return window;
}
</code></pre>

<p>Then we just need to do the drawing. This is also basically just plumbing, in this case we're sticking together our new list access method and our window painting method. So that we see at least <em>some</em> visual change for all of our work today, we'll also make the desktop 'paint' itself by painting a background color beneath everything before iterating through the windows.</p>

<pre><code class="language-C">//Paint the desktop 
void Desktop_paint(Desktop* desktop) {

    //Loop through all of the children and call paint on each of them 
    unsigned int i;
    Window* current_window;

    //Start by clearing the desktop background
    Context_fillRect(desktop-&gt;context, 0, 0, desktop-&gt;context-&gt;width,
                     desktop-&gt;context-&gt;height, 0xFFFF9933); //Change pixel format if needed 
                                                            //Currently: ABGR

    //Get and draw windows until we stop getting valid windows out of the list 
    for(i = 0; (current_window = (Window*)List_get_at(desktop-&gt;children, i)); i++)
        Window_paint(current_window);

}
</code></pre>

<p>Now that's starting to look like some kind of actual manager of something!<sup>[<a href=" " title="maybe even windows!">9</a>]</sup></p>

<p>&nbsp;</p>

<h2 id="oncemorewithfeelingandabackdrop">Once More, With Feeling (And A Backdrop)</h2>

<p>That's enough implementation of new crap today. We got our initial goals completed, namely that we now, instead of manually having to manage our own pointers for each window we want to make, have a central object which handles the spawning and drawing of windows. So let's update our main function to take advantage of that:</p>

<pre><code class="language-C">//Create and draw a few rectangles and exit
int main(int argc, char* argv[]) {

    //Fill this in with the info particular to your project
    //(In this case, we're still using our 'fake os' functions)
    Context context = { 0, 0, 0 };
    context.buffer = fake_os_getActiveVesaBuffer(&amp;context.width, &amp;context.height);

    //Create the desktop based on our new graphics context
    Desktop* desktop = Desktop_new(&amp;context);

    //Now, we just replace the calls to the window constructor
    //with calls to the desktop window generator, which drops
    //the need for the win1, win2,... variables and gives us
    //centralized management basically for free
    Desktop_create_window(desktop, 10, 10, 300, 200);
    Desktop_create_window(desktop, 100, 150, 400, 400);
    Desktop_create_window(desktop, 200, 100, 200, 600);

    //And what's more, now we don't even have to deal with drawing 
    //each window individually. We can just ask the desktop to do
    //it all for us.
    Desktop_paint(desktop);

    return 0;
}
</code></pre>

<p>Sit back, wipe the sweat off of your brow, and crack your knuckles if you need it, because that's it for code today. If we give it a quick compile, we get this fancy new result:</p>

<p><img src="/web/20180715114631im_/http://trackze.ro/content/images/2016/08/output_2.png" alt=""/></p>

<p>&nbsp;</p>

<h2 id="iamseriousanddontcallmeshirley">I Am Serious. And Don't Call Me Shirley.</h2>

<p>Even less new to see than last week. Why am I even wasting my time?</p>

<p>In this case, beauty is far more than skin deep. Don't be fooled by the fact that ultimately we really only added a blue background to last week's work. While it wouldn't be at all obvious from looking at it, by giving this thing a central source of window control we've actually set ourselves up to be able to handle all of the important tasks that our windowing system needs to perform.</p>

<p>The thing is, <em>our windows need to know about each other</em>, or, in this case, we need something that knows about all of our windows. Without some way of knowing how all of our windows are related to each other, we could never do any calculations of what the proper depth sorting of each of them is. We could never request that a window be raised or lowered, because we'd have no idea what we need to put that window ahead of or behind. And what's more, we'd have nothing to make that request <em>to</em>.</p>

<p>Next week, we're going to expand on both the desktop class and the list class that it uses in order to start implementing actual window operations: hiding, showing, raising, lowering and all of the other good stuff that makes windowing useful. </p>

<p><a href="https://web.archive.org/web/20180715114631/https://github.com/jmarlin/wsbe">As always, full code for all of these articles can be found over in the git repo</a>. Feel free to play with it and shoehorn into your own OS project, or, if you don't have time for that kind of foolishness, use the included build scripts to compile to JavaScript and run it right in your browser.</p>