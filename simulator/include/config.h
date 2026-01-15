/**
 * @file config.h
 * @brief Configuration loader for dashboard windows and OBD II PIDs
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct PidConfig {
    std::string command;      // OBD II command, e.g. "010C"
    std::string formula;      // Decoding formula, e.g. "((A*256)+B)/4" for RPM
    std::string unit;         // Display unit, e.g. "RPM"
};

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
    std::map<std::string, PidConfig> pids;  // PID definitions keyed by type ("rpm", "speed", etc)
    std::vector<WindowConfig> windows;
    
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
