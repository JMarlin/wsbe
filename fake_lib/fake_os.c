#include "fake_os.h"
#include <emscripten.h>
#include <inttypes.h>
#include <stdlib.h>

mouse_handler installed_mouse_callback = (mouse_handler)0;

//Returns the pointer to the buffer in the return value and the width and the height
//in the supplied pointers
uint32_t* fake_os_getActiveVesaBuffer(uint16_t* width, uint16_t* height) {

    //This function will generate a fixed-size canvas and a fixed-size pixel array.
    //It then clears the buffer and installs a timer function to constantly copy the 
    //content of the pixel array to the canvas context at 60fps
    
    //Declare our return variable
    uint32_t *return_buffer = (uint32_t*)0;

    //Clear the dimensions until we've gotten past any potential errors
    *width = 0;
    *height = 0;

    //Attempt to create the framebuffer array 
    if(!(return_buffer = (uint32_t*)malloc(sizeof(uint32_t) * FO_SCREEN_WIDTH * FO_SCREEN_HEIGHT)))
        return return_buffer; //Exit early indicating error with an empty pointer 

    //Now that we've gotten past the potential error, we'll set the return 
    //screen dimension values
    *width = FO_SCREEN_WIDTH;
    *height = FO_SCREEN_HEIGHT;

    //Clear the framebuffer to black
    int i;
    for(i = 0; i < (*width) * (*height); i++)
        return_buffer[i] = 0xFF000000; //The canvas *does* care about the opacity being set, which is annoying
    
    //Now we'll create the output canvas and insert it into the document
    //(EM_ASM allows us to embed JS into our C)
    //We will also se up the refresh timer here
    EM_ASM_({
        
        //Create and store canvas and information
        window.fo_canvas = document.createElement('canvas');
        document.body.style.margin = '0px';
        window.fo_canvas.style.cursor = 'none';
        window.fo_canvas.width = $0;
        window.fo_canvas.height = $1;
        window.fo_buf_address = $2;
        window.fo_buf_size = 4 * $0 * $1;
        document.body.appendChild(window.fo_canvas);
        window.fo_context = window.fo_canvas.getContext('2d');
        window.fo_canvas_data = window.fo_context.getImageData(0, 0, $0, $1);

        //Start refresh handler
        setInterval(function() {

            //Create an unsigned byte subarray  
            window.fo_canvas_data.data.set(
                Module.HEAPU8.subarray(
                    window.fo_buf_address, window.fo_buf_address + window.fo_buf_size
                )
            ); 
            window.fo_context.putImageData(window.fo_canvas_data, 0, 0);
        }, 17);
    }, FO_SCREEN_WIDTH, FO_SCREEN_HEIGHT, return_buffer);

    return return_buffer;
}

void EMSCRIPTEN_KEEPALIVE fake_os_doMouseCallback(void) {

    uint16_t mouse_x, mouse_y;
    uint8_t buttons;

    if(!installed_mouse_callback)
        return;

    //A mouse event has happened, so get the updated info 
    mouse_x = EM_ASM_INT({
        return window.fo_mouse_x;
    }, 0);

    mouse_y = EM_ASM_INT({
        return window.fo_mouse_y;
    }, 0);

    buttons = EM_ASM_INT({
        return window.fo_button_status;
    }, 0);

    //Fire callback
    installed_mouse_callback(mouse_x, mouse_y, buttons);
}

void fake_os_installMouseCallback(mouse_handler new_handler) {

    //This is literally just here so that the function 
    //doesn't get optimized out
    fake_os_doMouseCallback();

    EM_ASM(

        //Attach status update function to the screen canvas
        window.fo_button_status = 0;
        window.fo_mouse_x = 0;
        window.fo_mouse_y = 0;
        window.fo_canvas.onmousemove = function(e) {
            
            window.fo_mouse_x = e.clientX;
            window.fo_mouse_y = e.clientY;
            Module.ccall('fake_os_doMouseCallback');
        };
        window.fo_canvas.onmousedown = function(e) {

            window.fo_button_status = 1;
            Module.ccall('fake_os_doMouseCallback');
        };
        window.fo_canvas.onmouseup = function(e) {

            window.fo_button_status = 0;
            Module.ccall('fake_os_doMouseCallback');
        };
    );

    installed_mouse_callback = new_handler;
}