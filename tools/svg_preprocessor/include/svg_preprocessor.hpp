#pragma once
#include <string>
#include <vector>

#include "types.hpp"

// Forward declare ThorVG types
namespace tvg {
class Paint;
class Shape;
}

namespace digidash {

class SvgLoader {
public:
    GaugeDocument load_from_file(const std::string& path);
    GaugeDocument load_from_string(const std::string& svg);

private:
    // ThorVG shape extraction helpers
    void _extract_thorvg_shape(tvg::Shape* shape, GaugeDocument& doc);
    
    // Deprecated NanoSVG helpers (kept for reference only)
    void _extract_nanosvg_shape(void* shape, GaugeDocument& doc);
    void _extract_shapes(tvg::Paint* paint, GaugeDocument& doc);
    void _extract_path_from_shape(tvg::Shape* shape, GaugeDocument& doc);
    void _extract_path_commands(tvg::Shape* shape, Path& path);
};

class SvgNormalizer {
public:
    void normalize(GaugeDocument& doc);
    // resolve transforms, styles, convert circle/ellipse/rect → paths
};

class PathFlattener {
public:
    void flatten(GaugeDocument& doc);
    // convert arcs → cubic Béziers, optional line subdivision
};

class GaugeSerializer {
public:
    void write_binary(const GaugeDocument& doc, const std::string& out_path);
};

} // namespace digidash