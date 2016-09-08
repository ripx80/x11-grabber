#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <errno.h>
#include <locale.h>
#include "std_fun.c"
#include "jpeg.c"
#include <pthread.h>
/*
 * [-- Module Info --]
 * 
 *   _____  _            ___   ___  
 *  |  __ \(_)          / _ \ / _ \ 
 *  | |__) |_ _ ____  _| (_) | | | |
 *  |  _  /| | '_ \ \/ /> _ <| | | |
 *  | | \ \| | |_) >  <| (_) | |_| |
 *  |_|  \_\_| .__/_/\_\\___/ \___/ 
 *           | |                    
 *           |_|         
 * Name: X11 Module
 * ID:
 * Author: Ripx80
 * Date: 01.09.2015
 * 
 * [-- Functions ---]
 * 
 * get window informations
 * get screen informations
 * Screenshots
 *      take full screenshot
 *      take focused screenshot
 *      take from all Windows (seperate)
 *      take screenshot from pid
 *          - Child processes not supported
 * 
 * Using Events on Value of the X11 unmapped variable
 * comming:      
 * 
 * Module/Septeration compatibility
 * 
 * Live Capture     
 *      Xsession capture to mp4     
 *      ffmpeg -video_size 1024x768 -framerate 25 -f x11grab -i :0.0+100,200 -f alsa -ac 2 -i hw:0 output.mkv
 *      ffmpeg -video_size 1024x768 -framerate 25 -f x11grab -i :0.0+100,200 -f pulse -ac 2 -i default output.mkv
 * 
 *      fullscreen and lostless
 *      ffmpeg -video_size 1920x1080 -framerate 30 -f x11grab -i :0.0 -c:v libx264 -qp 0 -preset ultrafast capture.mkv
 * 
 * Keylogging
 * Live Screen Recording
 * Clipboard selection (Xsel) - Manipulate the x selection
 * 
 * 
 *  [-- Build Info --]
 * 
 * 
 * gcc -o x -lX11 -ljpeg -lXmu -pthread x.c
 * 
 * [-- Advanced Info --]
 * on Arch:
 * pacman -S xorg-xwininfo #for infos about the windows ;-)
 * xwininfo -tree -root
 *  - use this for window informations. This is implemented with xcb.
 * 
 * 
 * https://stackoverflow.com/questions/17599240/is-there-a-more-efficent-way-to-shadow-a-users-x11-session-than-what-im-doing
 * executing commands with root privs...
 * https://trac.ffmpeg.org/wiki/Capture/Desktop
 * http://ubuntuforums.org/showthread.php?t=2119648
 * 
 * [-- HowTo use --]
 * 
 * #root screenshot
 * ./x -f test.jpg
 * 
 * # focused window screenshot
 * ./x -m1 -f test.jpg
 * 
 * # shot all mapped windows
 * ./x -m2 -f test
 * 
 * # shot all mapped windows and wait for unmapped to shot
 * ./x -m2 -u -f test
 *  
 * # shot window by pid
 * ./x -m3 -p20236 -f test.jpg
 * 
 * # shot window by pid with delay of 2 sec
 * ./x -m3 -p20236 -d2 -f test.jpg
 * 
 */

Bool xerror = False;

struct ptargs {
    Window *win;
    Display *display;
    int ptnum;
    char fn[220];
};

pthread_mutex_t mutex;
short shot_all = 0;

struct xwin {
    Display * display;
    XWindowAttributes attr;    
    int screen;
    Window root;
    Window window;
    Pixmap pixmap;
    GC gc;
    XGCValues gcv;
    XFontStruct * font;
    unsigned long black_pixel;    
    unsigned long white_pixel;
};

struct xwin_list{
    Window *list;
    unsigned long len;        
};

/* ** Begin X Functions ** */

Window get_focus_window(Display* d){
    Window w;
    int revert_to;
    debug("getting input focus window ... ");
    XGetInputFocus(d, &w, &revert_to);
    
    if(xerror){
        printf("fail\n");
        exit(1);
    }else if(w == None){
        printf("no focus window\n");
        exit(1);
    }else{
        printf("success (window: %d)\n", (int)w);
    }
    return w;
}

Window *winlist (Display *disp, unsigned long *len) {
	Atom prop = XInternAtom(disp,"_NET_CLIENT_LIST",False), type;
	int form;
	unsigned long remain;
	unsigned char *list;

	errno = 0;
	if (XGetWindowProperty(disp,XDefaultRootWindow(disp),prop,0,1024,False,XA_WINDOW,
                &type,&form,len,&remain,&list) != Success) {
		perror("winlist() -- GetWinProp");
		return 0;
	}
	return (Window*)list;
}

char *winame (Display *disp, Window win) {
	Atom prop = XInternAtom(disp,"WM_NAME",False), type;
	int form;
	unsigned long remain, len;
	unsigned char *list;

	errno = 0;
	if (XGetWindowProperty(disp,win,prop,0,1024,False,XA_STRING,
                &type,&form,&len,&remain,&list) != Success) {
		perror("winlist() -- GetWinProp");
		return NULL;
	}
	return (char*)list;
}

void print_named_tree(Display *display, Window *list, long unsigned len){
    char *name;
    int i;
    for (i=0;i<len;i++) {
        name = winame(display,list[i]);
        printf("-->%s<--\n",name);
        free(name);
    }
}

//~ void print_window_class(Display* d, Window w){
  //~ Status s;
  //~ XClassHint* class;
  //~ class = XAllocClassHint();   
  //~ if(xerror) printf("[ERROR]: XAllocClassHint\n");
  //~ s = XGetClassHint(d, w, class);
  //~ if(xerror || s)
    //~ printf("\tname: %s\n\tclass: %s\n", class->res_name, class->res_class);
  //~ else
    //~ printf("[ERROR]: XGetClassHint\n");  
//~ }

//~ void window_info(Display* display,Window win,XWindowAttributes attrs,int screen){
    //~ printf("[*] Display Infos:\n\tscreen: %d\n\tresolution:%dx%d\n",screen,attrs.width,attrs.height);
    //~ printf("\tdepth: %d\n",attrs.depth);   
    //~ print_window_class(display,win);
//~ }

Display* get_def_dpl(){
    Display* display;
    display = XOpenDisplay(":0.0");
    if(display == NULL) {
        fprintf(stderr, "cannot connect to X server %s\n",getenv("DISPLAY") ? getenv("DISPLAY") : "(default)");
        exit(1);
    }   
    return display;
}


int take_screenshot(struct xwin cwin, const char* filename){
    //printf("Taking screenshot now on Window: %d, display:%d screen: %d, x: %d y: %d depth:%d Colormap: %d,Screen: %d Root: %d Visual: %d\n",    
    //cwin.window,cwin.display,cwin.attr.screen,cwin.attr.x,cwin.attr.y,cwin.attr.depth,cwin.attr.colormap,cwin.attr.screen,cwin.attr.root,cwin.attr.visual);
    XImage* img;
    unsigned int buffer_size;
    
    cwin.gcv.function = GXcopy;    
    cwin.gcv.subwindow_mode = IncludeInferiors;
    cwin.gc = XCreateGC (cwin.display, cwin.window, GCFunction | GCSubwindowMode, &cwin.gcv);
    debug("build the pixmap...\n");

    cwin.pixmap = XCreatePixmap(cwin.display, cwin.window, cwin.attr.width, cwin.attr.height, cwin.attr.depth);
    XSetForeground(cwin.display, cwin.gc, BlackPixelOfScreen(DefaultScreenOfDisplay(cwin.display)));
    XFillRectangle(cwin.display, cwin.pixmap, cwin.gc, 0, 0, cwin.attr.width, cwin.attr.height);
    XCopyArea(cwin.display, cwin.window, cwin.pixmap, cwin.gc, 0, 0, cwin.attr.width, cwin.attr.height, 0, 0);
    //img = XGetImage(cwin.display, cwin.pixmap, 0, 0, cwin.attr.width, cwin.attr.height,AllPlanes, ZPixmap);
    img = XGetImage(cwin.display, cwin.pixmap, 0, 0, cwin.attr.width, cwin.attr.height,AllPlanes, XYPixmap);
    XFreePixmap(cwin.display,cwin.pixmap);
    
    if(!img){
        fprintf(stderr, "Can't create ximage\n");
        return 1;
    }    
    if(img->depth != 24){
        fprintf(stderr, "Image depth is %d. Must be 24 bits.\n", img->depth);
        return 1;
    }    
    if(write_jpeg(img, filename)){
        printf("JPEG file successfully written to %s\n",filename);
    }    
    XDestroyImage(img);
    return 0;   
}

//thread function
void * SeperateWindowShot(void *context){
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    struct ptargs *args = context;

    XEvent event;
    struct xwin cwin;
    cwin.window = *args->win;
    cwin.display= args->display;    
    
    XSelectInput(cwin.display, cwin.window, (        
        StructureNotifyMask|
        SubstructureNotifyMask|
        SubstructureRedirectMask|
        VisibilityChangeMask
    ));        
    //XMapWindow(cwin.display, cwin.window);
    
    Bool wait_shot = True;
    //check input length on cmd line parsing       
    //char fn[100];
    
    //must use delays to wait for rendering on multiple virtual desktops...
    while(wait_shot){
        //printf("waiting for event on Window: %d\n",cwin.window);        
        //XPeekEvent(cwin.display, &event);
        XNextEvent(cwin.display, &event);
        switch(event.type){
            //UnmapNotify
            case 18:
            //MapNotify
            case 19:              
              XGetWindowAttributes(cwin.display, cwin.window, &cwin.attr);  
              if(cwin.attr.map_state == 0 && cwin.attr.backing_store != Always){
                    debug("Mapping not for me...\n");
                    sleep(2);                                                    
               }else{              
                usleep(500);
                take_screenshot(cwin, args->fn);
                wait_shot = False;
                }
            break;
            //NoExpose
            case 14:
                debug("No Expose: %d\n",event.type);                
            break;
            default:
                debug("Event being thrown away: %d\n",event.type);
                sleep(1);
            break;
        }     
    } 
    debug("go exit\n");
    pthread_mutex_lock (&mutex); 
    shot_all++;   
    pthread_mutex_unlock (&mutex);
    return NULL;
}
/* ** End X Functions ** */

/* ** Begin Helper Functions ** */
void print_help(char *name){
    printf("\n");
    printf("usage: %s [options] -f <filname>\n\n",name);    
    printf("\t[ -d <seconds>]\t delay in seconds\n");
    printf("\t[ -m 1|2|3\t set mode, 1=root shot, 2=shot all windows, 3=shot by pid\n");
    printf("\t[ -s raw|jpeg ]\t save screenshot in format\n");
    printf("\t[ -i ]\t\t print info of shot\n");
    printf("\t[ -v [0|1|2] ]\t verbose\n");
    printf("\n");    
}

Window get_screenshot_mode(Display* display, int screen,int mode){    
    Window win;
    switch(mode){
        case 1:
            debug("[*] Mode: focused screen shot\n");            
            win = get_focus_window(display);
            //win = get_top_window(display, win);
            //win = get_named_window(display, win);
        break;
        case 2:
            debug("[*] Mode: Shot them all!\n");                             
            win = RootWindow(display, screen);
        break;
        
        default:
            printf("[*] Mode: full screen shot\n");
            win = RootWindow(display, screen);
        break;        
    }
    return win;    
}

int w2pid(Window win,Display * display, unsigned long pid){   
    Atom           type;
    int            format;
    unsigned long  nItems;
    unsigned long  bytesAfter;
    unsigned char *propPID = 0;
    display = get_def_dpl();
    
    Atom pid_atom = XInternAtom(display,"_NET_WM_PID",False);
    if (pid_atom == None){
         printf("Error: No Such Atom\n");
    }
    
    if(Success == XGetWindowProperty(display, win, pid_atom, 0, 1, False, XA_CARDINAL,
                                     &type, &format, &nItems, &bytesAfter, &propPID)){
        if(propPID != 0){
        // If the PID matches, add this window to the result set.
            if(pid == *((unsigned long *)propPID)){
                debug("Found Window with id: %d\n",win);
                return 1;
            }else{
                debug("Not Window: %d, pid: %d, prop_id: %d\n",win,pid,*((unsigned long *)propPID));
                return 0;
            }
            XFree(propPID);
        }
    }else{
        debug("Not Success\n");
    }
    
// Recurse into child windows.
    //~ Window    wRoot;
    //~ Window    wParent;
    //~ Window   *wChild;
    //~ unsigned  nChildren;
    //~ Window res_win;
    //~ if(0 != XQueryTree(display, win, &wRoot, &wParent, &wChild, &nChildren)){
        //~ for(unsigned i = 0; i < nChildren; i++){
            //~ //debug("Go to Child: %d with pid: %d\n",wChild[i],pid);
            //~ if(w2pid(wChild[i],display,pid) != 0)
                //~ return 1;
        //~ }
    //~ }
}
/* ** End Helper Functions ** */

int main(int argc, char **argv) {
    setlocale(LC_ALL, "");
    uint8_t delay = 0;
    uint8_t mode = 0;
    uint8_t fmt = 0; //0 = jpg, 1 = raw
    int time=0;
    Bool info = False; //info only
    char filename[200];   
    char fn_buffer[220];   
    memset(filename, '\0', sizeof(filename));
    memset(fn_buffer, '\0', sizeof(fn_buffer));
    int index;
    int c;
    int pid=0;
    int uwait=0;
    opterr = 0;

    if(argc < 2){
        printf("to less arguments, you need a output file\n");
        print_help(argv[0]);
        exit(1);
    }
    while ((c = getopt (argc, argv, "uid:p:m:s:v:f:")) != -1){
        switch (c){
            case 'd':
                delay = s2ui8(optarg);            
            break;
            case 'm':
                mode = s2ui8(optarg);
            break;
            case 'i':
                info = True; 
            break;
            case 's':
                fmt = s2ui8(optarg);      
            break;
            case 'u':
                uwait = 1;
            break;
            case 'v':                
                DEBUG = (Bool) s2ui8(optarg);      
                debug("Debug Mode set\n");
            break; 
            case 'p':                
                pid = s2i(optarg);
            break;   
            case 'f':        
                strncpy(filename, optarg,199);
            break;
            case '?': 
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                if (isprint (optopt))
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
                    print_help(argv[0]);
                    return 1;
            break;
            default:
                print_help(argv[0]);
                abort ();
          }  
    }
    for (index = optind; index < argc; index++){
        printf ("Non-option argument %s\n", argv[index]);
        return 1;
    }
    
    if(strlen(filename) == 0){
        printf("No output file. go exit\n");
        exit(1);
    }
    
    debug("delay = %d, mode = %d, fmt = %s, debug = %d, filename = %s\n",
          delay, mode, fmt,DEBUG,filename);
    
    debug("Begin delay...");
    sleep(delay);
    //init vars
/* --------------------------------------------------*/
    struct xwin xroot,cwin;
    struct xwin_list xlist;    
    XInitThreads();  //needed by X server for access with threads
    
    //setting up the root window
    xroot.display = get_def_dpl();
    xroot.screen = DefaultScreen(xroot.display); 
    xroot.window = XDefaultRootWindow(xroot.display);
    XGetWindowAttributes(xroot.display, xroot.window, &xroot.attr); 
    
    //set window defaults
    cwin.display = xroot.display;
    debug("Root Window %d on Display: %d",xroot.window,xroot.display);

    //switch the screenshot mode
    switch(mode){
        case 1:{
            debug("[*] Mode: focused screen shot\n");
            cwin.window = get_focus_window(xroot.display);            
            XGetWindowAttributes(cwin.display, cwin.window, &cwin.attr); 
            take_screenshot(cwin,filename);
            //win = get_top_window(display, win);
            //win = get_named_window(display, win);
        }
        break;
        case 2:{
            debug("[*] Mode: Shot them all!\n"); 
            // Make Screenshots of all Windows... Unmapped must wait
            /* --------------------------------------------------*/
                xlist.list = winlist(xroot.display,&xlist.len);
                print_named_tree(xroot.display,xlist.list,xlist.len);
                
                const int winc = 50;
                //implement dynamic allocation of this array...
                Window * ptr[winc]; 
                unsigned int ulen=0;
                int i;    
               
                for (i=0; (i < xlist.len) || (i > (winc-1));i++) {    
                    struct xwin cwin;
                    cwin.window = xlist.list[i];
                    XGetWindowAttributes(xroot.display, cwin.window, &cwin.attr);
                    //rebuild context
                    cwin.screen = xroot.screen;
                    cwin.display = xroot.display;
                
                    debug("Window: %d\nAttr:%dx%d\n",(int) cwin.window,cwin.attr.width,cwin.attr.height);
                    if(cwin.attr.map_state == 0 && cwin.attr.backing_store != Always){
                        ptr[ulen] = &xlist.list[i];            
                        ++ulen;
                    }else{                        
                        debug("taking mapped screenshot\n");                        
                        sprintf(fn_buffer,"m_%s-%d.jpg",filename,i);                  
                        take_screenshot(cwin,fn_buffer);
                        memset(fn_buffer, '\0', sizeof(fn_buffer));                        
                    }
                }
                debug("Unmapped Windows %d: \n",ulen);
                if(uwait==0){
                    debug("Not waiting for unmapped windows use -u to wait...\n");
                }else{                   
                    // X Event Watcher
                    /* --------------------------------------------------*/    
                    pthread_mutex_t mutex;
                    pthread_t worker[10]; //worker list
                    struct ptargs args[10]; //worker args                     
                    int workercnt = 0; //current using    
                    pthread_mutex_init (&mutex, NULL);
                    
                    for(i=0;i<ulen;i++){        
                        args[i].win=ptr[i];
                        args[i].display=xroot.display;
                        args[i].ptnum = i; 
                        memset(args[i].fn, '\0', sizeof(args[i].fn));  
                        sprintf(args[i].fn,"u_%s-%d.jpg",filename,i); 
                        
                        pthread_create(&(worker[i]), NULL, SeperateWindowShot, (void *)&args[i]);        
                        pthread_detach(worker[i]);
                    }    
                    while(shot_all != ulen){     
                        sleep(2);
                    }
            	}
            }         
        break;
        case 3:{
            debug("Mode: Process Shot");
            xlist.list = winlist(xroot.display,&xlist.len);
            const int winc = 50;
            Window * ptr[winc]; 
            unsigned int ulen=0;
            int wi,i = 0;    
            //double code... optimize that...
            for (i=0; (i < xlist.len) || (i > (winc-1));i++) {
                struct xwin cwin;
                cwin.window = xlist.list[i];
                XGetWindowAttributes(xroot.display, cwin.window, &cwin.attr);
                //rebuild context
                cwin.screen = xroot.screen;
                cwin.display = xroot.display;
                if(w2pid(cwin.window,cwin.display,pid)){
                    wi=1;
                    if(cwin.attr.map_state == 0 && cwin.attr.backing_store != Always){
                        if(uwait == 1){
                        debug("Founded Window by pid is unmapped.. waiting\n");
                        unsigned int ulen=1;
                        pthread_t worker;
                        struct ptargs args;
                        args.win=&cwin.window;
                        args.display=xroot.display;
                        args.ptnum = 1; 
                        memset(args.fn, '\0', sizeof(args.fn));
                        sprintf(args.fn,"u_%s-pid-%d.jpg",filename,pid); 
                                              
                        pthread_create(&worker, NULL, SeperateWindowShot, (void *)&args);        
                        pthread_detach(worker);
                        while(shot_all != ulen){     
                            sleep(2);
                        }
                    }else{
                        debug("Not waiting for unmapped windows use -u to wait...\n");
                    }
                    }else{                    
                        debug("Shooting window: %d\n",cwin.window);
                        take_screenshot(cwin,filename);
                    }
                    break;
                }
            }
            if(wi==0) warn("Canott found Window with pid: %d",pid);
        }break;
        //comming soon.. 
        //~ case 4:{
            //~ debug("Mode: Keylogging focused window");            
                //~ XEvent event;   
    
                
                //~ cwin.window = get_focus_window(display);
                //~ cwin.display= xroot.display;    
                //~ XSelectInput(cwin.display, cwin.window, (        
                    //~ StructureNotifyMask|
                    //~ SubstructureNotifyMask|
                    //~ SubstructureRedirectMask|
                    //~ VisibilityChangeMask|
                //~ ));  
        //~ }
        //~ break;        
        default:{
            debug("Mode: full screen shot\n");                     
            take_screenshot(xroot,filename);
        }
        break;        
    }
    //window_info(display,win,attrs,screen);
    return 0;
}
