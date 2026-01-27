#include "svg_preprocessor.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <thorvg.h>
#include <iostream>

namespace digidash {

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

    // Initialize ThorVG (SwCanvas for software rasterizer, 0 threads = main thread only)
    if (tvg::Initializer::init(tvg::CanvasEngine::Sw, 0) != tvg::Result::Success) {
        throw std::runtime_error("Failed to initialize ThorVG");
    }

    try {
        // Create a picture and load SVG data
        auto picture = tvg::Picture::gen();
        if (!picture) {
            throw std::runtime_error("Failed to create ThorVG picture");
        }
        
        // Load SVG from memory buffer
        if (picture->load(svg.data(), svg.size(), "svg", false) != tvg::Result::Success) {
            throw std::runtime_error("Failed to parse SVG with ThorVG");
        }

        // Get picture dimensions
        float w, h;
        picture->size(&w, &h);
        doc.width = w;
        doc.height = h;

        std::cout << "ThorVG: Loaded SVG " << w << "x" << h << std::endl;
        
        // Use Accessor to traverse the Picture's internal scene tree
        auto accessor = tvg::Accessor::gen();
        if (!accessor) {
            throw std::runtime_error("Failed to create ThorVG accessor");
        }
        
        // Lambda callback to extract shapes from the scene tree
        auto extractCallback = [](const tvg::Paint* paint, void* data) -> bool {
            auto* docPtr = static_cast<GaugeDocument*>(data);
            
            if (paint->type() == tvg::Type::Shape) {
                // Extract this shape
                auto* shape = const_cast<tvg::Shape*>(static_cast<const tvg::Shape*>(paint));
                
                // Get the path commands and points from the shape
                const tvg::PathCommand* cmds = nullptr;
                uint32_t cmdCnt = shape->pathCommands(&cmds);
                
                const tvg::Point* pts = nullptr;
                uint32_t ptsCnt = shape->pathCoords(&pts);
                
                if (cmdCnt > 0 && ptsCnt > 0) {
                    Path our_path;
                    static int path_counter = 0;
                    our_path.id = "path_" + std::to_string(path_counter++);
                    
                    // Extract stroke style
                    uint8_t stroke_r, stroke_g, stroke_b, stroke_a;
                    shape->strokeColor(&stroke_r, &stroke_g, &stroke_b, &stroke_a);
                    float stroke_width = shape->strokeWidth();
                    
                    our_path.stroke.color = Color{stroke_r, stroke_g, stroke_b, stroke_a};
                    our_path.stroke.width = stroke_width;
                    
                    // Extract stroke cap style
                    tvg::StrokeCap cap = shape->strokeCap();
                    switch (cap) {
                        case tvg::StrokeCap::Butt:
                            our_path.stroke.cap = StrokeLineCap::Butt;
                            break;
                        case tvg::StrokeCap::Round:
                            our_path.stroke.cap = StrokeLineCap::Round;
                            break;
                        case tvg::StrokeCap::Square:
                            our_path.stroke.cap = StrokeLineCap::Square;
                            break;
                    }

                    // Extract fill style
                    uint8_t fill_r, fill_g, fill_b, fill_a;
                    shape->fillColor(&fill_r, &fill_g, &fill_b, &fill_a);
                    
                    our_path.fill.enabled = (fill_a > 0);
                    our_path.fill.color = Color{fill_r, fill_g, fill_b, fill_a};

                    // Extract path commands from ThorVG
                    uint32_t ptIdx = 0;
                    for (uint32_t i = 0; i < cmdCnt; i++) {
                        PathCommand cmd;

                        switch (cmds[i]) {
                            case tvg::PathCommand::MoveTo:
                                if (ptIdx >= ptsCnt) continue;
                                cmd.type = PathCommand::Type::MoveTo;
                                cmd.x1 = pts[ptIdx].x;
                                cmd.y1 = pts[ptIdx].y;
                                ptIdx++;
                                break;

                            case tvg::PathCommand::LineTo:
                                if (ptIdx >= ptsCnt) continue;
                                cmd.type = PathCommand::Type::LineTo;
                                cmd.x1 = pts[ptIdx].x;
                                cmd.y1 = pts[ptIdx].y;
                                ptIdx++;
                                break;

                            case tvg::PathCommand::CubicTo:
                                if (ptIdx + 2 >= ptsCnt) continue;
                                cmd.type = PathCommand::Type::CubicTo;
                                cmd.x1 = pts[ptIdx].x;
                                cmd.y1 = pts[ptIdx].y;
                                cmd.x2 = pts[ptIdx + 1].x;
                                cmd.y2 = pts[ptIdx + 1].y;
                                cmd.x3 = pts[ptIdx + 2].x;
                                cmd.y3 = pts[ptIdx + 2].y;
                                ptIdx += 3;
                                break;

                            case tvg::PathCommand::Close:
                                cmd.type = PathCommand::Type::Close;
                                break;

                            default:
                                continue;
                        }

                        our_path.commands.push_back(cmd);
                    }

                    if (!our_path.commands.empty()) {
                        docPtr->paths.push_back(our_path);
                    }
                }
            }
            
            // Continue traversing
            return true;
        };
        
        // Traverse the picture's scene tree
        if (accessor->set(picture.get(), extractCallback, &doc) != tvg::Result::Success) {
            throw std::runtime_error("Failed to traverse picture scene tree");
        }
        
        tvg::Initializer::term(tvg::CanvasEngine::Sw);
        
        std::cout << "Extracted " << doc.paths.size() << " paths" << std::endl;
        
    } catch (...) {
        tvg::Initializer::term(tvg::CanvasEngine::Sw);
        throw;
    }
    
    return doc;
}

void SvgLoader::_extract_thorvg_shape(tvg::Shape* shape, GaugeDocument& doc) {
    if (!shape) return;

    // Get the path commands and points from the shape
    const tvg::PathCommand* cmds = nullptr;
    uint32_t cmdCnt = shape->pathCommands(&cmds);
    
    const tvg::Point* pts = nullptr;
    uint32_t ptsCnt = shape->pathCoords(&pts);
    
    if (cmdCnt == 0 || ptsCnt == 0) {
        return; // Empty shape
    }

    Path our_path;

    // Extract shape ID (use deprecated API for compatibility)
    static int path_counter = 0;
    our_path.id = "path_" + std::to_string(path_counter++);

    // Extract stroke style
    uint8_t stroke_r, stroke_g, stroke_b, stroke_a;
    shape->strokeColor(&stroke_r, &stroke_g, &stroke_b, &stroke_a);
    float stroke_width = shape->strokeWidth();
    
    our_path.stroke.color = Color{stroke_r, stroke_g, stroke_b, stroke_a};
    our_path.stroke.width = stroke_width;
    
    // Extract stroke cap style
    tvg::StrokeCap cap = shape->strokeCap();
    switch (cap) {
        case tvg::StrokeCap::Butt:
            our_path.stroke.cap = StrokeLineCap::Butt;
            break;
        case tvg::StrokeCap::Round:
            our_path.stroke.cap = StrokeLineCap::Round;
            break;
        case tvg::StrokeCap::Square:
            our_path.stroke.cap = StrokeLineCap::Square;
            break;
    }

    // Extract fill style
    uint8_t fill_r, fill_g, fill_b, fill_a;
    shape->fillColor(&fill_r, &fill_g, &fill_b, &fill_a);
    
    // Check if fill is enabled (non-zero alpha suggests it's set)
    our_path.fill.enabled = (fill_a > 0);
    our_path.fill.color = Color{fill_r, fill_g, fill_b, fill_a};

    // Extract path commands from ThorVG
    // ThorVG already converts SVG arcs, circles, ellipses to cubic Béziers
    uint32_t ptIdx = 0;
    for (uint32_t i = 0; i < cmdCnt; i++) {
        PathCommand cmd;

        switch (cmds[i]) {
            case tvg::PathCommand::MoveTo:
                if (ptIdx >= ptsCnt) continue;
                cmd.type = PathCommand::Type::MoveTo;
                cmd.x1 = pts[ptIdx].x;
                cmd.y1 = pts[ptIdx].y;
                ptIdx++;
                break;

            case tvg::PathCommand::LineTo:
                if (ptIdx >= ptsCnt) continue;
                cmd.type = PathCommand::Type::LineTo;
                cmd.x1 = pts[ptIdx].x;
                cmd.y1 = pts[ptIdx].y;
                ptIdx++;
                break;

            case tvg::PathCommand::CubicTo:
                if (ptIdx + 2 >= ptsCnt) continue;
                cmd.type = PathCommand::Type::CubicTo;
                cmd.x1 = pts[ptIdx].x;     // Control point 1
                cmd.y1 = pts[ptIdx].y;
                cmd.x2 = pts[ptIdx + 1].x; // Control point 2
                cmd.y2 = pts[ptIdx + 1].y;
                cmd.x3 = pts[ptIdx + 2].x; // End point
                cmd.y3 = pts[ptIdx + 2].y;
                ptIdx += 3;
                break;

            case tvg::PathCommand::Close:
                cmd.type = PathCommand::Type::Close;
                break;

            default:
                continue;
        }

        our_path.commands.push_back(cmd);
    }

    if (!our_path.commands.empty()) {
        doc.paths.push_back(our_path);
    }
}

void SvgLoader::_extract_nanosvg_shape(void* shape, GaugeDocument& doc) {
    // Deprecated - NanoSVG is no longer used
}

void SvgLoader::_extract_shapes(tvg::Paint* paint, GaugeDocument& doc) {
    if (!paint) return;
    
    // Check paint type and handle accordingly
    auto paintType = paint->type();
    
    if (paintType == tvg::Type::Shape) {
        // Cast to Shape and extract
        auto* shape = static_cast<tvg::Shape*>(paint);
        _extract_thorvg_shape(shape, doc);
    }
    else if (paintType == tvg::Type::Scene) {
        // Recursively traverse scene children
        auto* scene = static_cast<tvg::Scene*>(paint);
        auto& children = scene->paints();
        for (auto* child : children) {
            _extract_shapes(child, doc);
        }
    }
    else if (paintType == tvg::Type::Picture) {
        // Pictures contain an internal scene - we need to traverse it
        // Note: In ThorVG 0.15, Picture's internal scene is not directly accessible
        // The canvas push already flattened it for us
        std::cout << "Warning: Skipping Picture type (should be flattened)" << std::endl;
    }
    // Other types (Text, etc.) are not relevant for gauge preprocessing
}

void SvgLoader::_extract_path_from_shape(tvg::Shape* shape, GaugeDocument& doc) {
    _extract_thorvg_shape(shape, doc);
}

void SvgLoader::_extract_path_commands(tvg::Shape* shape, Path& path) {
    // Legacy method - functionality moved to _extract_thorvg_shape
}

} // namespace digidash
