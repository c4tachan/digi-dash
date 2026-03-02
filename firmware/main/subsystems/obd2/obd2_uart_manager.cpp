#include "obd2_uart_manager.h"

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstdio>
#include <cstring>

static const char* TAG = "OBD2UART";

namespace digidash {

namespace {

void format_uart_bytes(const uint8_t* data, size_t len, char* out, size_t out_size) {
    if (out_size == 0) {
        return;
    }

    size_t write_index = 0;
    for (size_t i = 0; i < len && write_index + 1 < out_size; ++i) {
        const char ch = static_cast<char>(data[i]);

        if (ch == '\r') {
            if (write_index + 2 < out_size) {
                out[write_index++] = '\\';
                out[write_index++] = 'r';
            } else {
                break;
            }
            continue;
        }

        if (ch == '\n') {
            if (write_index + 2 < out_size) {
                out[write_index++] = '\\';
                out[write_index++] = 'n';
            } else {
                break;
            }
            continue;
        }

        if (ch >= 32 && ch <= 126) {
            out[write_index++] = ch;
        } else {
            out[write_index++] = '.';
        }
    }

    out[write_index] = '\0';
}

} // namespace

OBD2UartManager::OBD2UartManager()
    : state_(ConnectionState::DISCONNECTED)
    , pid_callback_(nullptr)
    , state_callback_(nullptr)
    , uart_initialized_(false)
    , current_baud_index_(0)
    , rx_buffer_len_(0)
    , waiting_pid_response_(false)
    , pending_pid_(0)
    , pending_pid_sent_ms_(0)
    , protocol_locked_(false)
    , next_protocol_probe_ms_(0)
    , last_pid_update_ms_(0)
    , next_retry_ms_(0)
    , current_pid_index_(0) {
    std::memset(status_message_, 0, sizeof(status_message_));
    std::memset(rx_buffer_, 0, sizeof(rx_buffer_));
    std::strcpy(status_message_, "Not initialized");
}

OBD2UartManager::~OBD2UartManager() {
    uart_driver_delete(static_cast<uart_port_t>(UART_PORT));
}

bool OBD2UartManager::initialize() {
    if (!uart_initialized_) {
        if (!configure_uart(UART_BAUD_CANDIDATES[current_baud_index_])) {
            set_state(ConnectionState::ERROR, "UART config failed");
            return false;
        }
        uart_initialized_ = true;
    }

    bool ok = try_initialize_elm327();
    if (!ok) {
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        next_retry_ms_ = now + RECONNECT_RETRY_INTERVAL_MS;
    }
    return ok;
}

bool OBD2UartManager::configure_uart(int baud_rate) {
    set_state(ConnectionState::INITIALIZING, "Configuring UART...");

    const uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t ret = ESP_OK;

    if (!uart_initialized_) {
        ret = uart_driver_install(
            static_cast<uart_port_t>(UART_PORT),
            UART_BUFFER_SIZE,
            0,
            0,
            nullptr,
            0);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(ret));
            return false;
        }

        ret = uart_set_pin(
            static_cast<uart_port_t>(UART_PORT),
            UART_TX_PIN,
            UART_RX_PIN,
            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(ret));
            return false;
        }
    }

    ret = uart_param_config(static_cast<uart_port_t>(UART_PORT), &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(ret));
        return false;
    }

    ret = uart_set_baudrate(static_cast<uart_port_t>(UART_PORT), baud_rate);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_baudrate failed: %s", esp_err_to_name(ret));
        return false;
    }

    uart_flush(static_cast<uart_port_t>(UART_PORT));
    rx_buffer_len_ = 0;
    std::memset(rx_buffer_, 0, sizeof(rx_buffer_));

    ESP_LOGI(TAG, "UART configured for ELM327: TX=GPIO%d(A1), RX=GPIO%d(A0), %d baud",
             UART_TX_PIN, UART_RX_PIN, baud_rate);

    return true;
}

bool OBD2UartManager::try_initialize_elm327() {
    const int baud = UART_BAUD_CANDIDATES[current_baud_index_];
    if (!configure_uart(baud)) {
        set_state(ConnectionState::ERROR, "UART config failed");
        return false;
    }

    set_state(ConnectionState::INITIALIZING, "Initializing ELM327...");

    send_command("");
    vTaskDelay(pdMS_TO_TICKS(200));
    send_command("");
    vTaskDelay(pdMS_TO_TICKS(200));
    uart_flush(static_cast<uart_port_t>(UART_PORT));

    if (!send_command_and_wait("AT", 1200)) {
        current_baud_index_ = (current_baud_index_ + 1) % UART_BAUD_CANDIDATE_COUNT;
        set_state(ConnectionState::ERROR, "ELM327 handshake failed (retrying)");
        return false;
    }

    if (!send_command_and_wait("ATZ", 5000)) {
        ESP_LOGW(TAG, "ATZ did not complete; continuing with current session");
    }

    if (!send_command_and_wait("ATE0", 1000)) {
        set_state(ConnectionState::ERROR, "ELM327 echo off failed");
        return false;
    }
    if (!send_command_and_wait("ATL0", 1000)) {
        set_state(ConnectionState::ERROR, "ELM327 linefeed off failed");
        return false;
    }
    if (!send_command_and_wait("ATH0", 1000)) {
        set_state(ConnectionState::ERROR, "ELM327 headers off failed");
        return false;
    }
    // Lock to K-line family instead of auto protocol detection.
    // Try ISO 9141-2 first, then KWP variants.
    bool protocol_set = false;
    if (send_command_and_wait("ATSP3", 1500)) { // ISO 9141-2
        ESP_LOGI(TAG, "ELM protocol locked: ATSP3 (ISO 9141-2)");
        protocol_set = true;
    } else if (send_command_and_wait("ATSP4", 1500)) { // KWP 5 baud init
        ESP_LOGI(TAG, "ELM protocol locked: ATSP4 (KWP 5 baud)");
        protocol_set = true;
    } else if (send_command_and_wait("ATSP5", 1500)) { // KWP fast init
        ESP_LOGI(TAG, "ELM protocol locked: ATSP5 (KWP fast)");
        protocol_set = true;
    }

    if (!protocol_set) {
        set_state(ConnectionState::ERROR, "ELM327 K-line protocol lock failed");
        return false;
    }

    rx_buffer_len_ = 0;
    waiting_pid_response_ = false;
    pending_pid_ = 0;
    protocol_locked_ = false;
    next_protocol_probe_ms_ = 0;
    last_pid_update_ms_ = 0;
    current_pid_index_ = 0;
    next_retry_ms_ = 0;

    set_state(ConnectionState::CONNECTED, "ELM327 UART connected");
    return true;
}

bool OBD2UartManager::send_command(const char* cmd) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%s\r", cmd);
    int written = uart_write_bytes(
        static_cast<uart_port_t>(UART_PORT),
        buffer,
        std::strlen(buffer));
    ESP_LOGI(TAG, "TX -> %s", cmd[0] == '\0' ? "<CR>" : cmd);
    return written > 0;
}

bool OBD2UartManager::send_command_and_wait(const char* cmd, uint32_t timeout_ms) {
    if (!send_command(cmd)) {
        return false;
    }

    char response[256];
    if (!wait_for_prompt(timeout_ms, response, sizeof(response))) {
        ESP_LOGW(TAG, "No prompt for command: %s", cmd);
        return false;
    }

    ESP_LOGI(TAG, "ELM response (%s): %s", cmd, response);

    if (std::strstr(response, "ERROR") || std::strstr(response, "UNABLE TO CONNECT")) {
        return false;
    }
    return true;
}

bool OBD2UartManager::wait_for_prompt(uint32_t timeout_ms, char* out_response, size_t out_size) {
    size_t len = 0;
    size_t total_bytes = 0;
    uint32_t start_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

    while ((xTaskGetTickCount() * portTICK_PERIOD_MS) - start_ms < timeout_ms) {
        uint8_t temp[64];
        int read = uart_read_bytes(
            static_cast<uart_port_t>(UART_PORT),
            temp,
            sizeof(temp),
            pdMS_TO_TICKS(20));

        if (read > 0) {
            total_bytes += static_cast<size_t>(read);

            char chunk[196];
            format_uart_bytes(temp, static_cast<size_t>(read), chunk, sizeof(chunk));
            ESP_LOGI(TAG, "RX <- %s", chunk);

            for (int i = 0; i < read; ++i) {
                if (len + 1 >= out_size) {
                    out_response[len] = '\0';
                    return false;
                }
                out_response[len++] = static_cast<char>(temp[i]);
                if (temp[i] == '>') {
                    out_response[len] = '\0';
                    return true;
                }
            }
        }
    }

    if (len < out_size) {
        out_response[len] = '\0';
    }
    ESP_LOGW(TAG, "Prompt wait timed out after %u ms (received %u bytes)",
             static_cast<unsigned>(timeout_ms),
             static_cast<unsigned>(total_bytes));
    return false;
}

void OBD2UartManager::read_uart_data() {
    uint8_t temp[128];
    int read = uart_read_bytes(
        static_cast<uart_port_t>(UART_PORT),
        temp,
        sizeof(temp),
        0);

    if (read <= 0) {
        return;
    }

    char chunk[260];
    format_uart_bytes(temp, static_cast<size_t>(read), chunk, sizeof(chunk));
    ESP_LOGI(TAG, "RX <- %s", chunk);

    size_t space = sizeof(rx_buffer_) - 1 - rx_buffer_len_;
    size_t to_copy = (read > static_cast<int>(space)) ? space : static_cast<size_t>(read);
    if (to_copy == 0) {
        rx_buffer_len_ = 0;
        return;
    }

    std::memcpy(rx_buffer_ + rx_buffer_len_, temp, to_copy);
    rx_buffer_len_ += to_copy;
    rx_buffer_[rx_buffer_len_] = '\0';
}

bool OBD2UartManager::try_extract_response(char* out_response, size_t out_size) {
    char* prompt = std::strchr(rx_buffer_, '>');
    if (!prompt) {
        return false;
    }

    size_t response_len = static_cast<size_t>(prompt - rx_buffer_);
    if (response_len >= out_size) {
        response_len = out_size - 1;
    }

    std::memcpy(out_response, rx_buffer_, response_len);
    out_response[response_len] = '\0';

    size_t consumed = static_cast<size_t>(prompt - rx_buffer_) + 1;
    size_t remaining = rx_buffer_len_ - consumed;
    if (remaining > 0) {
        std::memmove(rx_buffer_, rx_buffer_ + consumed, remaining);
    }
    rx_buffer_len_ = remaining;
    rx_buffer_[rx_buffer_len_] = '\0';
    return true;
}

bool OBD2UartManager::parse_pid_response(const char* response, uint8_t expected_pid, float& value) {
    const char* pos = response;
    while ((pos = std::strstr(pos, "41")) != nullptr) {
        unsigned mode = 0;
        unsigned pid = 0;
        unsigned a = 0;
        unsigned b = 0;

        int parsed = std::sscanf(pos, "%x %x %x %x", &mode, &pid, &a, &b);
        if (parsed >= 3 && mode == 0x41 && pid == expected_pid) {
            switch (expected_pid) {
                case 0x0C:
                    if (parsed < 4) {
                        return false;
                    }
                    value = ((a * 256.0f) + b) / 4.0f;
                    return true;
                case 0x0D:
                    value = static_cast<float>(a);
                    return true;
                case 0x05:
                    value = static_cast<float>(a) - 40.0f;
                    return true;
                default:
                    return false;
            }
        }
        ++pos;
    }
    return false;
}

void OBD2UartManager::update_pids() {
    static constexpr uint8_t pids[] = {0x0C, 0x0D, 0x05}; // RPM, Speed, Coolant
    static constexpr uint8_t num_pids = sizeof(pids) / sizeof(pids[0]);

    pending_pid_ = pids[current_pid_index_];
    current_pid_index_ = static_cast<uint8_t>((current_pid_index_ + 1) % num_pids);

    char cmd[8];
    std::snprintf(cmd, sizeof(cmd), "01%02X", pending_pid_);
    if (send_command(cmd)) {
        waiting_pid_response_ = true;
        pending_pid_sent_ms_ = xTaskGetTickCount() * portTICK_PERIOD_MS;
    }
}

void OBD2UartManager::update() {
    if (state_ != ConnectionState::CONNECTED) {
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (next_retry_ms_ == 0 || now >= next_retry_ms_) {
            try_initialize_elm327();
            if (state_ != ConnectionState::CONNECTED) {
                next_retry_ms_ = now + RECONNECT_RETRY_INTERVAL_MS;
            }
        }
        return;
    }

    read_uart_data();

    if (waiting_pid_response_) {
        char response[256];
        if (try_extract_response(response, sizeof(response))) {
            float value = 0.0f;
            if (!protocol_locked_ && std::strstr(response, "41 00")) {
                protocol_locked_ = true;
                ESP_LOGI(TAG, "Protocol locked (41 00 received), starting normal PID polling");
            }

            if (parse_pid_response(response, pending_pid_, value)) {
                uint32_t pid_id = 0;
                switch (pending_pid_) {
                    case 0x0C: pid_id = 0; break; // RPM
                    case 0x0D: pid_id = 1; break; // Speed
                    case 0x05: pid_id = 3; break; // Coolant
                    default: break;
                }
                if (pid_callback_) {
                    pid_callback_(pid_id, value);
                }
                ESP_LOGD(TAG, "PID 0x%02X => %.2f", pending_pid_, value);
            }
            waiting_pid_response_ = false;
            pending_pid_ = 0;
            return;
        }

        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (now - pending_pid_sent_ms_ > PID_RESPONSE_TIMEOUT_MS) {
            waiting_pid_response_ = false;
            pending_pid_ = 0;
        }
        return;
    }

    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    if (!protocol_locked_) {
        if (now >= next_protocol_probe_ms_) {
            if (send_command("0100")) {
                waiting_pid_response_ = true;
                pending_pid_ = 0x00;
                pending_pid_sent_ms_ = now;
                next_protocol_probe_ms_ = now + PROTOCOL_PROBE_INTERVAL_MS;
            }
        }
        return;
    }

    if (now - last_pid_update_ms_ >= PID_UPDATE_INTERVAL_MS) {
        last_pid_update_ms_ = now;
        update_pids();
    }
}

void OBD2UartManager::set_pid_callback(PIDCallback callback) {
    pid_callback_ = callback;
}

void OBD2UartManager::set_state_callback(StateCallback callback) {
    state_callback_ = callback;
}

void OBD2UartManager::set_state(ConnectionState state, const char* message) {
    state_ = state;
    if (message) {
        std::strncpy(status_message_, message, sizeof(status_message_) - 1);
        status_message_[sizeof(status_message_) - 1] = '\0';
    }

    ESP_LOGI(TAG, "State: %d - %s", static_cast<int>(state_), status_message_);
    if (state_callback_) {
        state_callback_(state_, status_message_);
    }
}

const char* OBD2UartManager::get_status_message() const {
    return status_message_;
}

} // namespace digidash
