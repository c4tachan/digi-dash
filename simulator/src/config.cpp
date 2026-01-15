/**
 * @file config.cpp
 * @brief Configuration loader implementation
 */

#include "config.h"
#include <fstream>
#include <cstdio>

DashboardConfig DashboardConfig::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        fprintf(stderr, "Failed to open config file: %s\n", filename.c_str());
        return DashboardConfig();
    }
    
    try {
        json j;
        file >> j;
        return loadFromString(j.dump());
    } catch (const json::exception& e) {
        fprintf(stderr, "JSON parse error: %s\n", e.what());
        return DashboardConfig();
    }
}

DashboardConfig DashboardConfig::loadFromString(const std::string& json_str) {
    DashboardConfig config;
    
    try {
        json j = json::parse(json_str);
        
        if (j.contains("windows") && j["windows"].is_array()) {
            for (const auto& win_json : j["windows"]) {
                WindowConfig win_cfg;
                
                win_cfg.id = win_json.value("id", 0);
                win_cfg.x = win_json.value("x", 0);
                win_cfg.y = win_json.value("y", 0);
                win_cfg.width = win_json.value("width", 320);
                win_cfg.height = win_json.value("height", 480);
                
                if (win_json.contains("gauges") && win_json["gauges"].is_array()) {
                    for (const auto& gauge_json : win_json["gauges"]) {
                        GaugeConfig gauge_cfg;
                        
                        gauge_cfg.type = gauge_json.value("type", "rpm");
                        gauge_cfg.x = gauge_json.value("x", 0);
                        gauge_cfg.y = gauge_json.value("y", 0);
                        gauge_cfg.width = gauge_json.value("width", 320);
                        gauge_cfg.height = gauge_json.value("height", 480);
                        
                        win_cfg.gauges.push_back(gauge_cfg);
                    }
                }
                
                config.windows.push_back(win_cfg);
            }
        }
        
        printf("Loaded configuration with %zu windows\n", config.windows.size());
        for (size_t i = 0; i < config.windows.size(); i++) {
            printf("  Window %zu: %dx%d at (%d, %d) with %zu gauges\n",
                   i, config.windows[i].width, config.windows[i].height,
                   config.windows[i].x, config.windows[i].y,
                   config.windows[i].gauges.size());
        }
        
    } catch (const json::exception& e) {
        fprintf(stderr, "JSON parse error: %s\n", e.what());
    }
    
    return config;
}
