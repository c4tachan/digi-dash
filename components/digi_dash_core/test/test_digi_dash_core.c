#include <stdio.h>
#include "unity.h"
#include "digi_dash_core.h"

TEST_CASE("display initialization with valid parameters", "[digi_dash]")
{
    esp_err_t ret = digi_dash_display_init(320, 240);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_TRUE(digi_dash_is_initialized());
}

TEST_CASE("display initialization with zero width fails", "[digi_dash]")
{
    esp_err_t ret = digi_dash_display_init(0, 240);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

TEST_CASE("display initialization with zero height fails", "[digi_dash]")
{
    esp_err_t ret = digi_dash_display_init(320, 0);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

TEST_CASE("update data requires initialization", "[digi_dash]")
{
    // Without init, update should fail
    esp_err_t ret = digi_dash_update_data(60.0f, 3000, 90.0f);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
    
    // After init, should succeed
    digi_dash_display_init(320, 240);
    ret = digi_dash_update_data(60.0f, 3000, 90.0f);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

TEST_CASE("update data with negative speed fails", "[digi_dash]")
{
    digi_dash_display_init(320, 240);
    esp_err_t ret = digi_dash_update_data(-10.0f, 3000, 90.0f);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

TEST_CASE("update data with excessive rpm fails", "[digi_dash]")
{
    digi_dash_display_init(320, 240);
    esp_err_t ret = digi_dash_update_data(60.0f, 15000, 90.0f);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

TEST_CASE("update data with extreme temperature fails", "[digi_dash]")
{
    digi_dash_display_init(320, 240);
    
    // Too cold
    esp_err_t ret = digi_dash_update_data(60.0f, 3000, -100.0f);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    
    // Too hot
    ret = digi_dash_update_data(60.0f, 3000, 250.0f);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

TEST_CASE("kmh to mph conversion", "[digi_dash][conversion]")
{
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, digi_dash_kmh_to_mph(0.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 62.14f, digi_dash_kmh_to_mph(100.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 31.07f, digi_dash_kmh_to_mph(50.0f));
}

TEST_CASE("celsius to fahrenheit conversion", "[digi_dash][conversion]")
{
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 32.0f, digi_dash_celsius_to_fahrenheit(0.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 212.0f, digi_dash_celsius_to_fahrenheit(100.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 98.6f, digi_dash_celsius_to_fahrenheit(37.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -40.0f, digi_dash_celsius_to_fahrenheit(-40.0f));
}

TEST_CASE("value range checking", "[digi_dash][validation]")
{
    TEST_ASSERT_TRUE(digi_dash_is_in_range(50.0f, 0.0f, 100.0f));
    TEST_ASSERT_TRUE(digi_dash_is_in_range(0.0f, 0.0f, 100.0f));
    TEST_ASSERT_TRUE(digi_dash_is_in_range(100.0f, 0.0f, 100.0f));
    
    TEST_ASSERT_FALSE(digi_dash_is_in_range(-1.0f, 0.0f, 100.0f));
    TEST_ASSERT_FALSE(digi_dash_is_in_range(101.0f, 0.0f, 100.0f));
}

TEST_CASE("typical dashboard data updates", "[digi_dash][integration]")
{
    digi_dash_display_init(320, 240);
    
    // Normal highway driving
    TEST_ASSERT_EQUAL(ESP_OK, digi_dash_update_data(120.0f, 2500, 95.0f));
    
    // City driving
    TEST_ASSERT_EQUAL(ESP_OK, digi_dash_update_data(40.0f, 1500, 88.0f));
    
    // Stopped at light
    TEST_ASSERT_EQUAL(ESP_OK, digi_dash_update_data(0.0f, 800, 85.0f));
    
    // Cold start
    TEST_ASSERT_EQUAL(ESP_OK, digi_dash_update_data(0.0f, 1200, 20.0f));
}
