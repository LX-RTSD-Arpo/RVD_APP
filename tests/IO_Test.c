#include <stdio.h>
#include "../include/lib/modbus/modbus.h"
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#define SLAVEID 1
#define MAX_SERIAL_PORT_ATTEMPTS 5
#define REGIS_START 0
#define REGIS_COUNT 2
#define POLL_INTERVAL 2

int parse_config(const char *filename, uint8_t *reset_output1_enable, uint8_t *reset_output2_enable)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Unable to open configuration file");
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), file))
    {
        char *key = strtok(line, "=");
        char *value = strtok(NULL, "\n");

        if (key && value)
        {
            if (strcmp(key, "RESET_OUTPUT1_ENABLE") == 0)
                *reset_output1_enable = atoi(value);
            else if (strcmp(key, "RESET_OUTPUT2_ENABLE") == 0)
                *reset_output2_enable = atoi(value);
        }
    }

    fclose(file);
    return 0;
}

int try_open_serial_port(const char *port_format, int attempt, char *serial_port)
{
    snprintf(serial_port, 20, port_format, attempt);
    int fd = open(serial_port, O_RDWR | O_NOCTTY);
    if (fd == -1)
    {
        if (errno == ENOENT)
        {
            printf("%s does not exist. Trying the next port...\n", serial_port);
        }
        else
        {
            perror("Error opening serial port");
        }
    }
    return fd;
}

int main() {
    uint8_t reset_output1_enable = 1;
    uint8_t reset_output2_enable = 1;

    if (parse_config("/root/RVD/config.txt", &reset_output1_enable, &reset_output2_enable) != 0)
    {
        fprintf(stderr, "Failed to parse configuration file\n");
        return -1;
    }

    int attempts = 0;
    int fd = -1;
    char serial_port[20];
    modbus_t *ctx = NULL;
    uint16_t holding_registers[REGIS_COUNT] = {0};  // Array to store received data
    uint16_t input_registers[REGIS_COUNT] = {0};
    uint8_t coils[REGIS_COUNT] = {0};

    // Try /dev/ttyUSB and /dev/ttyACM for MAX_SERIAL_PORT_ATTEMPTS times
    while (attempts < MAX_SERIAL_PORT_ATTEMPTS && fd == -1)
    {
        fd = try_open_serial_port("/dev/ttyUSB%d", attempts, serial_port);
        if (fd == -1)
            fd = try_open_serial_port("/dev/ttyACM%d", attempts, serial_port);

        attempts++;
    }

    if (fd == -1)
    {
        fprintf(stderr, "Failed to open any serial ports after %d attempts\n", MAX_SERIAL_PORT_ATTEMPTS);
        return -1;
    }
    else
    {
        printf("Serial port %s opened successfully.\n", serial_port);
        close(fd); // Close since we only need this for verification.
    }

    ctx = modbus_new_rtu(serial_port, 9600, 'N', 8, 1);
    if (ctx == NULL)
    {
        fprintf(stderr, "Unable to create the libmodbus context\n");
        return -2;
    }

    modbus_set_slave(ctx, SLAVEID);
    modbus_set_response_timeout(ctx, 5, 0);

    // Attempt to connect to the device
    int mc = modbus_connect(ctx);
    if (mc == -1)
    {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        return -3;
    }

    coils[0] = reset_output1_enable;
    coils[1] = reset_output2_enable;

    int rcw = modbus_write_bits(ctx, REGIS_START, REGIS_COUNT, coils);
    if (rcw == -1)
    {
        fprintf(stderr, "MODBUS Write error: %s\n", modbus_strerror(errno));
        modbus_close(ctx);
        modbus_free(ctx);
        return -3;
    }

    // Cleanup: Close the MODBUS connection and free the context
    modbus_close(ctx);
    modbus_free(ctx);

    return 0;
}
