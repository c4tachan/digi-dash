#include "svg_preprocessor.hpp"

namespace digidash {

void SvgNormalizer::normalize(GaugeDocument& doc) {
    // ThorVG already normalizes SVG elements during parsing:
    // - Circles, ellipses, rects are converted to path commands
    // - Arcs are converted to cubic Bézier curves
    // - Transforms are applied to coordinates
    // - Styles are resolved
    // 
    // This step is essentially a no-op for ThorVG-loaded data
    (void)doc;
}

} // namespace digidash