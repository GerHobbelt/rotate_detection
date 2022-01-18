#include "pix_rotate.h"
#include <cmath>
#include "get_rotation.h"
#include <allheaders.h>

PixWrap pix_rotate(const Pix* pix, int degrees, bool extend_size) noexcept
{
    double rad = degrees * M_PI / 180;
    auto [width, height] = extend_size ? get_pix_rotation_wh(pix, degrees) : std::make_pair(0, 0);

    // pixRotate ( Pix *pixs, float angle, int type, int incolor, int width, int height );
    return PixWrap(pixRotate(const_cast<Pix*>(pix), rad, L_ROTATE_AREA_MAP, L_BRING_IN_WHITE, width, height ));
}