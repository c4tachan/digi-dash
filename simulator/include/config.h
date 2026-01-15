/**
 * @file config.h
 * @brief Configuration loader for dashboard windows
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct GaugeConfig {
    std::string type;  // "rpm", "speed", "temp"
    int x;
    int y;
    int width;
    int height;
};

struct WindowConfig {
    int id;
    int x;
    int y;
    int width;
    int height;
    std::vector<GaugeConfig> gauges;
};

struct DashboardConfig {
    std::vector<WindowConfig> windows;
    
    /**
     * @brief Load configuration from JSON file
     * @param filename Path to JSON configuration file
     * @return true if loaded successfully
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
