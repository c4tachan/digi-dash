/**
 * @file gauge_factory.cpp
 * @brief Gauge factory implementation
 */

#include "gauge_factory.h"
#include <cstdio>

Gauge* GaugeFactory::createGauge(lv_obj_t* parent, const GaugeDefinition& gauge_def) {
    if (!parent) {
        fprintf(stderr, "GaugeFactory: Parent object is null\n");
        return nullptr;
    }
    
    if (gauge_def.type.empty() || gauge_def.name.empty()) {
        fprintf(stderr, "GaugeFactory: Invalid gauge definition (missing type or name)\n");
        return nullptr;
    }
    
    try {
        Gauge* gauge = new Gauge(parent, gauge_def);
        return gauge;
    } catch (const std::exception& e) {
        fprintf(stderr, "GaugeFactory: Failed to create gauge: %s\n", e.what());
        return nullptr;
    }
}
