#include "pix_rotate.h"
#include "get_rotation.h"
#include <cmath>
#include <leptonica/allheaders.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795028841971693993751058209749445923078164
#endif

derot::PixWrap pix_rotate(const Pix* pix, int degrees, bool extend_size) noexcept
{
    double rad = degrees * M_PI / 180;
    auto [width, height] = extend_size ? derot::get_pix_rotation_wh(pix, degrees) : std::make_pair(0, 0);

    // pixRotate ( Pix *pixs, float angle, int type, int incolor, int width, int height );
    return pixRotate(const_cast<Pix*>(pix), rad, L_ROTATE_AREA_MAP, L_BRING_IN_WHITE, width, height);
}