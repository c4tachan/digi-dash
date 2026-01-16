/**
 * @file config.h
 * @brief Configuration loader for dashboard windows, gauges, and OBD II PIDs
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// OBD II Protocol configuration
struct ProtocolConfig {
    std::string type;         // "CAN", "KWP2000", "ISO9141", etc.
    int baudrate;             // e.g. 500000 for CAN, 10400 for KWP2000
    std::string description;  // Human-readable description
};

// Unit system configuration
struct UnitConfig {
    std::string system;       // "metric" or "sae"
    std::string description;  // Human-readable description
};

struct PidConfig {
    std::string command;      // OBD II command, e.g. "010C"
    std::string formula;      // Decoding formula, e.g. "((A*256)+B)/4" for RPM
    std::string unit;         // Display unit, e.g. "RPM"
};

// Zone colors based on value ranges (for arcs and bars)
struct Zone {
    int min;
    int max;
    std::string color;        // Hex color, e.g. "#FF0000"
    std::string label;        // Optional label
};

// Visual markers on gauges (redlines, ticks, etc.)
struct Marker {
    std::string name;
    int value;
    std::string style;        // "line", "tick", "dot", "label"
    std::string color;
    int width;
    std::string label;
};

// Alert thresholds that trigger visual/audio feedback
struct Alert {
    int threshold;
    std::string condition;    // "above", "below", "above_or_equal", "below_or_equal"
    std::string action;       // "flash", "flash_intense", "shake", "color_pulse"
    std::string flash_color;  // Color while flashing
    int flash_rate;           // Milliseconds between flashes
    std::string label;        // Alert description
};

// Visual styling properties
struct VisualConfig {
    std::map<std::string, std::string> colors;  // color name -> hex value
    int arc_width;
    int bar_height;
    int font_size;
    int border_width;
    int decimal_places;
};

// Complete gauge definition (can be used to create any gauge type)
struct GaugeDefinition {
    std::string name;         // Unique identifier
    std::string type;         // "arc", "digital", "bar", "needle"
    std::string title;        // Display title
    std::string data_source;  // PID name, e.g. "rpm"
    int min;
    int max;
    std::string unit;
    
    VisualConfig visual;
    std::vector<Zone> zones;
    std::vector<Marker> markers;
    std::vector<Alert> alerts;
};

// Gauge instance in a display (references a definition)
struct DashboardElement {
    std::string gauge_name;   // References a GaugeDefinition by name
    int x;
    int y;
    int width;
    int height;
};

// Single display configuration
struct DisplayConfig {
    int width;
    int height;
    std::vector<DashboardElement> elements;
};

struct DashboardConfig {
    ProtocolConfig protocol;                            // OBD II protocol configuration
    UnitConfig units;                                   // Unit system configuration
    std::map<std::string, PidConfig> pids;              // PID definitions
    std::map<std::string, GaugeDefinition> gauges;      // Gauge definitions
    DisplayConfig display;
    
    /**
     * @brief Load configuration from JSON file
     * @param filename Path to JSON configuration file
     * @return Configuration object
     */
    static DashboardConfig loadFromFile(const std::string& filename);
    
    /**
     * @brief Load configuration from JSON string
     * @param json_str JSON configuration as string
     * @return Configuration object
     */
    static DashboardConfig loadFromString(const std::string& json_str);
};

#endif // CONFIG_H
