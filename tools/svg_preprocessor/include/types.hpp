#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace digidash {

struct Color {
    uint8_t r{0}, g{0}, b{0}, a{255};
};

enum class StrokeLineCap : uint8_t {
    Butt = 0,
    Round = 1,
    Square = 2
};

struct StrokeStyle {
    float width{1.0f};
    Color color{255, 255, 255, 255};
    StrokeLineCap cap{StrokeLineCap::Butt};
};

struct FillStyle {
    bool enabled{false};
    Color color{0, 0, 0, 255};
};

struct PathCommand {
    enum class Type : uint8_t { MoveTo = 0, LineTo = 1, CubicTo = 2, Close = 3 };

    Type type{Type::MoveTo};
    float x1{0.0f}, y1{0.0f};
    float x2{0.0f}, y2{0.0f};
    float x3{0.0f}, y3{0.0f};
};

struct Path {
    std::string id;
    std::vector<PathCommand> commands;
    StrokeStyle stroke;
    FillStyle fill;
};

struct GaugeDocument {
    float width{0.0f};
    float height{0.0f};
    std::vector<Path> paths;
};

} // namespace digidash