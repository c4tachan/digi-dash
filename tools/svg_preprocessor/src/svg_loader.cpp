#include "svg_preprocessor.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <thorvg.h>
#include <memory>

namespace digidash {

// Helper to convert ThorVG color (uint32 AARRGGBB) to Color struct
static Color from_thorvg_color(uint32_t tvg_color) {
    uint8_t a = (tvg_color >> 24) & 0xFF;
    uint8_t r = (tvg_color >> 16) & 0xFF;
    uint8_t g = (tvg_color >> 8) & 0xFF;
    uint8_t b = tvg_color & 0xFF;
    return Color{r, g, b, a};
}

GaugeDocument SvgLoader::load_from_file(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs) {
        throw std::runtime_error("Failed to open SVG file: " + path);
    }
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return load_from_string(buffer.str());
}

GaugeDocument SvgLoader::load_from_string(const std::string& svg) {
    GaugeDocument doc;

    // Initialize ThorVG engine
    tvg::Initializer::init(tvg::CanvasEngine::Sw, 1);

    try {
        // Create an SVG picture from the string data
        auto picture = tvg::Picture::gen();
        auto tvg_data = reinterpret_cast<const char*>(svg.data());
        if (picture->load(tvg_data, svg.size()) != tvg::Result::Success) {
            throw std::runtime_error("Failed to load SVG with ThorVG");
        }

        // Get SVG dimensions
        float w = 0, h = 0;
        picture->viewbox(&w, &h, nullptr, nullptr);

        // Process all shapes in the picture
        _extract_shapes(picture.get(), doc);

        tvg::Initializer::term(tvg::CanvasEngine::Sw);
        return doc;

    } catch (const std::exception& e) {
        tvg::Initializer::term(tvg::CanvasEngine::Sw);
        throw std::runtime_error(std::string("SVG parsing error: ") + e.what());
    }
}

void SvgLoader::_extract_shapes(tvg::Paint* paint, GaugeDocument& doc) {
    if (!paint) return;

    // Handle shape objects (Path, Circle, etc.)
    if (auto shape = dynamic_cast<tvg::Shape*>(paint)) {
        _extract_path_from_shape(shape, doc);
    }

    // Handle groups (recursively extract children)
    if (auto group = dynamic_cast<tvg::Group*>(paint)) {
        group->forEach([this, &doc](tvg::Paint* child) {
            _extract_shapes(child, doc);
            return true;
        });
    }
}

void SvgLoader::_extract_path_from_shape(tvg::Shape* shape, GaugeDocument& doc) {
    if (!shape) return;

    Path path;

    // Get shape ID if available
    // Note: ThorVG doesn't provide direct ID access; we could add it later
    static int path_counter = 0;
    path.id = "path_" + std::to_string(path_counter++);

    // Extract stroke style
    float stroke_width = 1.0f;
    shape->strokeWidth(&stroke_width);
    path.stroke.width = stroke_width;

    uint8_t r, g, b, a;
    if (shape->strokeColor(&r, &g, &b, &a) == tvg::Result::Success) {
        path.stroke.color = Color{r, g, b, a};
    } else {
        path.stroke.color = Color{255, 255, 255, 255};  // Default white
    }

    // Extract fill style
    if (shape->fillColor(&r, &g, &b, &a) == tvg::Result::Success) {
        path.fill.enabled = true;
        path.fill.color = Color{r, g, b, a};
    } else {
        path.fill.enabled = false;
        path.fill.color = Color{0, 0, 0, 255};
    }

    // Extract path commands from the shape's path data
    _extract_path_commands(shape, path);

    if (!path.commands.empty()) {
        doc.paths.push_back(path);
    }
}

void SvgLoader::_extract_path_commands(tvg::Shape* shape, Path& path) {
    if (!shape) return;

    // Get raw path commands from ThorVG
    uint32_t cmd_count = 0;
    const tvg::PathCommand* cmds = nullptr;
    const tvg::Point* pts = nullptr;

    if (shape->pathCoords(&pts) == nullptr) {
        return;  // No path data
    }

    // Get path commands
    uint32_t pt_count = 0;
    if (shape->pathCommands(&cmds, &cmd_count) != tvg::Result::Success) {
        return;
    }

    // Convert ThorVG path commands to our PathCommand format
    uint32_t pt_idx = 0;
    for (uint32_t i = 0; i < cmd_count; ++i) {
        PathCommand cmd;

        switch (cmds[i]) {
            case tvg::PathCommand::MoveTo:
                cmd.type = PathCommand::Type::MoveTo;
                if (pt_idx < pt_count) {
                    cmd.x1 = pts[pt_idx].x;
                    cmd.y1 = pts[pt_idx].y;
                    pt_idx++;
                }
                break;

            case tvg::PathCommand::LineTo:
                cmd.type = PathCommand::Type::LineTo;
                if (pt_idx < pt_count) {
                    cmd.x1 = pts[pt_idx].x;
                    cmd.y1 = pts[pt_idx].y;
                    pt_idx++;
                }
                break;

            case tvg::PathCommand::CubicTo:
                cmd.type = PathCommand::Type::CubicTo;
                if (pt_idx + 2 < pt_count) {
                    cmd.x1 = pts[pt_idx].x;
                    cmd.y1 = pts[pt_idx].y;
                    cmd.x2 = pts[pt_idx + 1].x;
                    cmd.y2 = pts[pt_idx + 1].y;
                    cmd.x3 = pts[pt_idx + 2].x;
                    cmd.y3 = pts[pt_idx + 2].y;
                    pt_idx += 3;
                }
                break;

            case tvg::PathCommand::Close:
                cmd.type = PathCommand::Type::Close;
                break;
        }

        path.commands.push_back(cmd);
    }
}

} // namespace digidash