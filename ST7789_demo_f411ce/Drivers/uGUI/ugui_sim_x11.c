// Based on Fork: https://github.com/MarioIvancic/UGUI/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <execinfo.h>
#include <signal.h>

#include "ugui_sim.h"

#define BUFFER_SIZE         ((handle->simX * simCfg->screenMultiplier) * (handle->simY * simCfg->screenMultiplier)) * sizeof(UG_U32)

//Data Defines
typedef struct x11_data_s
{
    Display *dis;
    Window win;
    GC gc;
    Visual *visual;
    int screen;
    uint32_t *imgBuffer;
    int simX;
    int simY;
} x11_data_t;

//Global Vars -- I hate these too
x11_data_t *handle;
simcfg_t *simCfg;
UG_DEVICE device;

//Internal function declarations
void x11_pset(UG_S16 x, UG_S16 y, UG_COLOR c);
void x11_flush(void);
bool x11_setup(int width, int height);
void x11_process();

void handler(int sig) {
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

int main (void)
{
    signal(SIGSEGV, handler);   // install our handler
    signal(SIGBUS, handler);   // install our handler

    printf("Simulation Application\n");

    // Get config
    simCfg = GUI_SimCfg();

    //Setup X
    if (true != x11_setup(simCfg->width, simCfg->height))
    {
        printf("Error Initializing X11 driver\n");
        return 0;
    }
    //Setup UGUI
    device.x_dim = simCfg->width;
    device.y_dim = simCfg->height;
    device.pset = &x11_pset;
    device.flush = &x11_flush;

    GUI_Setup(&device);
    while (true)
    {
        GUI_Process();
        x11_process();
        usleep(100000);
    }
    return 0;
}

bool x11_setup(int width, int height)
{
    //Mem Alloc's
    handle = (x11_data_t *)malloc(sizeof(x11_data_t));
    if (NULL == handle)
        return false;

    handle->imgBuffer = (UG_U32 *)malloc(((width * simCfg->screenMultiplier) * (height * simCfg->screenMultiplier)) * sizeof(UG_U32));
    if (NULL == handle->imgBuffer)
        return false;
    handle->simX = width;
    handle->simY = height;

    //Setup X
    unsigned long black,white;

    handle->dis = XOpenDisplay((char *)0);
    handle->screen = DefaultScreen(handle->dis);
    black = BlackPixel(handle->dis, handle->screen),
    white = WhitePixel(handle->dis, handle->screen);
    handle->win = XCreateSimpleWindow(handle->dis,
                                  DefaultRootWindow(handle->dis),
                                  0,
                                  0,
                                  width * simCfg->screenMultiplier + (simCfg->screenMargin * 2),
                                  height * simCfg->screenMultiplier + (simCfg->screenMargin * 2),
                                  5,
                                  black,
                                  simCfg->windowBackColor);
    XSetStandardProperties(handle->dis, handle->win, "uGUI Window",
                         "uGUI", None, NULL, 0, NULL);
    XSelectInput(handle->dis, handle->win,
               ExposureMask|ButtonPressMask|
               ButtonReleaseMask|KeyPressMask|Button1MotionMask);
    handle->gc = XCreateGC(handle->dis, handle->win, 0,0);
    XSetBackground(handle->dis, handle->gc, white);
    XSetForeground(handle->dis, handle->gc, black);
    XClearWindow(handle->dis, handle->win);
    XMapRaised(handle->dis, handle->win);
    handle->visual = DefaultVisual(handle->dis, 0);

    return true;
}

// http://www.mi.uni-koeln.de/c/mirror/www.cs.curtin.edu.au/units/cg252-502/notes/lect5h1.html

//Process Function
void x11_process(void)
{
    XEvent event;
    uint32_t *ximage = (uint32_t *)malloc(BUFFER_SIZE);

    //Check for events
    while (XCheckMaskEvent(handle->dis,
        ButtonPressMask | ButtonReleaseMask | Button1MotionMask | PointerMotionMask, &event) == true)
    {
        #if defined(UGUI_USE_TOUCH)
        static int mouse_down;
        switch (event.type)
        {
            case ButtonPress:
                // we are interested only in LEFT button (button 1)
                if(event.xbutton.button != 1)
                    break;
                mouse_down = 1;
                UG_TouchUpdate((event.xbutton.x - simCfg->screenMargin) / simCfg->screenMultiplier, (event.xbutton.y - simCfg->screenMargin) / simCfg->screenMultiplier, TOUCH_STATE_PRESSED);
            break;

            case ButtonRelease:
                // we are interested only in LEFT button (button 1)
                if(event.xbutton.button != 1)
                    break;
                mouse_down = 0;
                UG_TouchUpdate(-1, -1, TOUCH_STATE_RELEASED);
            break;

            case MotionNotify:
                if(mouse_down && ( event.xmotion.state & Button1Mask ))
                {
                    UG_TouchUpdate((event.xmotion.x - simCfg->screenMargin) / simCfg->screenMultiplier, (event.xmotion.y - simCfg->screenMargin) / simCfg->screenMultiplier, TOUCH_STATE_PRESSED);
                }
            break;
        }
        #endif
    }
    if (ximage != NULL)
    {
        memcpy(ximage, handle->imgBuffer, BUFFER_SIZE);
        XImage *img = XCreateImage(handle->dis,
                                   handle->visual,
                                   24,
                                   ZPixmap,
                                   0,
                                   (char *)ximage,
                                   handle->simX * simCfg->screenMultiplier,
                                   handle->simY * simCfg->screenMultiplier,
                                   32,
                                   0);
        XPutImage(handle->dis,
              handle->win,
              handle->gc,
              img,
              0,
              0,
              simCfg->screenMargin,
              simCfg->screenMargin,
              handle->simX * simCfg->screenMultiplier,
              handle->simY * simCfg->screenMultiplier);
        XDestroyImage(img);
        XFlush(handle->dis);
    }
}

//Internal
void x11_pset(UG_S16 x, UG_S16 y, UG_COLOR c)
{
    UG_U32 tmp = c;

#if defined(UGUI_USE_COLOR_BW)
    /* Convert B/W to RGB888 */
    tmp = c == C_WHITE ? 0xFFFFFF : 0x000000;
#elif defined(UGUI_USE_COLOR_RGB565)
    /* Convert RGB565 to RGB888 */
    tmp = _UG_ConvertRGB565ToRGB888(c);
#endif

    for(UG_U8 i = 0; i < simCfg->screenMultiplier ; i++) {
        for(UG_U8 j = 0; j < simCfg->screenMultiplier ; j++) {
            handle->imgBuffer[(simCfg->width * simCfg->screenMultiplier * ((y * simCfg->screenMultiplier) + j)) + (x * simCfg->screenMultiplier) + i] = tmp;
        }
    }
}

void x11_flush(void)
{
    // nop
}

static const char* message_type[] = {
    "NONE",
    "WINDOW",
    "OBJECT"
};
static const char* event_type[] = {
    "NONE",
    "PRERENDER",
    "POSTRENDER",
    "PRESSED",
    "RELEASED"
    };

void decode_msg(UG_MESSAGE* msg)
{
    printf("%s %s for ID %d (SubId %d)\n", 
        message_type[msg->type],
        event_type[msg->event],
        msg->id, msg->sub_id);
}
