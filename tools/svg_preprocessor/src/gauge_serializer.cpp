#include "svg_preprocessor.hpp"
#include <fstream>
#include <stdexcept>

namespace digidash {

namespace {
    void write_u8(std::ofstream& os, uint8_t v) { os.write(reinterpret_cast<const char*>(&v), 1); }
    void write_u16(std::ofstream& os, uint16_t v) { os.write(reinterpret_cast<const char*>(&v), sizeof(v)); }
    void write_f32(std::ofstream& os, float v) { os.write(reinterpret_cast<const char*>(&v), sizeof(v)); }
}

void GaugeSerializer::write_binary(const GaugeDocument& doc, const std::string& out_path) {
    std::ofstream os(out_path, std::ios::binary);
    if (!os) {
        throw std::runtime_error("Failed to open output file: " + out_path);
    }

    // Header: magic + version + path_count
    const char magic[4] = {'D', 'G', 'G', 'E'};
    os.write(magic, 4);
    uint16_t version = 1;
    write_u16(os, version);
    write_u16(os, static_cast<uint16_t>(doc.paths.size()));

    for (const auto& path : doc.paths) {
        uint8_t id_len = static_cast<uint8_t>(path.id.size());
        write_u8(os, id_len);
        os.write(path.id.data(), id_len);

        write_f32(os, path.stroke.width);
        write_u8(os, path.stroke.color.r);
        write_u8(os, path.stroke.color.g);
        write_u8(os, path.stroke.color.b);
        write_u8(os, path.stroke.color.a);
        write_u8(os, static_cast<uint8_t>(path.stroke.cap));

        write_u8(os, path.fill.enabled ? 1 : 0);
        write_u8(os, path.fill.color.r);
        write_u8(os, path.fill.color.g);
        write_u8(os, path.fill.color.b);
        write_u8(os, path.fill.color.a);

        write_u16(os, static_cast<uint16_t>(path.commands.size()));
        for (const auto& cmd : path.commands) {
            write_u8(os, static_cast<uint8_t>(cmd.type));
            write_f32(os, cmd.x1);
            write_f32(os, cmd.y1);
            write_f32(os, cmd.x2);
            write_f32(os, cmd.y2);
            write_f32(os, cmd.x3);
            write_f32(os, cmd.y3);
        }
    }
}

} // namespace digidash