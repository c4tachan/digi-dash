#include "../include/svg_preprocessor.hpp"

namespace digidash {

void PathFlattener::flatten(GaugeDocument& doc) {
    // TODO:
    // - Find any arc commands (if you model them)
    // - Convert them into cubic Bézier PathCommands
    // - Optionally subdivide curves into smaller segments
    (void)doc;
}

} // namespace digidash