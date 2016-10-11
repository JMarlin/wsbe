#include "calculator.h"
#include "textbox.h"
#include <inttypes.h>

void Calculator_button_handler(Button* button, int x, int y) {

    Calculator* calculator = (Calculator*)button->window.parent;

    if(button == calculator->button_0) {

        if(!(calculator->text_box->window.title[0] == '0' &&
            calculator->text_box->window.title[1] == 0))
            Window_append_title((Window*)calculator->text_box, "0");
    }

    if(button == calculator->button_1) {

        if(!(calculator->text_box->window.title[0] == '0' &&
            calculator->text_box->window.title[1] == 0))
            Window_append_title((Window*)calculator->text_box, "1");
        else
            Window_set_title((Window*)calculator->text_box, "1");
    }

    if(button == calculator->button_2) {

        if(!(calculator->text_box->window.title[0] == '0' &&
            calculator->text_box->window.title[1] == 0))
            Window_append_title((Window*)calculator->text_box, "2");
        else
            Window_set_title((Window*)calculator->text_box, "2");
    }

    if(button == calculator->button_3) {

        if(!(calculator->text_box->window.title[0] == '0' &&
            calculator->text_box->window.title[1] == 0))
            Window_append_title((Window*)calculator->text_box, "3");
        else
            Window_set_title((Window*)calculator->text_box, "3");
    }

    if(button == calculator->button_4) {

        if(!(calculator->text_box->window.title[0] == '0' &&
            calculator->text_box->window.title[1] == 0))
            Window_append_title((Window*)calculator->text_box, "4");
        else
            Window_set_title((Window*)calculator->text_box, "4");
    }

    if(button == calculator->button_5) {

        if(!(calculator->text_box->window.title[0] == '0' &&
            calculator->text_box->window.title[1] == 0))
            Window_append_title((Window*)calculator->text_box, "5");
        else
            Window_set_title((Window*)calculator->text_box, "5");
    }

    if(button == calculator->button_6) {

        if(!(calculator->text_box->window.title[0] == '0' &&
            calculator->text_box->window.title[1] == 0))
            Window_append_title((Window*)calculator->text_box, "6");
        else
            Window_set_title((Window*)calculator->text_box, "6");
    }

    if(button == calculator->button_7) {

        if(!(calculator->text_box->window.title[0] == '0' &&
            calculator->text_box->window.title[1] == 0))
            Window_append_title((Window*)calculator->text_box, "7");
        else
            Window_set_title((Window*)calculator->text_box, "7");
    }

    if(button == calculator->button_8) {

        if(!(calculator->text_box->window.title[0] == '0' &&
            calculator->text_box->window.title[1] == 0))
            Window_append_title((Window*)calculator->text_box, "8");
        else
            Window_set_title((Window*)calculator->text_box, "8");
    }

    if(button == calculator->button_9) {

        if(!(calculator->text_box->window.title[0] == '0' &&
            calculator->text_box->window.title[1] == 0))
            Window_append_title((Window*)calculator->text_box, "9");
        else
            Window_set_title((Window*)calculator->text_box, "9");
    }

    if(button == calculator->button_c) {
        Window_set_title((Window*)calculator->text_box, "0");
    }
}

Calculator* Calculator_new(void) {

    Calculator* calculator;
 
    //Attempt to allocate and initialize the window
    if(!(calculator = (Calculator*)malloc(sizeof(Calculator))))
        return calculator;

    if(!Window_init((Window*)calculator, 0, 0,
                    (2 * WIN_BORDERWIDTH) + 145,
                    WIN_TITLEHEIGHT + WIN_BORDERWIDTH + 170,
                    0, (Context*)0)) {

        free(calculator);
        return (Calculator*)0;
    }

    //Set a default title 
    Window_set_title((Window*)calculator, "Calculator");

    //Create the buttons
    calculator->button_7 = Button_new(WIN_BORDERWIDTH + 5, WIN_TITLEHEIGHT + 30, 30, 30);
    Window_set_title((Window*)calculator->button_7, "7");
    Window_insert_child((Window*)calculator, (Window*)calculator->button_7);

    calculator->button_8 = Button_new(WIN_BORDERWIDTH + 40, WIN_TITLEHEIGHT + 30, 30, 30);
    Window_set_title((Window*)calculator->button_8, "8");
    Window_insert_child((Window*)calculator, (Window*)calculator->button_8);

    calculator->button_9 = Button_new(WIN_BORDERWIDTH + 75, WIN_TITLEHEIGHT + 30, 30, 30);
    Window_set_title((Window*)calculator->button_9, "9");
    Window_insert_child((Window*)calculator, (Window*)calculator->button_9);

    calculator->button_add = Button_new(WIN_BORDERWIDTH + 110, WIN_TITLEHEIGHT + 30, 30, 30);
    Window_set_title((Window*)calculator->button_add, "+");
    Window_insert_child((Window*)calculator, (Window*)calculator->button_add);

    calculator->button_4 = Button_new(WIN_BORDERWIDTH + 5, WIN_TITLEHEIGHT + 65, 30, 30);
    Window_set_title((Window*)calculator->button_4, "4");
    Window_insert_child((Window*)calculator, (Window*)calculator->button_4);

    calculator->button_5 = Button_new(WIN_BORDERWIDTH + 40, WIN_TITLEHEIGHT + 65, 30, 30);
    Window_set_title((Window*)calculator->button_5, "5");
    Window_insert_child((Window*)calculator, (Window*)calculator->button_5);

    calculator->button_6 = Button_new(WIN_BORDERWIDTH + 75, WIN_TITLEHEIGHT + 65, 30, 30);
    Window_set_title((Window*)calculator->button_6, "6");
    Window_insert_child((Window*)calculator, (Window*)calculator->button_6);

    calculator->button_sub = Button_new(WIN_BORDERWIDTH + 110, WIN_TITLEHEIGHT + 65, 30, 30);
    Window_set_title((Window*)calculator->button_sub, "-");
    Window_insert_child((Window*)calculator, (Window*)calculator->button_sub);

    calculator->button_1 = Button_new(WIN_BORDERWIDTH + 5, WIN_TITLEHEIGHT + 100, 30, 30);
    Window_set_title((Window*)calculator->button_1, "1");
    Window_insert_child((Window*)calculator, (Window*)calculator->button_1);

    calculator->button_2 = Button_new(WIN_BORDERWIDTH + 40, WIN_TITLEHEIGHT + 100, 30, 30);
    Window_set_title((Window*)calculator->button_2, "2");
    Window_insert_child((Window*)calculator, (Window*)calculator->button_2);

    calculator->button_3 = Button_new(WIN_BORDERWIDTH + 75, WIN_TITLEHEIGHT + 100, 30, 30);
    Window_set_title((Window*)calculator->button_3, "3");
    Window_insert_child((Window*)calculator, (Window*)calculator->button_3);

    calculator->button_mul = Button_new(WIN_BORDERWIDTH + 110, WIN_TITLEHEIGHT + 100, 30, 30);
    Window_set_title((Window*)calculator->button_mul, "*");
    Window_insert_child((Window*)calculator, (Window*)calculator->button_mul);

    calculator->button_c = Button_new(WIN_BORDERWIDTH + 5, WIN_TITLEHEIGHT + 135, 30, 30);
    Window_set_title((Window*)calculator->button_c, "C");
    Window_insert_child((Window*)calculator, (Window*)calculator->button_c);

    calculator->button_0 = Button_new(WIN_BORDERWIDTH + 40, WIN_TITLEHEIGHT + 135, 30, 30);
    Window_set_title((Window*)calculator->button_0, "0");
    Window_insert_child((Window*)calculator, (Window*)calculator->button_0);

    calculator->button_ent = Button_new(WIN_BORDERWIDTH + 75, WIN_TITLEHEIGHT + 135, 30, 30);
    Window_set_title((Window*)calculator->button_ent, "=");
    Window_insert_child((Window*)calculator, (Window*)calculator->button_ent);

    calculator->button_div = Button_new(WIN_BORDERWIDTH + 110, WIN_TITLEHEIGHT + 135, 30, 30);
    Window_set_title((Window*)calculator->button_div, "/");
    Window_insert_child((Window*)calculator, (Window*)calculator->button_div);

    //We'll use the same handler to handle all of the buttons
    calculator->button_1->onmousedown = calculator->button_2->onmousedown = 
        calculator->button_3->onmousedown = calculator->button_4->onmousedown =
        calculator->button_5->onmousedown = calculator->button_6->onmousedown =
        calculator->button_7->onmousedown = calculator->button_8->onmousedown =
        calculator->button_9->onmousedown = calculator->button_0->onmousedown =
        calculator->button_add->onmousedown = calculator->button_sub->onmousedown = 
        calculator->button_mul->onmousedown = calculator->button_div->onmousedown =
        calculator->button_ent->onmousedown = calculator->button_c->onmousedown =
        Calculator_button_handler;          

    //Create the textbox
    calculator->text_box = TextBox_new(WIN_BORDERWIDTH + 5, WIN_TITLEHEIGHT + 5, 135, 20);
    Window_set_title((Window*)calculator->text_box, "0");
    Window_insert_child((Window*)calculator, (Window*)calculator->text_box);

    //Return the finished calculator
    return calculator;
}