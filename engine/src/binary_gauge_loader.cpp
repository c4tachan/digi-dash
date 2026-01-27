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
    if (!buffer || buffer_size < 8) {
        return false;
    }
    
    size_t offset = 0;
    
    // Parse header: magic + version + path_count
    uint32_t magic;
    std::memcpy(&magic, buffer + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    
    if (magic != 0x45474744) {  // "DGGE"
        return false;
    }
    
    uint16_t version;
    std::memcpy(&version, buffer + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    
    if (version != 1) {
        return false;
    }
    
    uint16_t path_count;
    std::memcpy(&path_count, buffer + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    
    // Set default dimensions (will be overridden if format includes them)
    asset_out.width = 800;
    asset_out.height = 600;
    
    // Parse each path
    for (uint16_t i = 0; i < path_count; ++i) {
        if (offset >= buffer_size) break;
        
        Path path;
        
        // Read path ID
        uint8_t id_len;
        std::memcpy(&id_len, buffer + offset, sizeof(uint8_t));
        offset += sizeof(uint8_t);
        
        if (offset + id_len > buffer_size) break;
        path.id.assign(reinterpret_cast<const char*>(buffer + offset), id_len);
        offset += id_len;
        
        // Read stroke style
        if (offset + 9 > buffer_size) break;
        std::memcpy(&path.stroke.width, buffer + offset, sizeof(float));
        offset += sizeof(float);
        
        path.stroke.color.r = buffer[offset++];
        path.stroke.color.g = buffer[offset++];
        path.stroke.color.b = buffer[offset++];
        path.stroke.color.a = buffer[offset++];
        path.stroke.cap = static_cast<StrokeLineCap>(buffer[offset++]);
        
        // Read fill style
        if (offset + 5 > buffer_size) break;
        path.fill.enabled = buffer[offset++] != 0;
        path.fill.color.r = buffer[offset++];
        path.fill.color.g = buffer[offset++];
        path.fill.color.b = buffer[offset++];
        path.fill.color.a = buffer[offset++];
        
        // Read command count
        if (offset + 2 > buffer_size) break;
        uint16_t cmd_count;
        std::memcpy(&cmd_count, buffer + offset, sizeof(uint16_t));
        offset += sizeof(uint16_t);
        
        // Read commands
        for (uint16_t c = 0; c < cmd_count; ++c) {
            if (offset + 25 > buffer_size) break;  // 1 + 6*4 bytes
            
            PathCommand cmd;
            cmd.type = static_cast<PathCommand::Type>(buffer[offset++]);
            
            std::memcpy(&cmd.x1, buffer + offset, sizeof(float)); offset += sizeof(float);
            std::memcpy(&cmd.y1, buffer + offset, sizeof(float)); offset += sizeof(float);
            std::memcpy(&cmd.x2, buffer + offset, sizeof(float)); offset += sizeof(float);
            std::memcpy(&cmd.y2, buffer + offset, sizeof(float)); offset += sizeof(float);
            std::memcpy(&cmd.x3, buffer + offset, sizeof(float)); offset += sizeof(float);
            std::memcpy(&cmd.y3, buffer + offset, sizeof(float)); offset += sizeof(float);
            
            path.commands.push_back(cmd);
        }
        
        asset_out.paths.push_back(path);
    }
    
    return validate_asset(asset_out);
}

bool BinaryGaugeLoader::validate_asset(const GaugeAsset& asset) {
    return asset.width > 0 && asset.height > 0 && !asset.paths.empty();
}

} // namespace digidash
