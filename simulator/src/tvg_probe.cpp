#include <thorvg.h>

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <vector>

// Render an SVG with ThorVG SW canvas into a buffer and optionally dump to PPM.
// Returns true on successful render, false otherwise.
bool tvg_probe_render(const char* path, int width, int height, const char* ppm_out) {
    if (!path || width <= 0 || height <= 0) {
        return false;
    }

    if (tvg::Initializer::init(tvg::CanvasEngine::Sw, 0) != tvg::Result::Success) {
        return false;
    }

    auto canvas = tvg::SwCanvas::gen();
    if (!canvas) {
           tvg::Initializer::term(tvg::CanvasEngine::Sw);
        return false;
    }

    std::vector<uint32_t> buffer(static_cast<size_t>(width) * static_cast<size_t>(height), 0);
        if (canvas->target(buffer.data(), width, width, height, tvg::SwCanvas::Colorspace::ARGB8888) != tvg::Result::Success) {
        tvg::Initializer::term(tvg::CanvasEngine::Sw);
        return false;
    }

    auto picture = tvg::Picture::gen();
    if (!picture) {
        tvg::Initializer::term(tvg::CanvasEngine::Sw);
        return false;
    }

    if (picture->load(path) != tvg::Result::Success) {
        tvg::Initializer::term(tvg::CanvasEngine::Sw);
        return false;
    }

    // Scale to target size
    picture->size(static_cast<float>(width), static_cast<float>(height));
    canvas->push(std::move(picture));

    if (canvas->draw() != tvg::Result::Success || canvas->sync() != tvg::Result::Success) {
        tvg::Initializer::term(tvg::CanvasEngine::Sw);
        return false;
    }

    // Optional PPM dump for visual inspection
    if (ppm_out) {
        std::ofstream ppm(ppm_out, std::ios::binary);
        if (ppm.good()) {
            ppm << "P6\n" << width << " " << height << "\n255\n";
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    uint32_t argb = buffer[static_cast<size_t>(y) * width + x];
                    uint8_t a = static_cast<uint8_t>((argb >> 24) & 0xFF);
                    uint8_t r = static_cast<uint8_t>((argb >> 16) & 0xFF);
                    uint8_t g = static_cast<uint8_t>((argb >> 8) & 0xFF);
                    uint8_t b = static_cast<uint8_t>(argb & 0xFF);
                    // Simple alpha premultiply-to-RGB (assumes straight alpha input)
                    r = static_cast<uint8_t>((r * a) / 255);
                    g = static_cast<uint8_t>((g * a) / 255);
                    b = static_cast<uint8_t>((b * a) / 255);
                    ppm.put(static_cast<char>(r));
                    ppm.put(static_cast<char>(g));
                    ppm.put(static_cast<char>(b));
                }
            }
            ppm.close();
        }
    }

    tvg::Initializer::term(tvg::CanvasEngine::Sw);
    return true;
}
