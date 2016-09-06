#include <ctype.h>
#include <stdio.h>
#include "jpeglib.h"

int write_jpeg(XImage *img, const char* filename)
{
    FILE* outfile;
    unsigned long pixel;
    int x, y;
    char* buffer;
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer;
    
    debug("Save to:%s\n",filename);

    outfile = fopen(filename, "wb");
    if (!outfile) {
        fprintf(stderr, "Failed to open output file %s", filename);
        return 1;
    }

    /* collect separate RGB values to a buffer */
    buffer = malloc(sizeof(char)*3*img->width*img->height);
    for (y = 0; y < img->height; y++) {
        for (x = 0; x < img->width; x++) {
            pixel = XGetPixel(img,x,y);
            buffer[y*img->width*3+x*3+0] = (char)(pixel>>16);
            buffer[y*img->width*3+x*3+1] = (char)((pixel&0x00ff00)>>8);
            buffer[y*img->width*3+x*3+2] = (char)(pixel&0x0000ff);
        }
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = img->width;
    cinfo.image_height = img->height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 85, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer = (JSAMPROW) &buffer[cinfo.next_scanline
                      *(img->depth>>3)*img->width];
        jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }
    free(buffer);
    jpeg_finish_compress(&cinfo);
    fclose(outfile);

    return 0;
}
