/**
 * @file gauge_factory.h
 * @brief Factory for creating gauges from definitions
 */

#ifndef GAUGE_FACTORY_H
#define GAUGE_FACTORY_H

#include "gauge.h"
#include "config.h"
#include "lvgl.h"

/**
 * @brief Factory for creating gauge instances from definitions
 * Handles legacy gauge types (rpm, speed, temp) and new definition-based gauges
 */
class GaugeFactory {
public:
    /**
     * @brief Create a gauge from a definition
     * @param parent LVGL parent object
     * @param gauge_def Gauge definition
     * @param data_map Optional map of data source names to current values
     * @return Pointer to created Gauge instance, or nullptr on error
     */
    static Gauge* createGauge(lv_obj_t* parent, const GaugeDefinition& gauge_def);
};

#endif // GAUGE_FACTORY_H
