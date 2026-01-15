/**
 * @file window_manager.cpp
 * @brief Implementation of window manager
 */

#include "window_manager.h"
#include <cstdio>
#include <algorithm>

WindowManager::WindowManager() : next_window_id_(0) {
}

WindowManager::~WindowManager() {
    windows_.clear();
}

bool WindowManager::initializeFromConfig(const DashboardConfig& config) {
    if (config.windows.empty()) {
        fprintf(stderr, "Configuration has no windows\n");
        return false;
    }
    
    if (config.windows.size() > MAX_WINDOWS) {
        fprintf(stderr, "Configuration has %zu windows, maximum is %zu\n", config.windows.size(), MAX_WINDOWS);
        return false;
    }
    
    bool all_success = true;
    
    for (const auto& win_cfg : config.windows) {
        // Determine gauge type from config
        GaugeWindow::GaugeType gauge_type = GaugeWindow::GAUGE_RPM;
        
        if (!win_cfg.gauges.empty()) {
            const auto& gauge = win_cfg.gauges[0];
            if (gauge.type == "speed") {
                gauge_type = GaugeWindow::GAUGE_SPEED;
            } else if (gauge.type == "temp") {
                gauge_type = GaugeWindow::GAUGE_TEMP;
            }
        }
        
        try {
            auto window = std::make_unique<GaugeWindow>(gauge_type, win_cfg.x, win_cfg.y, 
                                                        win_cfg.width, win_cfg.height, 
                                                        next_window_id_);
            
            if (!window->isValid()) {
                fprintf(stderr, "Failed to create valid gauge window from config\n");
                all_success = false;
                continue;
            }
            
            windows_.push_back(std::move(window));
            next_window_id_++;
        } catch (const std::exception& e) {
            fprintf(stderr, "Exception creating window from config: %s\n", e.what());
            all_success = false;
        }
    }
    
    printf("Initialized %zu windows from configuration\n", windows_.size());
    return all_success;
}

bool WindowManager::addGaugeWindow(GaugeWindow::GaugeType gauge_type) {
    if (windows_.size() >= MAX_WINDOWS) {
        fprintf(stderr, "Maximum number of windows (%zu) reached\n", MAX_WINDOWS);
        return false;
    }
    
    int x = 100 + (next_window_id_ * 350);
    int y = 100;
    int width = 320;
    int height = 480;
    
    try {
        auto window = std::make_unique<GaugeWindow>(gauge_type, x, y, width, height, next_window_id_);
        
        if (!window->isValid()) {
            fprintf(stderr, "Failed to create valid gauge window\n");
            return false;
        }
        
        windows_.push_back(std::move(window));
        next_window_id_++;
        
        printf("Added window #%d. Total windows: %zu/%zu\n", next_window_id_ - 1, windows_.size(), MAX_WINDOWS);
        return true;
    } catch (const std::exception& e) {
        fprintf(stderr, "Exception creating gauge window: %s\n", e.what());
        return false;
    }
}

void WindowManager::removeWindow(uint32_t window_id) {
    auto it = std::find_if(windows_.begin(), windows_.end(),
        [window_id](const std::unique_ptr<GaugeWindow>& w) {
            return w->getWindowID() == window_id;
        });
    
    if (it != windows_.end()) {
        windows_.erase(it);
        printf("Removed window. Total windows: %zu\n", windows_.size());
    }
}

void WindowManager::update(const MockData& data) {
    for (auto& window : windows_) {
        if (window && window->isValid()) {
            window->update(data);
        }
    }
}

void WindowManager::handleEvent(const SDL_Event& event) {
    // Try to route the event to the appropriate window
    for (auto& window : windows_) {
        if (window && window->isValid()) {
            if (window->handleEvent(event)) {
                // Event was handled by this window
                break;
            }
        }
    }
}

void WindowManager::renderAll() {
    // Remove invalid windows
    windows_.erase(
        std::remove_if(windows_.begin(), windows_.end(),
            [](const std::unique_ptr<GaugeWindow>& w) {
                return !w || !w->isValid();
            }),
        windows_.end()
    );
    
    // Render all valid windows
    for (auto& window : windows_) {
        if (window && window->isValid()) {
            window->render();
        }
    }
}

GaugeWindow* WindowManager::findWindow(uint32_t window_id) {
    auto it = std::find_if(windows_.begin(), windows_.end(),
        [window_id](const std::unique_ptr<GaugeWindow>& w) {
            return w && w->getWindowID() == window_id;
        });
    
    if (it != windows_.end()) {
        return it->get();
    }
    return nullptr;
}
