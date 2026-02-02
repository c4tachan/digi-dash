#include <catch2/catch_test_macros.hpp>

#include <digidash/binary_gauge_loader.h>

#include <vector>
#include <cstring>

using namespace digidash;

static void append_u32(std::vector<uint8_t>& buf, uint32_t v) {
    uint8_t tmp[4]; std::memcpy(tmp, &v, 4); buf.insert(buf.end(), tmp, tmp+4);
}
static void append_u16(std::vector<uint8_t>& buf, uint16_t v) {
    uint8_t tmp[2]; std::memcpy(tmp, &v, 2); buf.insert(buf.end(), tmp, tmp+2);
}
static void append_u8(std::vector<uint8_t>& buf, uint8_t v) {
    buf.push_back(v);
}
static void append_f32(std::vector<uint8_t>& buf, float v) {
    uint8_t tmp[4]; std::memcpy(tmp, &v, 4); buf.insert(buf.end(), tmp, tmp+4);
}

TEST_CASE("BinaryGaugeLoader parses minimal valid buffer") {
    std::vector<uint8_t> buf;
    // magic 'DGGE' little-endian 0x45474744
    append_u32(buf, 0x45474744);
    append_u16(buf, 1); // version
    append_u16(buf, 1); // path_count
    append_u16(buf, 128); // width
    append_u16(buf, 64);  // height

    // path[0]
    const char* id = "p0";
    append_u8(buf, (uint8_t)strlen(id));
    buf.insert(buf.end(), id, id + strlen(id));

    // stroke.width (float)
    append_f32(buf, 1.0f);
    // stroke rgba
    append_u8(buf, 10); append_u8(buf,20); append_u8(buf,30); append_u8(buf,255);
    // cap
    append_u8(buf, 0);

    // fill enabled + rgba
    append_u8(buf, 1);
    append_u8(buf, 1); append_u8(buf,2); append_u8(buf,3); append_u8(buf,255);

    // cmd_count = 0
    append_u16(buf, 0);

    BinaryGaugeLoader loader;
    BinaryGaugeLoader::GaugeAsset asset;
    bool ok = loader.load_from_buffer(buf.data(), buf.size(), asset);
    REQUIRE(ok == true);
    REQUIRE(asset.width == 128);
    REQUIRE(asset.height == 64);
    REQUIRE(asset.paths.size() == 1);
    REQUIRE(asset.paths[0].id == "p0");
}

TEST_CASE("BinaryGaugeLoader rejects wrong magic or version") {
    std::vector<uint8_t> buf;
    append_u32(buf, 0x00000000);
    append_u16(buf, 1);
    append_u16(buf, 0);
    append_u16(buf, 1); append_u16(buf,1);

    BinaryGaugeLoader loader;
    BinaryGaugeLoader::GaugeAsset a;
    REQUIRE(loader.load_from_buffer(buf.data(), buf.size(), a) == false);

    // now correct magic but bad version
    buf.clear();
    append_u32(buf, 0x45474744);
    append_u16(buf, 99); // bad version
    append_u16(buf, 0);
    append_u16(buf, 1); append_u16(buf,1);
    REQUIRE(loader.load_from_buffer(buf.data(), buf.size(), a) == false);
}

TEST_CASE("BinaryGaugeLoader rejects null buffer") {
    BinaryGaugeLoader loader;
    BinaryGaugeLoader::GaugeAsset a;
    REQUIRE(loader.load_from_buffer(nullptr, 10, a) == false);
}

TEST_CASE("BinaryGaugeLoader rejects undersized buffer") {
    BinaryGaugeLoader loader;
    BinaryGaugeLoader::GaugeAsset a;
    uint8_t tiny[7];
    REQUIRE(loader.load_from_buffer(tiny, 7, a) == false);
}

TEST_CASE("BinaryGaugeLoader validates empty asset rejects") {
    // Create a valid buffer but with zero dimensions
    std::vector<uint8_t> buf;
    append_u32(buf, 0x45474744); // valid magic
    append_u16(buf, 1);          // valid version
    append_u16(buf, 1);          // path_count = 1
    append_u16(buf, 0);          // width = 0 (invalid)
    append_u16(buf, 64);         // height

    // path[0]
    const char* id = "p0";
    append_u8(buf, (uint8_t)strlen(id));
    buf.insert(buf.end(), id, id + strlen(id));
    append_f32(buf, 1.0f);
    append_u8(buf, 10); append_u8(buf,20); append_u8(buf,30); append_u8(buf,255);
    append_u8(buf, 0);
    append_u8(buf, 1); append_u8(buf,1); append_u8(buf,2); append_u8(buf,3); append_u8(buf,255);
    append_u16(buf, 0); // cmd_count

    BinaryGaugeLoader loader;
    BinaryGaugeLoader::GaugeAsset asset;
    REQUIRE(loader.load_from_buffer(buf.data(), buf.size(), asset) == false);
}

TEST_CASE("BinaryGaugeLoader handles truncated path data") {
    std::vector<uint8_t> buf;
    append_u32(buf, 0x45474744); // valid magic
    append_u16(buf, 1);          // valid version
    append_u16(buf, 2);          // path_count = 2
    append_u16(buf, 128);        // width
    append_u16(buf, 64);         // height

    // path[0] - complete
    const char* id = "p0";
    append_u8(buf, (uint8_t)strlen(id));
    buf.insert(buf.end(), id, id + strlen(id));
    append_f32(buf, 1.0f);
    append_u8(buf, 10); append_u8(buf,20); append_u8(buf,30); append_u8(buf,255);
    append_u8(buf, 0);
    append_u8(buf, 1); append_u8(buf,1); append_u8(buf,2); append_u8(buf,3); append_u8(buf,255);
    append_u16(buf, 0); // cmd_count

    // path[1] - incomplete (just truncate before reaching ID length)
    // This should cause the loader to gracefully stop

    BinaryGaugeLoader loader;
    BinaryGaugeLoader::GaugeAsset asset;
    bool ok = loader.load_from_buffer(buf.data(), buf.size(), asset);
    // Should succeed with just the first path
    REQUIRE(ok == true);
    REQUIRE(asset.paths.size() == 1);
}

TEST_CASE("BinaryGaugeLoader parses multiple paths with commands") {
    std::vector<uint8_t> buf;
    append_u32(buf, 0x45474744);
    append_u16(buf, 1);
    append_u16(buf, 1); // 1 path
    append_u16(buf, 100);
    append_u16(buf, 100);

    // Path with 1 command
    const char* id = "curve";
    append_u8(buf, (uint8_t)strlen(id));
    buf.insert(buf.end(), id, id + strlen(id));
    append_f32(buf, 2.5f); // stroke width
    append_u8(buf, 100); append_u8(buf,150); append_u8(buf,200); append_u8(buf,255);
    append_u8(buf, 1); // cap = round

    append_u8(buf, 0); // fill not enabled
    append_u8(buf, 0); append_u8(buf,0); append_u8(buf,0); append_u8(buf,0);

    append_u16(buf, 1); // cmd_count = 1
    // Command: CubicTo
    append_u8(buf, 2); // PathCommand::Type::CubicTo
    append_f32(buf, 10.0f); append_f32(buf, 20.0f);
    append_f32(buf, 30.0f); append_f32(buf, 40.0f);
    append_f32(buf, 50.0f); append_f32(buf, 60.0f);

    BinaryGaugeLoader loader;
    BinaryGaugeLoader::GaugeAsset asset;
    REQUIRE(loader.load_from_buffer(buf.data(), buf.size(), asset) == true);
    REQUIRE(asset.paths.size() == 1);
    REQUIRE(asset.paths[0].commands.size() == 1);
}
