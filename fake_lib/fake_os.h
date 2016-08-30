#ifndef FAKE_OS_H

#include <inttypes.h>

//Used to dimension the canvas and the framebuffer array 
#define FO_SCREEN_WIDTH  1024
#define FO_SCREEN_HEIGHT 768

//Mouse handler callback function pointer type
typedef void (*mouse_handler)(uint16_t, uint16_t, uint8_t);

//Exposed functions
uint32_t* fake_os_getActiveVesaBuffer(uint16_t* width, uint16_t* height);
void fake_os_installMouseCallback(mouse_handler new_handler);

#endif //FAKE_OS_H
