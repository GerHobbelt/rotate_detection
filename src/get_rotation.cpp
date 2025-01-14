#include "get_rotation.h"
#include "utils.h"

#include <cmath>
#include <limits>
#include <thread>
#include <future>
#include <leptonica/allheaders.h> // leptonica

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795028841971693993751058209749445923078164
#endif

namespace {
    using derot::PixWrap;
    using derot::PixRotOpts;

    // Get pix value (0 -- white / 1 -- black)
    int getPixVal(size_t x, size_t y, const uint32_t *data, int32_t wpl) noexcept
    {
        return GET_DATA_BIT(data + y * wpl, x) ? 1 : 0;
    }

    double getEntropy(double p) noexcept
    {
        double q = 1.0 - p;
        // The Rényi entropy (deg = 1/2)
        return (p > 0.0 && p < 1.0) ? 2 * log(sqrt(p) + sqrt(q)) : 0.0;
    }

    double getEntropy(const uint32_t *pixData, size_t width, size_t height, size_t diagonal, unsigned int margin, int32_t wpl, int angle, bool use_vertical) noexcept
    {
        double angle_rad = angle * M_PI / 180;
        double cos_ = cos(angle_rad);
        double sin_ = sin(angle_rad);

        int x_from = diagonal / 2.0 - (height * abs(sin_) + width * abs(cos_)) / 2.0;
        int x_to   = diagonal - x_from;
        int y_from = diagonal / 2.0 - (height * abs(cos_) + width * abs(sin_)) / 2.0;
        int y_to   = diagonal - y_from;

        double entSum = 0;
        for (int y = y_from; y < y_to; ++y)
        {
            size_t blacks = 0;
            for (int x = x_from; x < x_to; ++x)
            {
                
                double xx = x - diagonal / 2.0;
                double yy = y - diagonal / 2.0;

                int x_ = xx * cos_ - yy * sin_ + width  / 2.0 ;
                int y_ = xx * sin_ + yy * cos_ + height / 2.0 ;

                if(x_ >= int(margin) && x_ < int(width - margin) && y_ >= int(margin) && y_ < int(height - margin)) {
                    blacks += getPixVal(x_, y_, pixData, wpl);
                }
            }
            entSum += getEntropy(1.0 * blacks / diagonal) / diagonal;
        }

        if(use_vertical) {
            for (int x = x_from; x < x_to; ++x)
            {
                size_t blacks = 0;
                for (int y = y_from; y < y_to; ++y)
                {
                    double xx = x - diagonal / 2.0;
                    double yy = y - diagonal / 2.0;

                    int x_ = xx * cos_ - yy * sin_ + width  / 2.0 ;
                    int y_ = xx * sin_ + yy * cos_ + height / 2.0 ;

                    if(x_ >= int(margin) && x_ < int(width - margin) && y_ >= int(margin) && y_ < int(height - margin)) {
                        blacks += getPixVal(x_, y_, pixData, wpl);
                    }
                }
                entSum += getEntropy(1.0 * blacks / diagonal) / diagonal;
            }
            entSum /= 2.0;
            
        }
        // std::cout << angle << ", " << entSum << std::endl;
        // TODO: Use sample entropy instead of average to improve quality
        return entSum;
    }

    std::pair<int, double> find_best(const uint32_t *pixData, size_t width, size_t height, size_t diagonal, unsigned int margin, int32_t wpl, const PixRotOpts& opts) noexcept
    {
        int    best_angle = 0;
        double min_ent    = std::numeric_limits<double>::max();

        for (int angle = opts.angle_first; angle <= opts.angle_last; angle += opts.angle_step) {
            double ent = getEntropy(pixData, width, height, diagonal, margin, wpl, angle, !opts.fast);
            if(min_ent > ent) {
                min_ent    = ent;
                best_angle = angle;
            }
        }

        return std::make_pair(-best_angle, min_ent);
    }
}

namespace derot{ //detect rotation

    Pix* get_bw_pix(const Pix* orig_pix, float contrast_factor, int threshold) noexcept
    {
        if(pixGetDepth(orig_pix) == 1) {
            return nullptr;
        }

        const Pix *pix = orig_pix;
        PixWrap gray_pix;
        if(pixGetDepth(orig_pix) > 8) {
            gray_pix = pixConvertRGBToLuminance(const_cast<Pix*>(pix));
            pix      = gray_pix;
        }

        PixWrap ctr_pix {pixContrastTRC(nullptr, const_cast<Pix*>(pix), contrast_factor)};
        return pixConvertTo1(ctr_pix, threshold);
    }

    int get_pix_rotation(const Pix *orig_pix, const PixRotOpts& opts) noexcept
    {
        PixWrap bw_pix = get_bw_pix(orig_pix, opts.contrast_factor, opts.threshold);
        const Pix *pix = bw_pix ? bw_pix : orig_pix;

        size_t width    = pixGetWidth  (pix);
        size_t height   = pixGetHeight (pix);
        int32_t   wpl   = pixGetWpl    (pix);
        size_t diagonal = get_pix_diagonal(pix);
        const uint32_t *pixData = pixGetData(const_cast<Pix*>(pix));


        unsigned int threads = (0 == opts.threads) ? std::thread::hardware_concurrency() : opts.threads;
        if(1 == threads) {
            auto [best_angle, min_ent] = find_best(pixData, width, height, diagonal, opts.margin, wpl, opts);
            return best_angle;
        }

        //
        // Multithreading
        //
        std::vector<std::future<std::pair<int, double>>> tasks;
        tasks.reserve(threads);

        for (auto&& [cur_from, cur_to]: splitRange(opts.angle_first, opts.angle_last, threads)) {
            PixRotOpts o  = opts;
            o.angle_first = cur_from;
            o.angle_last  = cur_to;

            tasks.emplace_back( std::async(std::launch::async, find_best, pixData, width, height, diagonal, opts.margin, wpl, o) );
        }

        int    best_angle = 0;
        double min_ent    = std::numeric_limits<double>::max();
        for(auto& task: tasks) {
            auto [angle, ent] = task.get();
            if(min_ent > ent) {
                min_ent    = ent;
                best_angle = angle;
            }
        }
        
        return best_angle;
    }

    // returns diagonal size to extend image with rotatiion
    size_t get_pix_diagonal(const Pix *pix) noexcept
    {
        size_t width  = pixGetWidth (pix);
        size_t height = pixGetHeight(pix);

        return static_cast<size_t>(sqrt(width * width + height * height) + 0.5);
    }

    // returns [width, height]
    std::pair<int, int> get_pix_rotation_wh(const Pix *pix, int angle) noexcept
    {
        int width  = pixGetWidth (pix);
        int height = pixGetHeight(pix);
        double angle_rad = angle * M_PI / 180;
        double cos_ = abs(cos(angle_rad));
        double sin_ = abs(sin(angle_rad));

        return std::make_pair(
            width * sin_ + height * cos_,
            width * cos_ + height * sin_
        );
    }
}