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
        
        // Parse protocol configuration
        if (j.contains("protocol") && j["protocol"].is_object()) {
            const auto& protocol_json = j["protocol"];
            config.protocol.type = protocol_json.value("type", "CAN");
            config.protocol.baudrate = protocol_json.value("baudrate", 500000);
            config.protocol.description = protocol_json.value("description", "");
            
            printf("Loaded protocol: %s @ %d baud\n", 
                   config.protocol.type.c_str(), config.protocol.baudrate);
            if (!config.protocol.description.empty()) {
                printf("  %s\n", config.protocol.description.c_str());
            }
        } else {
            // Default CAN configuration
            config.protocol.type = "CAN";
            config.protocol.baudrate = 500000;
            printf("No protocol specified, using default: CAN @ 500000 baud\n");
        }
        
        // Parse unit system configuration
        if (j.contains("units") && j["units"].is_object()) {
            const auto& units_json = j["units"];
            config.units.system = units_json.value("system", "metric");
            config.units.description = units_json.value("description", "");
            
            printf("Loaded units: %s\n", config.units.system.c_str());
            if (!config.units.description.empty()) {
                printf("  %s\n", config.units.description.c_str());
            }
        } else {
            // Default to metric
            config.units.system = "metric";
            printf("No units specified, using default: metric (km/h, °C)\n");
        }
        
        // Parse PID definitions
        if (j.contains("pids") && j["pids"].is_object()) {
            for (auto& [key, pid_json] : j["pids"].items()) {
                PidConfig pid_cfg;
                pid_cfg.command = pid_json.value("command", "");
                pid_cfg.formula = pid_json.value("formula", "A");
                pid_cfg.unit = pid_json.value("unit", "");
                config.pids[key] = pid_cfg;
            }
            printf("Loaded PID definitions: %zu types\n", config.pids.size());
        }
        
        // Parse gauge definitions
        if (j.contains("gauges") && j["gauges"].is_object()) {
            for (auto& [name, gauge_json] : j["gauges"].items()) {
                GaugeDefinition gauge_def;
                gauge_def.name = name;
                gauge_def.type = gauge_json.value("type", "arc");
                gauge_def.title = gauge_json.value("title", "");
                gauge_def.data_source = gauge_json.value("data_source", "");
                gauge_def.min = gauge_json.value("min", 0);
                gauge_def.max = gauge_json.value("max", 100);
                gauge_def.unit = gauge_json.value("unit", "");
                
                // Parse visual config
                if (gauge_json.contains("visual") && gauge_json["visual"].is_object()) {
                    const auto& visual_json = gauge_json["visual"];
                    
                    if (visual_json.contains("colors") && visual_json["colors"].is_object()) {
                        for (auto& [color_name, color_value] : visual_json["colors"].items()) {
                            gauge_def.visual.colors[color_name] = color_value.get<std::string>();
                        }
                    }
                    
                    gauge_def.visual.arc_width = visual_json.value("arc_width", 15);
                    gauge_def.visual.bar_height = visual_json.value("bar_height", 20);
                    gauge_def.visual.font_size = visual_json.value("font_size", 24);
                    gauge_def.visual.border_width = visual_json.value("border_width", 0);
                    gauge_def.visual.decimal_places = visual_json.value("decimal_places", 0);
                }
                
                // Parse zones
                if (gauge_json.contains("zones") && gauge_json["zones"].is_array()) {
                    for (const auto& zone_json : gauge_json["zones"]) {
                        Zone zone;
                        zone.min = zone_json.value("min", 0);
                        zone.max = zone_json.value("max", 100);
                        zone.color = zone_json.value("color", "#FFFFFF");
                        zone.label = zone_json.value("label", "");
                        gauge_def.zones.push_back(zone);
                    }
                }
                
                // Parse markers
                if (gauge_json.contains("markers") && gauge_json["markers"].is_array()) {
                    for (const auto& marker_json : gauge_json["markers"]) {
                        Marker marker;
                        marker.name = marker_json.value("name", "");
                        marker.value = marker_json.value("value", 0);
                        marker.style = marker_json.value("style", "line");
                        marker.color = marker_json.value("color", "#FFFFFF");
                        marker.width = marker_json.value("width", 2);
                        marker.label = marker_json.value("label", "");
                        gauge_def.markers.push_back(marker);
                    }
                }
                
                // Parse alerts
                if (gauge_json.contains("alerts") && gauge_json["alerts"].is_array()) {
                    for (const auto& alert_json : gauge_json["alerts"]) {
                        Alert alert;
                        alert.threshold = alert_json.value("threshold", 0);
                        alert.condition = alert_json.value("condition", "above");
                        alert.action = alert_json.value("action", "flash");
                        alert.flash_color = alert_json.value("flash_color", "#FF0000");
                        alert.flash_rate = alert_json.value("flash_rate", 500);
                        alert.label = alert_json.value("label", "");
                        gauge_def.alerts.push_back(alert);
                    }
                }
                
                config.gauges[name] = gauge_def;
            }
            printf("Loaded gauge definitions: %zu gauges\n", config.gauges.size());
            for (const auto& [name, gauge] : config.gauges) {
                printf("  %s: type=%s data_source=%s min=%d max=%d\n", 
                       name.c_str(), gauge.type.c_str(), gauge.data_source.c_str(), 
                       gauge.min, gauge.max);
                if (!gauge.zones.empty()) {
                    printf("    Zones: %zu\n", gauge.zones.size());
                    for (const auto& zone : gauge.zones) {
                        printf("      %d-%d: %s\n", zone.min, zone.max, zone.color.c_str());
                    }
                }
                if (!gauge.alerts.empty()) {
                    printf("    Alerts: %zu\n", gauge.alerts.size());
                    for (const auto& alert : gauge.alerts) {
                        printf("      %s %d: %s\n", alert.condition.c_str(), alert.threshold, alert.action.c_str());
                    }
                }
                if (!gauge.markers.empty()) {
                    printf("    Markers: %zu\n", gauge.markers.size());
                    for (const auto& marker : gauge.markers) {
                        printf("      %s at %d\n", marker.name.c_str(), marker.value);
                    }
                }
            }
        }
        
        // Parse display configuration
        if (j.contains("display") && j["display"].is_object()) {
            const auto& display_json = j["display"];
            
            config.display.width = display_json.value("width", 480);
            config.display.height = display_json.value("height", 320);
            
            if (display_json.contains("elements") && display_json["elements"].is_array()) {
                for (const auto& element_json : display_json["elements"]) {
                    DashboardElement element;
                    
                    element.gauge_name = element_json.value("gauge_name", "");
                    element.x = element_json.value("x", 0);
                    element.y = element_json.value("y", 0);
                    element.width = element_json.value("width", 320);
                    element.height = element_json.value("height", 480);
                    
                    config.display.elements.push_back(element);
                }
            }
        }
        
        printf("Loaded display configuration: %dx%d with %zu elements\n",
               config.display.width, config.display.height, config.display.elements.size());
        for (size_t i = 0; i < config.display.elements.size(); i++) {
            printf("  Element %zu: %s at (%d, %d) size %dx%d\n",
                   i, config.display.elements[i].gauge_name.c_str(),
                   config.display.elements[i].x, config.display.elements[i].y,
                   config.display.elements[i].width, config.display.elements[i].height);
        }
        
        
    } catch (const json::exception& e) {
        fprintf(stderr, "JSON parse error: %s\n", e.what());
    }
    
    return config;
}
