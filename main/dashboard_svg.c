#include <stdint.h>
#include <stddef.h>

// dashboard.svg embedded as C array
const char dashboard_svg_data[] =
#include "dashboard_svg_data.inc"
;

const size_t dashboard_svg_size = sizeof(dashboard_svg_data) - 1; // -1 for null terminator
