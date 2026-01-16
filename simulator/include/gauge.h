/**
 * @file gauge.h
 * @brief Generic gauge widget that renders based on definition
 */

#ifndef GAUGE_H
#define GAUGE_H

#include "lvgl.h"
#include "config.h"
#include <cstdint>
#include <ctime>
#include <cstdio>

/**
 * @brief Generic gauge that renders based on GaugeDefinition
 * Supports arc, digital, and bar gauge types with zones, markers, and alerts
 */
class Gauge {
public:
    /**
     * @brief Create a gauge from a definition
     * @param parent LVGL parent object
     * @param definition Gauge definition with visual properties
     */
    Gauge(lv_obj_t* parent, const GaugeDefinition& definition);
    ~Gauge();
    
    /**
     * @brief Update the gauge with a new data value
     * @param value Current sensor value
     */
    void update(int value);
    
    /**
     * @brief Set speed and temperature values for arc gauge display
     * @param speed Speed value in km/h
     * @param temp Temperature value in °C
     */
    void setAuxiliaryValues(int speed, int temp) {
        speed_value_ = speed;
        temp_value_ = temp;
        if (speed_label_) {
            char buf[32];
            snprintf(buf, sizeof(buf), "Spd: %d", speed);
            lv_label_set_text(speed_label_, buf);
        }
        if (temp_label_) {
            char buf[32];
            snprintf(buf, sizeof(buf), "Tmp: %d°", temp);
            lv_label_set_text(temp_label_, buf);
        }
    }
    
    /**
     * @brief Get the container widget
     */
    lv_obj_t* getContainer() const { return container_; }
    
    /**
     * @brief Get the gauge definition
     */
    const GaugeDefinition& getDefinition() const { return definition_; }
    
    /**
     * @brief Get the data source name this gauge displays
     */
    const std::string& getDataSource() const { return definition_.data_source; }
    
private:
    lv_obj_t* container_;
    const GaugeDefinition& definition_;
    
    int current_value_;
    int speed_value_;
    int temp_value_;
    uint32_t last_flash_time_;
    bool flash_state_;
    
    // Type-specific UI elements
    lv_obj_t* arc_;
    lv_obj_t* bar_;
    lv_obj_t* value_label_;
    lv_obj_t* title_label_;
    lv_obj_t* unit_label_;
    lv_obj_t* speed_label_;
    lv_obj_t* temp_label_;
    
    /**
     * @brief Create arc gauge UI
     */
    void createArcGauge();
    
    /**
     * @brief Create digital (number) gauge UI
     */
    void createDigitalGauge();
    
    /**
     * @brief Create bar gauge UI
     */
    void createBarGauge();
    
    /**
     * @brief Update arc gauge display
     */
    void updateArcGauge(int value);
    
    /**
     * @brief Update digital gauge display
     */
    void updateDigitalGauge(int value);
    
    /**
     * @brief Update bar gauge display
     */
    void updateBarGauge(int value);
    
    /**
     * @brief Check and apply active alerts
     */
    void updateAlerts(int value);
    
    /**
     * @brief Get color for a value based on zones
     */
    std::string getColorForValue(int value);
    
    /**
     * @brief Parse hex color string to LVGL color
     */
    lv_color_t parseColor(const std::string& hex_color);
};

#endif // GAUGE_H
