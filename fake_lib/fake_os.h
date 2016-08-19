#ifndef FAKE_OS_H

#include <inttypes.h>

//Used to dimension the canvas and the framebuffer array 
#define FO_SCREEN_WIDTH  1024
#define FO_SCREEN_HEIGHT 768

//Exposed functions
uint32_t* fake_os_getActiveVesaBuffer(uint16_t* width, uint16_t* height);

#endif //FAKE_OS_H
