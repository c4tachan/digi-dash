#pragma once

#include <cstdint>
#include <vector>

namespace digidash {

class StorageManager {
public:
    StorageManager();
    ~StorageManager();

    bool initialize();
    bool read_file(const char* path, std::vector<uint8_t>& data);
    bool file_exists(const char* path);
    
    size_t get_total_bytes() const { return total_bytes_; }
    size_t get_used_bytes() const { return used_bytes_; }
    
private:
    bool initialized_;
    size_t total_bytes_;
    size_t used_bytes_;
};

} // namespace digidash
