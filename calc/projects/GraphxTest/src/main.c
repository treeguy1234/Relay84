#include <tice.h>
#include <usbdrvce.h>
#include <srldrvce.h>
#include <string.h>
#include <stdio.h>
#include <graphx.h>
#include <keypadc.h>

static char lastText[256] = {0};

uint8_t drawMenus(int);
uint8_t drawMenuOption(int, char*);

int main(void) {
    gfx_Begin();
    gfx_palette[2] = gfx_RGBTo1555(81, 85, 81);
    gfx_palette[1] = gfx_RGBTo1555(255, 255, 255);
    gfx_SetColor(2);
    gfx_FillRectangle(0, 0, 320, 15);
    gfx_SetTextFGColor(1);
    gfx_PrintStringXY("Messages READY", 5, 3);

    drawMenuOption(1, "OH YEAHH");
    drawMenuOption(2, "Lets GO");
    drawMenuOption(3, "Is This L");
    drawMenuOption(4, "!?!?");
    drawMenuOption(5, ":)");

    while(!os_GetCSC());

    gfx_End();

    return 1;
}

uint8_t drawMenus(int screen) {
    static int last_screen = -1;
    if (last_screen == screen) return 1;
    last_screen = screen;

    gfx_SetColor(2);
    gfx_FillRectangle(0, 0, 320, 20);
    gfx_SetColor(1);
    gfx_FillRectangle(0, 215, 320, 25);

    if (lastText[0]) {
        gfx_SetTextFGColor(1);
        gfx_PrintStringXY(lastText, 5, 5);
    }

    gfx_SetColor(0);
    gfx_SetTextFGColor(0);
    switch (screen) {
        case 0:
            gfx_Rectangle(0, 215, 60, 25);
            gfx_PrintStringXY("Type", 10, 222);
            gfx_Rectangle(266, 215, 54, 25);
            gfx_PrintStringXY("Home", 275, 222);
            break;
        case 1:
            gfx_Rectangle(266, 215, 54, 25);
            gfx_PrintStringXY("Send", 275, 222);
            gfx_Rectangle(206, 215, 54, 25);
            gfx_PrintStringXY("Cancel", 209, 222);
            break;
        case 2:
            gfx_Rectangle(266, 215, 54, 25);
            gfx_PrintStringXY("Home", 275, 222);
            break;
        default:
            break;
    }

    return 1;
}

uint8_t drawMenuOption(int column, char* text) {
    int x = 64 * column;
    int y = 215;
    int width = 64;
    int height = 25;
    int length = gfx_GetStringWidth(text);

    if(length > 64) {
        return 0;
    }

    gfx_SetColor(0);
    gfx_SetTextFGColor(0);

    gfx_Rectangle(x - 64, y, width, height);
    gfx_PrintStringXY(text, x - (64 - ((64 - length) / 2)), y + 9);

    return 1;
}