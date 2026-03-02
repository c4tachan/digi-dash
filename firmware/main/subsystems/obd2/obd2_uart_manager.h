#pragma once

#include <cstdint>
#include <functional>

namespace digidash {

class OBD2UartManager {
public:
    enum class ConnectionState {
        DISCONNECTED,
        INITIALIZING,
        CONNECTED,
        ERROR
    };

    using PIDCallback = std::function<void(uint32_t pid_id, float value)>;
    using StateCallback = std::function<void(ConnectionState state, const char* message)>;

    OBD2UartManager();
    ~OBD2UartManager();

    bool initialize();
    void update();

    void set_pid_callback(PIDCallback callback);
    void set_state_callback(StateCallback callback);

    ConnectionState get_state() const { return state_; }
    const char* get_status_message() const;

private:
    bool send_command(const char* cmd);
    bool send_command_and_wait(const char* cmd, uint32_t timeout_ms);
    bool configure_uart(int baud_rate);
    bool try_initialize_elm327();
    bool wait_for_prompt(uint32_t timeout_ms, char* out_response, size_t out_size);
    void read_uart_data();
    bool try_extract_response(char* out_response, size_t out_size);
    bool parse_pid_response(const char* response, uint8_t expected_pid, float& value);
    void update_pids();
    void set_state(ConnectionState state, const char* message = nullptr);

    ConnectionState state_;
    char status_message_[64];

    PIDCallback pid_callback_;
    StateCallback state_callback_;

    static constexpr int UART_PORT = 1;
    static constexpr int UART_TX_PIN = 16; // Qualia A1
    static constexpr int UART_RX_PIN = 17; // Qualia A0
    static constexpr int UART_BUFFER_SIZE = 1024;

    static constexpr int UART_BAUD_CANDIDATES[4] = {38400, 9600, 115200, 57600};
    static constexpr int UART_BAUD_CANDIDATE_COUNT = sizeof(UART_BAUD_CANDIDATES) / sizeof(UART_BAUD_CANDIDATES[0]);

    bool uart_initialized_;
    int current_baud_index_;

    char rx_buffer_[512];
    size_t rx_buffer_len_;

    bool waiting_pid_response_;
    uint8_t pending_pid_;
    uint32_t pending_pid_sent_ms_;
    bool protocol_locked_;
    uint32_t next_protocol_probe_ms_;

    uint32_t last_pid_update_ms_;
    uint32_t next_retry_ms_;
    uint8_t current_pid_index_;

    static constexpr uint32_t PID_UPDATE_INTERVAL_MS = 100;
    static constexpr uint32_t PID_RESPONSE_TIMEOUT_MS = 300;
    static constexpr uint32_t PROTOCOL_PROBE_INTERVAL_MS = 1000;
    static constexpr uint32_t RECONNECT_RETRY_INTERVAL_MS = 3000;
};

} // namespace digidash
