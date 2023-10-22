#include <iostream>
#include <cmath>
#include <leptonica/allheaders.h>
#include "pixwrap.h"
#include "get_rotation.h"

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795028841971693993751058209749445923078164
#endif

using derot::PixWrap;

#if defined(BUILD_MONOLITHIC)
#define main			librotate_detect_example1_main
#endif

int main(int argc, const char** argv)
{
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <from_image_file> <to_file.png>" << std::endl; 
        return 1;
    }

    PixWrap pix {argv[1]};
    
    int angle = derot::get_pix_rotation(pix);
    std::cerr << "angle: " << angle << std::endl; 
    
    double angle_rad = angle * M_PI / 180;    
    
    std::cerr << "width : " << pixGetWidth (pix) << std::endl; 
    std::cerr << "height: " << pixGetHeight(pix) << std::endl; 

    auto [width, height] = derot::get_pix_rotation_wh(pix, angle);
    std::cerr << "width : " << width << std::endl; 
    std::cerr << "height: " << height << std::endl; 

    PixWrap(pixRotate(pix, angle_rad, L_ROTATE_AREA_MAP, L_BRING_IN_WHITE, width, height )).writePng(argv[2]);
    // PixWrap(pixRotate(pix, angle_rad, L_ROTATE_AREA_MAP, L_BRING_IN_WHITE, 0, 0 )).writePng("out-rot.png");

    return 0;
}

// bin/example1.bin ../demo/1-in.jpg ../demo/1-out.png
// bin/example1.bin ../demo/2-in.jpg ../demo/2-out.png
// bin/example1.bin ../demo/3-in.jpg ../demo/3-out.png
// bin/example1.bin ../demo/4-in.jpg ../demo/4-out.png
// bin/example1.bin ../demo/5-in.jpg ../demo/5-out.png