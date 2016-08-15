//Object representing a window on the screen
function WindowObj(x, y, width, height, context) {

    var that = this; //Avoid js 'this'-wonkiness

    //Assign properties
    that.x = x;
    that.y = y;
    that.width = width;
    that.height = height;
    that.context = context;

    //Method for drawing a window into its context
    that.paint = function() {

        //For now, the window is just going to be a random-colored rect
        //The canvas already does a ton for us, so this is really easy in JS
        that.context.fillStyle = "rgb(" +
                                 Math.floor(Math.random()*255) +
                                 ", " +
                                 Math.floor(Math.random()*255) +
                                 ", " +
                                 Math.floor(Math.random()*255) +
                                 ")";
        that.context.fillRect(that.x, that.y, that.width, that.height);
    };
}

//Create a canvas and get its context
var canvas = document.createElement('canvas');

//Arbitrary static size to keep things easy
canvas.width = 1024;  
canvas.height = 768;  
document.body.appendChild(canvas);

var context = canvas.getContext('2d');

//Create a few windows
var win1 = new WindowObj(10, 10, 300, 200, context);  
var win2 = new WindowObj(100, 150, 400, 400, context);  
var win3 = new WindowObj(200, 100, 200, 600, context);

//And draw them
win1.paint();  
win2.paint();  
win3.paint(); 
