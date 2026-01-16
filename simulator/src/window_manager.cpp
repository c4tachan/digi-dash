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
        try {
            auto window = std::make_unique<GaugeWindow>(win_cfg, config.gauges, next_window_id_);
            
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

void WindowManager::update(const DataSource& data, const std::map<std::string, PidConfig>& pid_map) {
    for (auto& window : windows_) {
        if (window && window->isValid()) {
            window->update(data, pid_map);
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
