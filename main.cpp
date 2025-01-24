// SPDX-License-Identifier: MPL-2.0
/**
 * @file main.cpp
 * @brief pmbil: Persistence map of bilinear image
 * @author Pascal Monasse <pascal.monasse@enpc.fr>
 * @date 2024
 */

#include "persistence.h"
#include "cmdLine.h"
#include "io_png.h"
#include <algorithm>
using namespace std;

/** \mainpage pmbil.
  * Persistence maps of image obtained by bilinear interpolation of the samples.
*/
int main(int argc, char* argv[]) { 
    // parse arguments
    CmdLine cmd;

    /*    int nScales=0;
    float grad=0;
    
    // options
    cmd.add( make_option('s', nScales, "scales")
             .doc("nb scales (0=automatic)") );
    cmd.add( make_option('g', grad, "gradient")
             .doc("Min gradient norm (0=automatic)") );
    */
    try {
        cmd.process(argc, argv);
    } catch(const std::string& s) {
        std::cerr << "Error: " << s << std::endl;
        return 1;
    }
    if(argc != 4) {
        cerr << "Usage: " << argv[0] << " [options] imgIn.png pm+.png pm-.png\n"
             << cmd;
        return 1;
    }

    size_t w, h;
    float* im = io_png_read_f32_gray(argv[1], &w, &h);
    if(! im) {
        cerr << "Unable to load image " << argv[1] << endl;
        return 1;
    }

    float* pm_min = new float[w*h];
    //std::copy(im, im+w*h, pm_min);
    persistence(im, w, h, pm_min);

    float* pm_max = new float[w*h];
    for(size_t i=w*h; i-->0;)
        im[i] = 255.0f - im[i];
    //std::copy(im, im+w*h, pm_max);
    persistence(im, w, h, pm_max);

    if(io_png_write_f32(argv[2], pm_min, w, h, 1)) {
        cerr << "Unable to save image " << argv[2] << endl;
        return 1;
    }
    if(io_png_write_f32(argv[3], pm_max, w, h, 1)) {
        cerr << "Unable to save image " << argv[3] << endl;
        return 1;
    }
    
    free(im);
    delete [] pm_min;
    delete [] pm_max;
    return 0;
}
