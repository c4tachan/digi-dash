#include "storage_manager.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include <cstdio>

static const char* TAG = "StorageManager";

namespace digidash {

StorageManager::StorageManager()
    : initialized_(false)
    , total_bytes_(0)
    , used_bytes_(0) {
}

StorageManager::~StorageManager() {
    if (initialized_) {
        esp_vfs_spiffs_unregister("spiffs");
    }
}

bool StorageManager::initialize() {
    if (initialized_) {
        ESP_LOGW(TAG, "Storage already initialized");
        return true;
    }

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "spiffs",
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return false;
    }

    ret = esp_spiffs_info(conf.partition_label, &total_bytes_, &used_bytes_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS info (%s)", esp_err_to_name(ret));
        return false;
    }
    
    ESP_LOGI(TAG, "SPIFFS initialized - total: %zu bytes, used: %zu bytes", 
             total_bytes_, used_bytes_);
    
    initialized_ = true;
    return true;
}

bool StorageManager::read_file(const char* path, std::vector<uint8_t>& data) {
    if (!initialized_) {
        ESP_LOGE(TAG, "Storage not initialized");
        return false;
    }

    FILE* file = fopen(path, "rb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file: %s", path);
        return false;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Read file
    data.resize(file_size);
    size_t bytes_read = fread(data.data(), 1, file_size, file);
    fclose(file);

    if (bytes_read != file_size) {
        ESP_LOGE(TAG, "Failed to read complete file: %s (expected %zu, got %zu)", 
                 path, file_size, bytes_read);
        return false;
    }

    ESP_LOGI(TAG, "Read file: %s (%zu bytes)", path, file_size);
    return true;
}

bool StorageManager::file_exists(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

} // namespace digidash
