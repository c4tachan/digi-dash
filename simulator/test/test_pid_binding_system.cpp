#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <digidash/pid_binding_system.h>

using namespace digidash;

TEST_CASE("PIDBindingSystem register and default value") {
    PIDBindingSystem sys;
    PIDBindingSystem::PIDBinding b{
        .pid_id = 0x0C,
        .type = PIDBindingSystem::PIDType::ENGINE_SPEED,
        .unit = "rpm",
        .min_value = 0.0f,
        .max_value = 8000.0f,
        .scale = 1.0f,
        .offset = 0.0f
    };

    sys.register_binding(b);
    REQUIRE(sys.get_pid_value(0x0C) == Catch::Approx(0.0f));
}

TEST_CASE("PIDBindingSystem scales and clamps values") {
    PIDBindingSystem sys;
    PIDBindingSystem::PIDBinding b{
        .pid_id = 0x0D,
        .type = PIDBindingSystem::PIDType::VEHICLE_SPEED,
        .unit = "kph",
        .min_value = 0.0f,
        .max_value = 250.0f,
        .scale = 0.5f,
        .offset = 2.0f
    };

    sys.register_binding(b);

    // raw 100 -> scaled = 100*0.5 + 2 = 52
    sys.set_pid_value(0x0D, 100.0f);
    REQUIRE(sys.get_pid_value(0x0D) == Catch::Approx(52.0f));

    // below min -> clamped
    sys.set_pid_value(0x0D, -1000.0f);
    REQUIRE(sys.get_pid_value(0x0D) == Catch::Approx(0.0f));

    // above max -> clamped
    sys.set_pid_value(0x0D, 10000.0f);
    REQUIRE(sys.get_pid_value(0x0D) == Catch::Approx(250.0f));
}

TEST_CASE("PIDBindingSystem format_value returns numeric string") {
    PIDBindingSystem sys;
    PIDBindingSystem::PIDBinding b{
        .pid_id = 0x0E,
        .type = PIDBindingSystem::PIDType::THROTTLE_POSITION,
        .unit = "%",
        .min_value = 0.0f,
        .max_value = 100.0f,
        .scale = 1.0f,
        .offset = 0.0f
    };
    sys.register_binding(b);
    sys.set_pid_value(0x0E, 33.5f);
    auto s = sys.format_value(0x0E);
    REQUIRE(s.find("33.5") != std::string::npos);
}
