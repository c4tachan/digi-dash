/**
 * @file obd2_data_source.cpp
 * @brief OBD II data source implementation
 */

#include "obd2_data_source.h"
#include "config.h"
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/select.h>

OBD2DataSource::OBD2DataSource(const char* port_name, const std::map<std::string, PidConfig>& pid_config)
    : serial_handle_(nullptr), rpm_(0), speed_(0), coolant_temp_(0), connected_(false) {
    
    strncpy(port_name_, port_name, sizeof(port_name_) - 1);
    port_name_[sizeof(port_name_) - 1] = '\0';
    
    // Extract PID commands from config
    if (pid_config.count("rpm")) {
        rpm_pid_ = pid_config.at("rpm").command;
    }
    if (pid_config.count("speed")) {
        speed_pid_ = pid_config.at("speed").command;
    }
    if (pid_config.count("coolant_temp") || pid_config.count("temp")) {
        // Support both "coolant_temp" and "temp" keys
        const auto& key = pid_config.count("coolant_temp") ? "coolant_temp" : "temp";
        coolant_temp_pid_ = pid_config.at(key).command;
    }
    
    // Build name string
    snprintf(name_buffer_, sizeof(name_buffer_), "OBD II (%s)", port_name_);
    
    printf("OBD2DataSource initialized with:\n");
    printf("  RPM PID: %s\n", rpm_pid_.c_str());
    printf("  Speed PID: %s\n", speed_pid_.c_str());
    printf("  CoolantTemp PID: %s\n", coolant_temp_pid_.c_str());
}

OBD2DataSource::~OBD2DataSource() {
    disconnect();
}

void OBD2DataSource::update() {
    if (!connected_) {
        if (!connect()) {
            return;
        }
    }
    
    if (connected_) {
        // Query RPM if configured
        if (!rpm_pid_.empty()) {
            int rpm = queryOBD2(rpm_pid_);
            if (rpm >= 0) {
                rpm_ = rpm;
            } else {
                printf("OBD2: RPM query returned no valid data\n");
                fflush(stdout);
            }
        }
        
        // Query speed if configured
        if (!speed_pid_.empty()) {
            int speed = queryOBD2(speed_pid_);
            if (speed >= 0) {
                speed_ = speed;
            } else {
                printf("OBD2: Speed query returned no valid data\n");
                fflush(stdout);
            }
        }
        
        // Query coolant temp if configured
        if (!coolant_temp_pid_.empty()) {
            int temp = queryOBD2(coolant_temp_pid_);
            if (temp >= 0) {
                coolant_temp_ = temp;
            } else {
                printf("OBD2: Temp query returned no valid data\n");
                fflush(stdout);
            }
        }
    }
}

bool OBD2DataSource::connect() {
    fprintf(stderr, "OBD2: Attempting to connect to %s...\n", port_name_);
    
    // Open serial port
    int fd = open(port_name_, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        fprintf(stderr, "OBD2: Failed to open %s: %s\n", port_name_, strerror(errno));
        return false;
    }
    
    // Configure serial port (9600 8N1 for ELM327)
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        fprintf(stderr, "OBD2: Failed to get terminal attributes: %s\n", strerror(errno));
        close(fd);
        return false;
    }
    
    // Set baud rate to 38400 (common ELM327 default, some use 9600)
    cfsetospeed(&tty, B38400);
    cfsetispeed(&tty, B38400);
    
    // 8N1, no flow control
    tty.c_cflag &= ~PARENB;        // No parity
    tty.c_cflag &= ~CSTOPB;        // 1 stop bit
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;            // 8 data bits
    tty.c_cflag &= ~CRTSCTS;       // No hardware flow control
    tty.c_cflag |= CREAD | CLOCAL; // Enable receiver, ignore modem control
    
    // Raw mode
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL);
    tty.c_oflag &= ~OPOST;
    
    // Timeouts
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10; // 1 second timeout
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "OBD2: Failed to set terminal attributes: %s\n", strerror(errno));
        close(fd);
        return false;
    }
    
    serial_handle_ = (void*)(intptr_t)fd;
    
    // Initialize ELM327 adapter
    usleep(100000); // Wait 100ms for adapter to be ready
    
    char response[256];
    
    // Reset adapter
    printf("OBD2: Sending ATZ (reset)\n");
    if (!sendCommand("ATZ\r", response, sizeof(response))) {
        fprintf(stderr, "OBD2: Failed to reset adapter\n");
        disconnect();
        return false;
    }
    printf("OBD2: Reset response: [%s]\n", response);
    usleep(1500000); // Wait 1.5 seconds after reset for adapter to fully initialize
    
    // Turn off echo
    printf("OBD2: Sending ATE0 (echo off)\n");
    if (!sendCommand("ATE0\r", response, sizeof(response))) {
        fprintf(stderr, "OBD2: Failed to disable echo\n");
        disconnect();
        return false;
    }
    printf("OBD2: Echo off response: [%s]\n", response);
    usleep(200000); // Wait 200ms
    
    // Turn off line feeds
    printf("OBD2: Sending ATL0 (linefeed off)\n");
    if (!sendCommand("ATL0\r", response, sizeof(response))) {
        fprintf(stderr, "OBD2: Failed to disable linefeeds\n");
    }
    printf("OBD2: Linefeed off response: [%s]\n", response);
    usleep(200000); // Wait 200ms
    
    // Turn off spaces (optional - makes parsing easier)
    printf("OBD2: Sending ATS0 (spaces off)\n");
    if (!sendCommand("ATS0\r", response, sizeof(response))) {
        fprintf(stderr, "OBD2: Failed to disable spaces\n");
    }
    printf("OBD2: Spaces off response: [%s]\n", response);
    usleep(200000); // Wait 200ms
    
    // Set protocol to CAN (ISO 15765) for modern vehicles (2008+)
    // Try protocol 6 first (CAN 11-bit, 500kbaud) - most common for Asian/European cars
    printf("OBD2: Sending ATSP6 (CAN 11-bit)\n");
    if (!sendCommand("ATSP6\r", response, sizeof(response))) {
        fprintf(stderr, "OBD2: Failed to set protocol\n");
        disconnect();
        return false;
    }
    printf("OBD2: Protocol response: [%s]\n", response);
    usleep(200000); // Wait 200ms
    
    connected_ = true;
    fprintf(stderr, "OBD2: Successfully connected to %s\n", port_name_);
    return true;
}

void OBD2DataSource::disconnect() {
    if (serial_handle_) {
        int fd = (int)(intptr_t)serial_handle_;
        close(fd);
        serial_handle_ = nullptr;
    }
    connected_ = false;
    fprintf(stderr, "OBD2: Disconnected\n");
}

int OBD2DataSource::queryOBD2(const std::string& pid_command) {
    if (!connected_ || !serial_handle_) {
        return -1;
    }
    
    printf("OBD2: Sending query %s...\n", pid_command.c_str());
    fflush(stdout);
    
    // Send command (e.g. "010C" for RPM) with carriage return
    std::string cmd = pid_command + "\r";
    char response[256];
    
    if (!sendCommand(cmd.c_str(), response, sizeof(response))) {
        fprintf(stderr, "OBD2: sendCommand failed for PID %s\n", pid_command.c_str());
        return -1;
    }
    
    // Debug: Print raw response
    printf("OBD2 Query %s -> Response: [%s] (len=%zu)\n", pid_command.c_str(), response, strlen(response));
    fflush(stdout);
    
    if (strlen(response) == 0) {
        printf("  -> Empty response\n");
        return -1;
    }
    
    // Skip "BUS INIT", "SEARCHING", and "STOPPED" messages
    if (strstr(response, "BUS INIT") || strstr(response, "SEARCHING") || 
        strstr(response, "STOPPED") || strstr(response, "UNABLE") || 
        strstr(response, "ERROR") || strstr(response, "NO DATA") ||
        strlen(response) < 6) {
        // Vehicle is initializing or not responding
        // Print helpful message on first occurrence
        static bool warned = false;
        if (!warned && (strstr(response, "STOPPED") || strstr(response, "NO DATA"))) {
            printf("\n*** Vehicle ECU not responding ***\n");
            printf("*** Make sure: ***\n");
            printf("***  1. Vehicle ignition is ON\n");
            printf("***  2. ELM327 is connected to vehicle\n");
            printf("***  3. Try starting the engine\n\n");
            warned = true;
        }
        return -1;
    }
    
    // Parse response based on PID
    // RPM (010C): Response "41 0C xx yy" -> value = ((A*256)+B)/4
    // Speed (010D): Response "41 0D xx" -> value = A
    // Coolant (0105): Response "41 05 xx" -> value = A - 40
    
    // Extract hex bytes from response (handle with or without spaces)
    unsigned int mode, pid, a, b;
    b = 0;  // Initialize b to 0 for PIDs that only return one byte
    
    int parsed = sscanf(response, "%x %x %x %x", &mode, &pid, &a, &b);
    
    if (parsed < 3 || mode != 0x41) {
        // Try without spaces (some adapters format differently)
        parsed = sscanf(response, "%2x%2x%2x%2x", &mode, &pid, &a, &b);
        if (parsed < 3 || mode != 0x41) {
            return -1;  // Silently ignore invalid responses during init
        }
    }
    
    // Decode based on PID
    if (pid == 0x0C && parsed >= 4) {
        // RPM: ((A*256)+B)/4
        int value = ((a * 256) + b) / 4;
        printf("  -> Decoded RPM: %d (raw: %02X %02X)\n", value, a, b);
        return value;
    } else if (pid == 0x0D) {
        // Speed: A km/h
        printf("  -> Decoded Speed: %d km/h (raw: %02X)\n", a, a);
        return a;
    } else if (pid == 0x05) {
        // Coolant temp: A - 40
        int value = a - 40;
        printf("  -> Decoded Temp: %d°C (raw: %02X)\n", value, a);
        return value;
    }
    
    return -1;
}

const char* OBD2DataSource::getName() const {
    return name_buffer_;
}

bool OBD2DataSource::sendCommand(const char* command, char* response, size_t response_size) {
    if (!serial_handle_) {
        return false;
    }
    
    int fd = (int)(intptr_t)serial_handle_;
    
    // Clear input buffer
    tcflush(fd, TCIFLUSH);
    
    // Send command
    size_t cmd_len = strlen(command);
    ssize_t written = write(fd, command, cmd_len);
    if (written != (ssize_t)cmd_len) {
        fprintf(stderr, "OBD2: Failed to write command\n");
        return false;
    }
    
    // If no response buffer provided, command is fire-and-forget
    if (!response || response_size == 0) {
        usleep(100000); // Wait 100ms
        return true;
    }
    
    // Read response with timeout
    char buffer[512];
    size_t total_read = 0;
    int attempts = 0;
    const int max_attempts = 10; // 1 second total timeout (reduced from 20)
    
    while (attempts < max_attempts) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // 100ms
        
        int ret = select(fd + 1, &readfds, nullptr, nullptr, &timeout);
        
        if (ret > 0) {
            ssize_t n = read(fd, buffer + total_read, sizeof(buffer) - total_read - 1);
            if (n > 0) {
                total_read += n;
                buffer[total_read] = '\0';
                
                // Check if we got a complete response (ends with '>' or contains "OK" or "ERROR")
                if (strchr(buffer, '>') || strstr(buffer, "OK") || strstr(buffer, "ERROR")) {
                    break;
                }
            }
        }
        
        attempts++;
    }
    
    if (total_read == 0) {
        printf("OBD2: Timeout - no response received for command\n");
        fflush(stdout);
        return false;
    }
    
    printf("OBD2: Raw buffer (%zu bytes): [%s]\n", total_read, buffer);
    fflush(stdout);
    
    // Copy to response buffer and parse
    // Keep the last line that looks like a valid response (not just ">")
    char* line_start = buffer;
    char* last_valid_line = nullptr;
    
    for (size_t i = 0; i < total_read; i++) {
        if (buffer[i] == '\r' || buffer[i] == '\n') {
            buffer[i] = '\0';
            if (line_start < buffer + i && strlen(line_start) > 0) {
                // Skip lines that are just ">" prompt
                if (strcmp(line_start, ">") != 0) {
                    last_valid_line = line_start;
                }
            }
            line_start = buffer + i + 1;
        }
    }
    // Handle case where buffer doesn't end with newline
    if (line_start < buffer + total_read && strlen(line_start) > 0) {
        // Skip if it's just ">"
        if (strcmp(line_start, ">") != 0) {
            last_valid_line = line_start;
        }
    }
    
    if (last_valid_line) {
        // Trim leading spaces
        while (*last_valid_line == ' ' || *last_valid_line == '\t') {
            last_valid_line++;
        }
        strncpy(response, last_valid_line, response_size - 1);
        response[response_size - 1] = '\0';
        
        // Trim trailing spaces and ">" prompt
        size_t len = strlen(response);
        while (len > 0 && (response[len-1] == ' ' || response[len-1] == '\t' || response[len-1] == '>')) {
            response[--len] = '\0';
        }
    } else {
        response[0] = '\0';
    }
    
    return true;
}
