// 
// Simplest wrapper for RAII reason
// 

#include "pixwrap.h"
#include <leptonica/allheaders.h>

namespace derot{ //detect rotation
    
    PixWrap::PixWrap(const char *fname) noexcept :
        pix(pixRead(fname))
    {}

    PixWrap::~PixWrap()
    {
        pixDestroy(&pix);
    }

    bool PixWrap::writePng(const char *fname, float gamma) noexcept
    {
        return pixWritePng(fname, pix, gamma);
    }
    
}