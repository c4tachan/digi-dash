#include "svg_preprocessor.hpp"

namespace digidash {

void SvgNormalizer::normalize(GaugeDocument& doc) {
    // TODO:
    // - Resolve transforms (apply to coordinates)
    // - Convert circle/ellipse/rect/line into Path with PathCommands
    // - Normalize stroke/fill styles
    // For now, this is a no-op.
    (void)doc;
}

} // namespace digidash