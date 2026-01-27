#include "digidash/binary_gauge_loader.h"
#include <cstring>
#include <fstream>

namespace digidash {

BinaryGaugeLoader::BinaryGaugeLoader() {}

BinaryGaugeLoader::~BinaryGaugeLoader() {}

bool BinaryGaugeLoader::load_from_file(const std::string& filepath,
                                       GaugeAsset& asset_out) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }
    
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(file_size);
    file.read(reinterpret_cast<char*>(buffer.data()), file_size);
    file.close();
    
    return load_from_buffer(buffer.data(), buffer.size(), asset_out);
}

bool BinaryGaugeLoader::load_from_buffer(const uint8_t* buffer,
                                        size_t buffer_size,
                                        GaugeAsset& asset_out) {
    if (!buffer || buffer_size < 128) {
        return false;
    }
    
    uint32_t width, height;
    if (!parse_header(buffer, buffer_size, width, height)) {
        return false;
    }
    
    asset_out.width = width;
    asset_out.height = height;
    
    // Parse path data
    size_t offset = 128;
    while (offset < buffer_size) {
        if (offset + 12 > buffer_size) break;  // Not enough for path header
        
        uint16_t point_count;
        std::memcpy(&point_count, buffer + offset, sizeof(uint16_t));
        offset += sizeof(uint16_t);
        
        uint32_t color;
        std::memcpy(&color, buffer + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        
        float stroke_width;
        std::memcpy(&stroke_width, buffer + offset, sizeof(float));
        offset += sizeof(float);
        
        uint8_t is_filled;
        std::memcpy(&is_filled, buffer + offset, sizeof(uint8_t));
        offset += sizeof(uint8_t);
        
        offset += 3;  // Skip padding
        
        // Check if we have enough data for all points
        if (offset + (point_count * sizeof(float) * 2) > buffer_size) {
            break;
        }
        
        // Store path data
        asset_out.path_data.push_back(point_count);
        asset_out.path_data.push_back((color >> 0) & 0xFF);
        asset_out.path_data.push_back((color >> 8) & 0xFF);
        asset_out.path_data.push_back((color >> 16) & 0xFF);
        asset_out.path_data.push_back((color >> 24) & 0xFF);
        
        uint32_t stroke_width_bits;
        std::memcpy(&stroke_width_bits, &stroke_width, sizeof(float));
        asset_out.path_data.push_back((stroke_width_bits >> 0) & 0xFF);
        asset_out.path_data.push_back((stroke_width_bits >> 8) & 0xFF);
        asset_out.path_data.push_back((stroke_width_bits >> 16) & 0xFF);
        asset_out.path_data.push_back((stroke_width_bits >> 24) & 0xFF);
        
        asset_out.path_data.push_back(is_filled);
        
        // Store point data
        for (int i = 0; i < point_count; ++i) {
            float x, y;
            std::memcpy(&x, buffer + offset, sizeof(float));
            offset += sizeof(float);
            std::memcpy(&y, buffer + offset, sizeof(float));
            offset += sizeof(float);
            
            uint32_t x_bits, y_bits;
            std::memcpy(&x_bits, &x, sizeof(float));
            std::memcpy(&y_bits, &y, sizeof(float));
            
            asset_out.path_data.push_back((x_bits >> 0) & 0xFF);
            asset_out.path_data.push_back((x_bits >> 8) & 0xFF);
            asset_out.path_data.push_back((x_bits >> 16) & 0xFF);
            asset_out.path_data.push_back((x_bits >> 24) & 0xFF);
            asset_out.path_data.push_back((y_bits >> 0) & 0xFF);
            asset_out.path_data.push_back((y_bits >> 8) & 0xFF);
            asset_out.path_data.push_back((y_bits >> 16) & 0xFF);
            asset_out.path_data.push_back((y_bits >> 24) & 0xFF);
        }
    }
    
    return validate_asset(asset_out);
}

bool BinaryGaugeLoader::validate_asset(const GaugeAsset& asset) {
    return asset.width > 0 && asset.height > 0 && !asset.path_data.empty();
}

bool BinaryGaugeLoader::parse_header(const uint8_t* buffer, size_t buffer_size,
                                     uint32_t& width_out,
                                     uint32_t& height_out) {
    if (buffer_size < 128) {
        return false;
    }
    
    uint32_t magic;
    std::memcpy(&magic, buffer, sizeof(uint32_t));
    
    if (magic != 0x44474147) {  // "DGAG"
        return false;
    }
    
    uint32_t version;
    std::memcpy(&version, buffer + 4, sizeof(uint32_t));
    if (version != 1) {
        return false;
    }
    
    std::memcpy(&width_out, buffer + 8, sizeof(uint32_t));
    std::memcpy(&height_out, buffer + 12, sizeof(uint32_t));
    
    return true;
}

} // namespace digidash
