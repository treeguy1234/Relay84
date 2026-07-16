#include <tice.h>
#include <time.h>
#include <usbdrvce.h>
#include <srldrvce.h>
#include <string.h>
#include <stdio.h>
#include <graphx.h>
#include <keypadc.h>
#include <ctype.h>
#include <math.h>

#define CURRENT_VERSION 0.6

static srl_device_t srl;
static bool serial_connected = false;
static uint8_t srl_buffer[256];
static char lastText[256] = {0};
static char name[128] = {""};
static int currentScreen = 0;
static bool hasDrawnMenus = false;
static int messageLine = 0;

/*
    Screen 0: Main screen
    Screen 1: Typing screen
    Screen 2: Single-Button Home Screen
    Screen 3: Settings Screen
*/

uint8_t enterTypeMode(void);
uint8_t goToScreen(int);
uint8_t drawMenuOption(int, char*);
uint8_t setStatusText(const char*, int);
uint8_t clearHome(bool);
uint8_t sendMessage(const char*, bool);
uint8_t openSettings(void);
uint8_t handleAction(char*);
uint8_t displayMessage(char* message);

const char *list[48] = {
    [10] = "\"",
    [11] = "W",
    [12] = "R",
    [13] = "M",
    [14] = "H",
    [17] = "?",
    [19] = "V",
    [20] = "Q",
    [21] = "L",
    [22] = "G",
    [25] = ":",
    [26] = "Z",
    [27] = "U",
    [28] = "P",
    [29] = "K",
    [30] = "F",
    [31] = "C",
    [33] = " ",   // space
    [34] = "Y",
    [35] = "T",
    [36] = "O",
    [37] = "J",
    [38] = "E",
    [39] = "B",
    [42] = "X",
    [43] = "S",
    [44] = "N",
    [45] = "I",
    [46] = "D",
    [47] = "A"
};
const char *shiftList[48] = {
    [18] = "3",
    [19] = "6",
    [20] = "9",
    [21] = ")",
    [25] = ".",
    [26] = "2",
    [27] = "5",
    [28] = "8",
    [29] = "(",
    [33] = "0",
    [34] = "1",
    [35] = "4",
    [36] = "7",
    [37] = ","
};

static usb_error_t usb_event_handler(
    usb_event_t event,
    void *event_data,
    usb_callback_data_t *callback_data __attribute__((unused))
) {
    usb_error_t err = srl_UsbEventCallback(event, event_data, callback_data);
    if (err != USB_SUCCESS) {
        return err;
    }

    if (event == USB_HOST_CONFIGURE_EVENT ||
        (event == USB_DEVICE_ENABLED_EVENT && !(usb_GetRole() & USB_ROLE_DEVICE))) {

        if (serial_connected) return USB_SUCCESS;

        usb_device_t device;
        if (event == USB_HOST_CONFIGURE_EVENT) {
            device = usb_FindDevice(NULL, NULL, USB_SKIP_HUBS);
            if (device == NULL) return USB_SUCCESS;
        } else {
            device = event_data;
        }

        if (srl_Open(&srl, device, srl_buffer, sizeof(srl_buffer),
                     SRL_INTERFACE_ANY, 115200) == SRL_SUCCESS) {
            serial_connected = true;
            memset(srl_buffer, 0, sizeof(srl_buffer));
            setStatusText("Bridge connection success", 0);
            goToScreen(0);
            hasDrawnMenus = true;
        }
    }

    if (event == USB_DEVICE_DISCONNECTED_EVENT) {
        usb_device_t dev = event_data;
        if (dev == srl.dev) {
            srl_Close(&srl);
            serial_connected = false;
            memset(srl_buffer, 0, sizeof(srl_buffer));
            setStatusText("Bridge disconnected", 0);
            goToScreen(2);
            hasDrawnMenus = false;
        }
    }

    return USB_SUCCESS;
}

int main(void) {
    gfx_Begin();
    gfx_palette[1] = gfx_RGBTo1555(255, 255, 255); // White
    gfx_palette[2] = gfx_RGBTo1555(81, 85, 81); // Dark Grey
    gfx_palette[3] = gfx_RGBTo1555(194, 194, 194); // Light Grey
    gfx_FillScreen(1);
    goToScreen(2);

    const usb_standard_descriptors_t *descs = srl_GetCDCStandardDescriptors();
    usb_error_t init_err = usb_Init(usb_event_handler, NULL, descs, USB_DEFAULT_INIT_FLAGS);
    
    setStatusText("Searching for bridge...", 0);

    if (init_err != USB_SUCCESS) {
        usb_Cleanup();
        return 1;
    }

    bool prevTraceDown = false;
    while (!kb_IsDown(kb_KeyGraph)) {
        kb_Scan();
        usb_HandleEvents();

        if (kb_IsDown(kb_KeyYequ) && hasDrawnMenus) {
            goToScreen(1);
        }

        bool traceDown = kb_IsDown(kb_KeyTrace);
        if (traceDown && !prevTraceDown && hasDrawnMenus) {
            goToScreen(3);
        }
        prevTraceDown = traceDown;

        if (serial_connected) {
            int bytes_read = srl_Read(&srl, srl_buffer, sizeof(srl_buffer));
            if (bytes_read > 0) {
                if (bytes_read >= (int)sizeof(srl_buffer)) {
                    bytes_read = (int)sizeof(srl_buffer) - 1;
                }
                for (int i = 0; i < bytes_read; i++) {
                    if (srl_buffer[i] == '\r' || srl_buffer[i] == '\n' || srl_buffer[i] == '\0') {
                        srl_buffer[i] = 0;
                        break;
                    }
                }
                const char *action_prefix = "//action:";
                size_t prefix_len = strlen(action_prefix);
                if (strncmp((char*)srl_buffer, action_prefix, prefix_len) == 0) {
                    char actionCommand[256] = {0};
                    strncpy(actionCommand, (char*)srl_buffer + prefix_len, sizeof(actionCommand) - 1);
                    //handleAction(actionCommand);
                } else {
                    srl_buffer[bytes_read] = 0;
                    displayMessage((char *)srl_buffer);
                }
            }
        }
    }

    if (serial_connected) srl_Close(&srl);
    usb_Cleanup();
    gfx_End();
    return 0;
}

uint8_t enterTypeMode(void) {
    int currentMessageLength = 0;
    currentScreen = 1;
    bool shift = false;
    bool caps = false;
    bool wasCanceled = false;
    char messageList[256] = {0};
    setStatusText("Begin typing (Graph/Enter to send)...", 1);
    while (true) {
        usb_HandleEvents();
        if (serial_connected) {
            int bytes_read = srl_Read(&srl, srl_buffer, sizeof(srl_buffer));
            if (bytes_read > 0) {
                if (bytes_read >= (int)sizeof(srl_buffer)) {
                    bytes_read = (int)sizeof(srl_buffer) - 1;
                }
                for (int i = 0; i < bytes_read; i++) {
                    if (srl_buffer[i] == '\r' || srl_buffer[i] == '\n' || srl_buffer[i] == '\0') {
                        srl_buffer[i] = 0;
                        break;
                    }
                }
                const char *action_prefix = "//action:";
                size_t prefix_len = strlen(action_prefix);
                if (strncmp((char*)srl_buffer, action_prefix, prefix_len) == 0) {
                    char actionCommand[256] = {0};
                    strncpy(actionCommand, (char*)srl_buffer + prefix_len, sizeof(actionCommand) - 1);
                    //handleAction(actionCommand);
                } else {
                    srl_buffer[bytes_read] = 0;
                    displayMessage((char *)srl_buffer);
                }
            }
        }
        kb_Scan();
        if (kb_IsDown(kb_KeyGraph) || kb_IsDown(kb_KeyEnter)) break;
        if(kb_IsDown(kb_KeyTrace)) {
            while (kb_IsDown(kb_KeyTrace)) {
                kb_Scan();
                usb_HandleEvents();
            }
            wasCanceled = true;
            break;
        }
        if(kb_IsDown(kb_Key2nd)) {
            while (kb_IsDown(kb_Key2nd)) {
                kb_Scan();
                usb_HandleEvents();
            }
            shift = !shift;
            continue;
        }
        if(kb_IsDown(kb_KeyAlpha)) {
            while (kb_IsDown(kb_KeyAlpha)) {
                kb_Scan();
                usb_HandleEvents();
            }
            caps = !caps;
            continue;
        }
        if(kb_IsDown(kb_KeyDel)) {
            messageList[strlen(messageList) - 1] = '\0';
            setStatusText(messageList, 0);
            gfx_PrintStringXY("_", 5 + gfx_GetStringWidth(messageList), 5);
            currentMessageLength = gfx_GetStringWidth(messageList) + 5;
            while (kb_IsDown(kb_KeyDel)) {
                kb_Scan();
                usb_HandleEvents();
            }
        }
        if(kb_IsDown(kb_KeyClear)) {
            memset(messageList, 0, sizeof(messageList));
            setStatusText(messageList, 0);
            setStatusText("Begin typing (Graph/Enter to send)...", 1);
            currentMessageLength = gfx_GetStringWidth(messageList) + 5;
            while (kb_IsDown(kb_KeyClear)) {
                kb_Scan();
                usb_HandleEvents();
            }
        }

        uint8_t key = os_GetCSC();
        if (key && key < (sizeof list / sizeof list[0]) && list[key] != NULL && shift == false && currentMessageLength <= 305) {
            const char *output = list[key];
            char ch = output[0];

            if(caps && isalpha(ch)) {
                ch = (char)tolower((unsigned char)ch);
            }

            size_t cur = strlen(messageList);
            size_t avail = sizeof(messageList) - cur - 1;
            strncat(messageList, &ch, avail > 0 ? 1 : 0);
            
            currentMessageLength = gfx_GetStringWidth(messageList) + 5;
            setStatusText(messageList, 0);
            gfx_PrintStringXY("_", 5 + gfx_GetStringWidth(messageList), 5);

            caps = true;
        } else if (key && key < (sizeof shiftList / sizeof shiftList[0]) && shiftList[key] != NULL && shift == true  && currentMessageLength <= 305) {
            const char *output = shiftList[key];

            size_t cur = strlen(messageList);
            size_t avail = sizeof(messageList) - cur - 1;
            strncat(messageList, output, avail);

            currentMessageLength = gfx_GetStringWidth(messageList) + 5;
            setStatusText(messageList, 0);
            gfx_PrintStringXY("_", 5 + gfx_GetStringWidth(messageList), 5);
        }
    }

    char temp_str[2] = {'\n', '\0'};
    strncat(messageList, temp_str, 1);

    while(kb_IsDown(kb_KeyGraph) || kb_IsDown(kb_KeyEnter) || kb_IsDown(kb_KeyTrace)) {
        kb_Scan();
        usb_HandleEvents();
    }

    setStatusText(messageList, 0);

    if(!wasCanceled) {
        sendMessage(messageList, false);
        displayMessage(messageList);
    }else{
        setStatusText("Message canceled!", 0);
    }

    while (kb_IsDown(kb_Graph)) {
        kb_Scan();
        usb_HandleEvents();
    }

    goToScreen(0);
    return 1;
}

uint8_t goToScreen(int screen) {
    static int last_screen = -1;
    if (last_screen == screen) return 1;
    last_screen = screen;

    gfx_SetColor(2);
    gfx_FillRectangle(0, 0, 320, 20);
    gfx_SetColor(1);
    gfx_FillRectangle(0, 215, 320, 25);

    if (lastText[0]) {
        gfx_SetTextFGColor(1);
        gfx_SetTextBGColor(2);
        gfx_PrintStringXY(lastText, 5, 5);
    }

    gfx_SetColor(0);
    gfx_SetTextFGColor(0);
    switch (screen) {
        case 0:
            drawMenuOption(1, "Type");
            drawMenuOption(5, "Home");
            drawMenuOption(4, "Settings");
            break;
        case 1:
            drawMenuOption(5, "Send");
            drawMenuOption(4, "Cancel");
            enterTypeMode();
            break;
        case 2:
            drawMenuOption(5, "Home");
            break;
        case 3:
            drawMenuOption(5, "Home");
            openSettings();
            break;
        default:
            break;
    }
    currentScreen = screen;

    return 1;
}

uint8_t drawMenuOption(int column, char* text) {
    const int x = 64 * column;
    const int y = 215;
    const int width = 64; //It is STRONGLY advised to keep the width set to 64
    const int height = 25; //It is STRONGLY advised to keep the height set to 25
    const int length = gfx_GetStringWidth(text);

    if(length > width) {
        return 0;
    }

    gfx_SetColor(0);
    gfx_SetTextFGColor(0);
    gfx_SetTextBGColor(1);

    gfx_Rectangle(x - width, y, width, height);
    gfx_PrintStringXY(text, x - (width - ((width - length) / 2)), y + 9);

    return 1;
}

uint8_t setStatusText(const char* text, int color) {
    if (strcmp(lastText, text) == 0) return 1;
    
    gfx_SetColor(2);
    gfx_FillRectangle(0, 0, 320, 20);

    switch (color)
    {
        case 0:
            gfx_SetTextFGColor(1);
            break;
        case 1:
            gfx_SetTextFGColor(3);
            break;
        default:
            gfx_SetTextFGColor(1);
            break;
    }
    
    gfx_SetTextBGColor(2);
    gfx_PrintStringXY(text, 5, 5);
    strncpy(lastText, text, sizeof(lastText) - 1);
    lastText[sizeof(lastText) - 1] = '\0';
    return 1; 
}

uint8_t clearHome(bool needsMenuRedraw) {
    gfx_SetColor(1);
    gfx_FillRectangle(0, 20, 320, 195);

    if(needsMenuRedraw) {
        goToScreen(currentScreen);
    }

    if (lastText[0]) {
        gfx_SetTextFGColor(1);
        gfx_PrintStringXY(lastText, 5, 5);
    }
    return 1;
}

uint8_t sendMessage(const char* mList, bool isSilent) {
    if (serial_connected && strlen(mList) > 1) {
        int w = srl_Write(&srl, mList, strlen(mList));
        if (w < 0) {
            setStatusText("srl_Write err", 0);
            return 0;
        }
        if(!isSilent) setStatusText("Message succesfully sent!", 1);
        return 1;
    } else {
        if(!isSilent) setStatusText("Message not sent", 0);
        return 0;
    }
}

uint8_t openSettings(void) {
    sendMessage("//action:ReadSettings", true);
    setStatusText("Success!", 0);
    return 1;
}

uint8_t handleAction(char* action) {
    if (strcmp(action, "Info") == 0) {
        sendMessage("##0.2", false);
        char nameMsg[132];
        strcpy(nameMsg, "##");
        strncat(nameMsg, name, sizeof(nameMsg) - strlen(nameMsg) - 1);
        sendMessage(nameMsg, false);
        sendMessage("##done", false);
        sendMessage("\n", false);
        return 1;
    }
    return 1;
}

uint8_t displayMessage(char* message) {
    /*if (strlen(message) == 0) {
        return 0;
    }*/

    if(message[strlen(message) - 1] == '\n') {
        message[strlen(message) - 1] = '\0';
    }

    const int xMargin = 5;
    const int yMargin = 5;
    const int fontSize = 8;
    const int messageAreaTop = 20;
    const int messageAreaHeight = 185;
    const int messageAreaWidth = 320;
    const int maxLines = messageAreaHeight / (fontSize + yMargin);

    gfx_SetFontHeight(fontSize);

    if (messageLine >= maxLines) {
        gfx_SetColor(1);
        gfx_FillRectangle(0, messageAreaTop, messageAreaWidth, messageAreaHeight);
        messageLine = 0;
    }

    gfx_SetColor(0);
    gfx_SetTextBGColor(1);
    gfx_SetTextFGColor(0);
    gfx_PrintStringXY(message, xMargin, messageAreaTop + yMargin + (fontSize + yMargin) * messageLine);
    //setStatusText("", 0);
    messageLine++;
    return 1;
}

/*uint8_t seperateString(char originString, char separator) {
    char* second_part;

    char* pos = strstr(originString, separator);

    if (pos != NULL) {
        *pos = '\0';
        char cutString = pos + strlen(separator);

        char* pos2 = strstr(second_part, separator);
    } else {
        return originString;
    }

    return 0;
}*/