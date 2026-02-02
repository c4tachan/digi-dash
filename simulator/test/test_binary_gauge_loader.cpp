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
