// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file reeb.cpp
 * @brief Display bilinear level lines of singular points.
 * 
 * (C) 2025, Pascal Monasse <pascal.monasse@enpc.fr>
 */

#include "lltree.h"
#include "draw_curve.h"
#include "fill_curve.h"
#include "cmdLine.h"
#include "io_png.h"
#include <map>

struct color_t {
    unsigned char r,g,b;
    color_t(): r(255), g(255), b(255) {}
    color_t(unsigned char r0, unsigned char g0, unsigned char b0)
    :r(r0),g(g0),b(b0) {}
};

struct TransformZoom : public TransformPoint {
    int z;
    TransformZoom(int zoom=1): z(zoom) {}
    Point operator()(const Point& p) const {
        return Point(z*p.x, z*p.y);
    }
};

const color_t WHITE(255,255,255);
const color_t GREEN(0,255,0);

/// Compute histogram of level at pixels at the border of the image.
static void histogram(unsigned char* im, size_t w, size_t h, size_t histo[256]){
    size_t j;
    for(j=0; j<w; j++) // First line
        ++histo[im[j]];
    for(size_t i=1; i+1<h; i++) { // All lines except first and last
        ++histo[im[j]];  // First pixel of line
        j+= w-1;
        ++histo[im[j++]]; // Last pixel of line
    }
    for(; j<w*h; j++) // Last line
        ++histo[im[j]];    
}

/// Put pixels at border of image to value \a v.
static void put_border(unsigned char* im, size_t w, size_t h, unsigned char v) {
    size_t j;
    for(j=0; j<w; j++)
        im[j] = v;
    for(size_t i=1; i+1<h; i++) {
        im[j] = v;
        j+= w-1;
        im[j++] = v;
    }
    for(; j<w*h; j++)
        im[j] = v;
}

/// Set all pixels at border of image to their median level.
static unsigned char fill_border(unsigned char* im, size_t w, size_t h) {
    size_t histo[256] = {0}; // This puts all values to zero
    histogram(im, w, h, histo);
    size_t limit=w+h-2; // Half number of pixels at border
    size_t sum=0;
    int i=-1;
    while((sum+=histo[++i]) < limit);
    put_border(im,w,h, (unsigned char)i);
    return (unsigned char)i;
}

/// Main procedure for curvature microscope.
int main(int argc, char** argv) {
    int z=1;
    CmdLine cmd; cmd.prefixDoc = "\t";
    cmd.add( make_option('z',z,"zoom").doc("Zoom factor (integer)") );
    cmd.process(argc, argv);
    if(argc!=3) {
        std::cerr << "Usage: " << argv[0]
                  << " [options] in.png out.png" << std::endl;
        std::cerr << "Option:\n" << cmd;
        return 1;
    }
    if(z<1) {
        std::cerr << "The zoom factor must be strictly positive" << std::endl;
        return 1;
    }

    size_t w, h;
    unsigned char* in = io_png_read_u8_gray(argv[1], &w, &h);
    if(! in) {
        std::cerr << "Error reading as PNG image: " << argv[1] << std::endl;
        return 1;
    }
    fill_border(in, w, h); // Background gray of output

    // Extract level lines
    LLTree tree(in, (int)w, (int)h, z-1);
    free(in);
    std::cout << tree.nodes().size() << " level lines:" << std::endl;

    // Draw level lines
    TransformZoom t(z);
    w *= z;
    h *= z;
    color_t* out = new color_t[w*h];
    const color_t palette[4] = {color_t(0,0,0),   color_t(0,0,255),
                                color_t(0,255,0), color_t(255,0,0)};
    int stats[4] = {0};
    for(LLTree::iterator it=tree.begin(); it!=tree.end(); ++it) {
        ++stats[it->ll->type];
        color_t color = palette[it->ll->type];
        draw_curve(it->ll->line,color, out,(int)w,(int)h, t);
    }
    std::cout <<   "Min: "     << stats[LevelLine::MIN]
              << ". Max: "     << stats[LevelLine::MAX]
              << ". Saddles: " << stats[LevelLine::SADDLE]
              << '.' << std::endl;

    // Output image
    if(io_png_write_u8(argv[2], (unsigned char*)out, (int)w, (int)h, 3)!=0){
        std::cerr << "Error writing image file " << argv[2] << std::endl;
        return 1;
    }
    delete [] out;

    return 0;
}
