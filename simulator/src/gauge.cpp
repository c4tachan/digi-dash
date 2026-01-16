/**
 * @file gauge.cpp
 * @brief Generic gauge implementation
 */

#include "gauge.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>

Gauge::Gauge(lv_obj_t* parent, const GaugeDefinition& definition)
    : container_(nullptr), definition_(definition), current_value_(0),
      speed_value_(0), temp_value_(0), last_flash_time_(0), flash_state_(false),
      arc_(nullptr), bar_(nullptr), value_label_(nullptr),
      title_label_(nullptr), unit_label_(nullptr), speed_label_(nullptr), temp_label_(nullptr) {
    
    // Create main container
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_PCT(100), LV_PCT(100));  // Will be sized by dashboard
    lv_obj_set_style_bg_color(container_, parseColor(definition_.visual.colors.count("background") ? 
                                                      definition_.visual.colors.at("background") : "#1a1a1a"), 0);
    lv_obj_set_style_border_width(container_, definition_.visual.border_width, 0);
    lv_obj_set_style_pad_all(container_, 10, 0);
    lv_obj_clear_flag(container_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(container_, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    
    // Set display for gauge creation
    lv_display_set_default(lv_obj_get_display(parent));
    
    // Create gauge based on type
    if (definition_.type == "arc") {
        createArcGauge();
    } else if (definition_.type == "digital") {
        createDigitalGauge();
    } else if (definition_.type == "bar") {
        createBarGauge();
    }
    
    printf("Created gauge: %s (type=%s, min=%d, max=%d)\n",
           definition_.name.c_str(), definition_.type.c_str(),
           definition_.min, definition_.max);
}

Gauge::~Gauge() {
    if (container_) {
        lv_obj_del(container_);
    }
}

void Gauge::createArcGauge() {
    // Set container background
    lv_obj_set_style_bg_color(container_, parseColor(definition_.visual.colors.count("background") ?
                                                      definition_.visual.colors.at("background") : "#1a1a1a"), 0);
    
    // Disable flex to use absolute positioning
    lv_obj_clear_flag(container_, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    
    // Use a reasonable fixed gauge size
    int gauge_size = 220;
    int center_x = 240;  // Center of 480px display
    int center_y = 140;  // Center of 320px display
    
    // Create arc FIRST (will be background layer)
    arc_ = lv_arc_create(container_);
    lv_obj_set_size(arc_, gauge_size, gauge_size);
    lv_obj_set_pos(arc_, center_x - gauge_size/2, center_y - gauge_size/2);
    lv_arc_set_rotation(arc_, 135);
    lv_arc_set_bg_angles(arc_, 0, 270);
    lv_arc_set_range(arc_, definition_.min, definition_.max);
    lv_obj_remove_style(arc_, nullptr, LV_PART_KNOB);
    lv_obj_set_style_arc_width(arc_, definition_.visual.arc_width, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_, definition_.visual.arc_width, LV_PART_INDICATOR);
    
    // Set colors
    lv_obj_set_style_arc_color(arc_, parseColor(definition_.visual.colors.count("arc_background") ?
                                                 definition_.visual.colors.at("arc_background") : "#404040"),
                               LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_, parseColor(definition_.visual.colors.count("arc_indicator") ?
                                                 definition_.visual.colors.at("arc_indicator") : "#FF6B00"),
                               LV_PART_INDICATOR);
    
    // Create arc tick marks AFTER arc so they render on top (like tachometer markings)
    // Draw as small colored rectangles instead of lines for better visibility
    // Add tick marks every 500 RPM with larger ticks every 1000 RPM
    int tick_interval = 500;  // Tick every 500 RPM
    int label_interval = 1000; // Label every 1000 RPM
    int range = definition_.max - definition_.min;
    int tick_count = 0;
    
    if (range > 0) {
        for (int tick_value = definition_.min + tick_interval; tick_value <= definition_.max; tick_value += tick_interval) {
            // Calculate angle for this tick (0-270 degrees)
            int angle = (tick_value - definition_.min) * 270 / range;
            
            // Convert angle to coordinates (135 degree rotation + angle offset)
            double rad = (135 + angle) * M_PI / 180.0;
            
            // Determine if this is a major tick (1000 RPM increment)
            bool is_major = (tick_value % label_interval) == 0;
            
            // Draw tick as a small rectangle at the arc edge
            int tick_size = is_major ? 8 : 4;  // Major ticks are larger squares
            int radius = 110;  // Position right at arc edge
            
            int tick_x = center_x + (int)(radius * cos(rad)) - tick_size/2;
            int tick_y = center_y + (int)(radius * sin(rad)) - tick_size/2;
            
            lv_obj_t* tick = lv_obj_create(container_);
            lv_obj_set_size(tick, tick_size, tick_size);
            lv_obj_set_pos(tick, tick_x, tick_y);
            
            // Use bright colors
            lv_color_t tick_color = is_major ? parseColor("#FF0000") : parseColor("#FFFF00");
            lv_obj_set_style_bg_color(tick, tick_color, 0);
            lv_obj_set_style_border_width(tick, 0, 0);
            lv_obj_clear_flag(tick, LV_OBJ_FLAG_SCROLLABLE);
            
            tick_count++;
            
            // Add labels for major ticks (1000 RPM increments) positioned beyond tick marks
            if (is_major) {
                char tick_label[16];
                snprintf(tick_label, sizeof(tick_label), "%dk", tick_value / 1000);
                
                lv_obj_t* tick_text = lv_label_create(container_);
                lv_obj_set_style_text_font(tick_text, &lv_font_montserrat_12, 0);
                lv_obj_set_style_text_color(tick_text, parseColor("#FFFF00"), 0);
                lv_label_set_text(tick_text, tick_label);
                
                // Position label even further out
                int label_radius = 155;
                int label_x = center_x + (int)(label_radius * cos(rad)) - 12;
                int label_y = center_y + (int)(label_radius * sin(rad)) - 10;
                lv_obj_set_pos(tick_text, label_x, label_y);
            }
        }
        printf("Created %d tick marks on arc\n", tick_count);
    }
    
    // Create speed value label - LARGE, centered in arc (displays speed value)
    value_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(value_label_, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(value_label_, parseColor(definition_.visual.colors.count("text") ?
                                                          definition_.visual.colors.at("text") : "#FFFFFF"), 0);
    lv_label_set_text(value_label_, "0");
    lv_obj_set_pos(value_label_, center_x - 35, center_y - 15);
    
    // Create small temperature arc - positioned above speed readout
    // This is a small progress arc showing temperature
    bar_ = lv_arc_create(container_);
    lv_obj_set_size(bar_, 80, 80);
    lv_obj_set_pos(bar_, center_x - 40, center_y - 90);
    lv_arc_set_rotation(bar_, 135);
    lv_arc_set_bg_angles(bar_, 0, 270);
    lv_arc_set_range(bar_, 0, 120);  // Temperature 0-120°C
    lv_obj_remove_style(bar_, nullptr, LV_PART_KNOB);
    lv_obj_set_style_arc_width(bar_, 4, LV_PART_MAIN);
    lv_obj_set_style_arc_width(bar_, 4, LV_PART_INDICATOR);
    
    // Set temperature arc colors
    lv_obj_set_style_arc_color(bar_, parseColor(definition_.visual.colors.count("cold") ?
                                                 definition_.visual.colors.at("cold") : "#0080FF"),
                               LV_PART_MAIN);
    lv_obj_set_style_arc_color(bar_, parseColor(definition_.visual.colors.count("hot") ?
                                                 definition_.visual.colors.at("hot") : "#FF0000"),
                               LV_PART_INDICATOR);
    
    // Create temp label - small, positioned in the temperature arc
    temp_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(temp_label_, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(temp_label_, parseColor(definition_.visual.colors.count("text") ?
                                                          definition_.visual.colors.at("text") : "#FFFFFF"), 0);
    lv_label_set_text(temp_label_, "0°");
    lv_obj_set_pos(temp_label_, center_x - 12, center_y - 75);
    
    // Create title label (RPM) - positioned at bottom in arc gap
    title_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title_label_, parseColor(definition_.visual.colors.count("text") ?
                                                          definition_.visual.colors.at("text") : "#FFFFFF"), 0);
    lv_label_set_text(title_label_, definition_.title.c_str());
    lv_obj_set_pos(title_label_, center_x - 20, center_y + 110);
    
    // Create visual zone boundary markers (like redline on tachometer)
    // Draw prominent lines at zone transition boundaries
    if (range > 0 && !definition_.zones.empty()) {
        // Create markers at zone boundaries
        for (size_t i = 0; i < definition_.zones.size() - 1; ++i) {
            // Get the boundary between this zone and the next
            int boundary_value = definition_.zones[i].max;
            
            // Calculate angle for this boundary (0-270 degrees)
            int angle = (boundary_value - definition_.min) * 270 / range;
            
            // Convert angle to coordinates (135 degree rotation + angle offset)
            double rad = (135 + angle) * M_PI / 180.0;
            int inner_x = center_x + (int)(85 * cos(rad));
            int inner_y = center_y + (int)(85 * sin(rad));
            int outer_x = center_x + (int)(125 * cos(rad));
            int outer_y = center_y + (int)(125 * sin(rad));
            
            lv_obj_t* zone_marker = lv_line_create(container_);
            lv_point_precise_t points[] = {{(lv_coord_t)inner_x, (lv_coord_t)inner_y}, 
                                           {(lv_coord_t)outer_x, (lv_coord_t)outer_y}};
            lv_line_set_points(zone_marker, points, 2);
            
            // Special styling for the danger zone boundary (redline)
            if (i == definition_.zones.size() - 2) {
                // This is the final boundary (between warning and danger)
                lv_obj_set_style_line_color(zone_marker, parseColor("#FF0000"), 0);
                lv_obj_set_style_line_width(zone_marker, 4, 0);
            } else {
                // Regular zone boundaries
                lv_obj_set_style_line_color(zone_marker, parseColor("#FFFF00"), 0);
                lv_obj_set_style_line_width(zone_marker, 3, 0);
            }
        }
    }
    
    // Unused speed_label_ pointer (was used for speed readout, now using value_label_ for that)
    speed_label_ = nullptr;
}

void Gauge::createDigitalGauge() {
    // Create background
    lv_obj_set_style_bg_color(container_, parseColor(definition_.visual.colors.count("background") ?
                                                      definition_.visual.colors.at("background") : "#1a1a1a"), 0);
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Create title label
    title_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title_label_, parseColor(definition_.visual.colors.count("text") ?
                                                          definition_.visual.colors.at("text") : "#FFFFFF"), 0);
    lv_label_set_text(title_label_, definition_.title.c_str());
    
    // Create value label
    value_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(value_label_, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(value_label_, parseColor(definition_.visual.colors.count("text") ?
                                                          definition_.visual.colors.at("text") : "#00FF00"), 0);
    lv_label_set_text(value_label_, "0");
    
    // Create unit label
    unit_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(unit_label_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(unit_label_, parseColor(definition_.visual.colors.count("text") ?
                                                         definition_.visual.colors.at("text") : "#FFFFFF"), 0);
    lv_label_set_text(unit_label_, definition_.unit.c_str());
}

void Gauge::createBarGauge() {
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(container_, 10, 0);
    
    // Create title
    title_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title_label_, parseColor(definition_.visual.colors.count("text") ?
                                                          definition_.visual.colors.at("text") : "#FFFFFF"), 0);
    lv_label_set_text(title_label_, definition_.title.c_str());
    
    // Create bar
    bar_ = lv_bar_create(container_);
    lv_obj_set_width(bar_, LV_PCT(90));  // 90% of container width
    lv_obj_set_height(bar_, definition_.visual.bar_height);
    lv_bar_set_range(bar_, definition_.min, definition_.max);
    lv_bar_set_value(bar_, definition_.min, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_, parseColor(definition_.visual.colors.count("bar_background") ?
                                               definition_.visual.colors.at("bar_background") : "#404040"),
                              LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_, parseColor(definition_.visual.colors.count("arc_indicator") ?
                                               definition_.visual.colors.at("arc_indicator") : "#FF6B00"),
                              LV_PART_INDICATOR);
    
    // Create value label
    value_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(value_label_, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(value_label_, parseColor(definition_.visual.colors.count("text") ?
                                                          definition_.visual.colors.at("text") : "#FFFFFF"), 0);
    lv_label_set_text(value_label_, "0");
    
    // Create unit label
    unit_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(unit_label_, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(unit_label_, parseColor(definition_.visual.colors.count("text") ?
                                                         definition_.visual.colors.at("text") : "#FFFFFF"), 0);
    lv_label_set_text(unit_label_, definition_.unit.c_str());
}

void Gauge::update(int value) {
    current_value_ = value;
    
    // Clamp to range
    if (value < definition_.min) value = definition_.min;
    if (value > definition_.max) value = definition_.max;
    
    // Update based on type
    if (definition_.type == "arc") {
        updateArcGauge(value);
    } else if (definition_.type == "digital") {
        updateDigitalGauge(value);
    } else if (definition_.type == "bar") {
        updateBarGauge(value);
    }
    
    // Check alerts
    updateAlerts(value);
}

void Gauge::updateArcGauge(int value) {
    if (arc_) {
        // Get color based on zones
        std::string color_str = getColorForValue(value);
        lv_obj_set_style_arc_color(arc_, parseColor(color_str), LV_PART_INDICATOR);
        
        lv_arc_set_value(arc_, value);
    }
    
    if (value_label_) {
        char buf[32];
        // Display speed value (3 digits max, zero-padded for cleaner look)
        snprintf(buf, sizeof(buf), "%03d", speed_value_);
        lv_label_set_text(value_label_, buf);
    }
    
    // Update temperature arc (bar_ is used for temperature)
    if (bar_) {
        lv_arc_set_value(bar_, temp_value_);
        
        // Color based on temperature
        lv_color_t temp_color;
        if (temp_value_ < 80) {
            temp_color = parseColor(definition_.visual.colors.count("cold") ?
                                   definition_.visual.colors.at("cold") : "#0080FF");
        } else if (temp_value_ < 100) {
            temp_color = parseColor(definition_.visual.colors.count("normal") ?
                                   definition_.visual.colors.at("normal") : "#00FF00");
        } else {
            temp_color = parseColor(definition_.visual.colors.count("hot") ?
                                   definition_.visual.colors.at("hot") : "#FF0000");
        }
        lv_obj_set_style_arc_color(bar_, temp_color, LV_PART_INDICATOR);
    }
    
    if (temp_label_) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d°", temp_value_);
        lv_label_set_text(temp_label_, buf);
    }
}

void Gauge::updateDigitalGauge(int value) {
    if (value_label_) {
        char buf[32];
        int decimal_places = definition_.visual.decimal_places;
        if (decimal_places > 0) {
            snprintf(buf, sizeof(buf), "%.*f", decimal_places, (float)value / (1 << decimal_places));
        } else {
            snprintf(buf, sizeof(buf), "%d", value);
        }
        lv_label_set_text(value_label_, buf);
    }
}

void Gauge::updateBarGauge(int value) {
    if (bar_) {
        // Get color based on zones
        std::string color_str = getColorForValue(value);
        lv_obj_set_style_bg_color(bar_, parseColor(color_str), LV_PART_INDICATOR);
        
        lv_bar_set_value(bar_, value, LV_ANIM_OFF);
    }
    
    if (value_label_) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", value);
        lv_label_set_text(value_label_, buf);
    }
}

void Gauge::updateAlerts(int value) {
    uint32_t now = lv_tick_get();
    
    for (const auto& alert : definition_.alerts) {
        bool condition_met = false;
        
        if (alert.condition == "above") {
            condition_met = (value > alert.threshold);
        } else if (alert.condition == "below") {
            condition_met = (value < alert.threshold);
        } else if (alert.condition == "above_or_equal") {
            condition_met = (value >= alert.threshold);
        } else if (alert.condition == "below_or_equal") {
            condition_met = (value <= alert.threshold);
        }
        
        if (condition_met && alert.action == "flash") {
            // Toggle flash state every flash_rate milliseconds
            if (now - last_flash_time_ >= (uint32_t)alert.flash_rate) {
                flash_state_ = !flash_state_;
                last_flash_time_ = now;
                
                // Apply flash effect
                if (container_) {
                    lv_color_t flash_color = parseColor(alert.flash_color);
                    if (flash_state_) {
                        lv_obj_set_style_bg_color(container_, flash_color, 0);
                    } else {
                        lv_obj_set_style_bg_color(container_, parseColor(definition_.visual.colors.count("background") ?
                                                                          definition_.visual.colors.at("background") : "#1a1a1a"), 0);
                    }
                }
            }
        }
    }
}

std::string Gauge::getColorForValue(int value) {
    // Find which zone this value falls into
    for (const auto& zone : definition_.zones) {
        if (value >= zone.min && value <= zone.max) {
            return zone.color;
        }
    }
    
    // Default to arc_indicator color if no zones match
    if (definition_.visual.colors.count("arc_indicator")) {
        return definition_.visual.colors.at("arc_indicator");
    }
    return "#FF6B00";
}

lv_color_t Gauge::parseColor(const std::string& hex_color) {
    // Parse hex color like "#FF6B00" to LVGL color
    if (hex_color.empty() || hex_color[0] != '#' || hex_color.length() < 7) {
        return lv_color_hex(0xFF6B00);  // Default orange
    }
    
    unsigned int color_val = 0;
    sscanf(hex_color.c_str(), "#%x", &color_val);
    return lv_color_hex(color_val);
}
