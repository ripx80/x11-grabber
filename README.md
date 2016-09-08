# x11-grabber
C implementation to get screenshots of mapped and unmapped X11 Windows and you can shot windows by their pid.

### Requirements

libjpeg (tested: libjpeg-turbo 1.5.0-1)
libxmu  (tested: libxmu 1.1.2-1)
libx11  (tested: libx11 1.6.3-1)

### Build Info
gcc -o x -lX11 -ljpeg -lXmu -pthread x.c

### Functions 

get window informations
get screen informations
Screenshots
 * take full screenshot
 * take focused screenshot
 * take from all Windows (seperate)
 * take screenshot from pid
 * Child processes not supported

### Comming Soon 
 * Keylogging
 * Live Screen Recording
 * Clipboard selection (Xsel) - Manipulate the x selection

At the moment you can use ffmpeg for live capturing. comming in new versions.

   
* ffmpeg -video_size 1024x768 -framerate 25 -f x11grab -i :0.0+100,200 -f alsa -ac 2 -i hw:0 output.mkv
* ffmpeg -video_size 1024x768 -framerate 25 -f x11grab -i :0.0+100,200 -f pulse -ac 2 -i default output.mkv
 
fullscreen and lostless
* ffmpeg -video_size 1920x1080 -framerate 30 -f x11grab -i :0.0 -c:v libx264 -qp 0 -preset ultrafast capture.mkv
     
### HowTo use
 
**root window screenshot**

./x -f test.jpg
 
**focused window screenshot**

./x -m1 -f test.jpg
 
**shot all mapped windows**

./x -m2 -f test
 
**shot all mapped windows and wait for unmapped to shot**

./x -m2 -u -f test

**shot window by pid**

./x -m3 -p20236 -f test.jpg
 
**shot window by pid with delay of 2 sec**

./x -m3 -p20236 -d2 -f test.jpg
