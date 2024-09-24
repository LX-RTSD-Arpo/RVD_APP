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
        perror("Unable to open configuration filee");
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

int main() {
    uint8_t reset_output1_enable = 1;
    uint8_t reset_output2_enable = 1;
    if (parse_config("../config.txt", &reset_output1_enable, &reset_output2_enable) != 0)
    {
        fprintf(stderr, "Failed to parse configuration file\n");
    }

    int attempts = 0;
    int fd, mc;
    float voltage = 0;
    char serial_port[20];
    modbus_t *ctx;
    uint16_t holding_registers[REGIS_COUNT] = {0};  // Array to store received data
    uint16_t input_registers[REGIS_COUNT] = {0};
    uint8_t coils[REGIS_COUNT] = {0};

    while (attempts < MAX_SERIAL_PORT_ATTEMPTS)
    {
        snprintf(serial_port, sizeof(serial_port), "/dev/ttyUSB%d", attempts);
        fd = open(serial_port, O_RDWR | O_NOCTTY);

        if (fd != -1)
        {
            printf("Serial port %s opened successfully.\n", serial_port);
            break;
        }
        else
        {
            if (errno == ENOENT)
            {
                printf("%s does not exist. Trying the next port...\n", serial_port);
            }
            else
            {
                perror("Error opening serial port");
                break;
            }
        }

        snprintf(serial_port, sizeof(serial_port), "/dev/ttyACM%d", attempts);
        fd = open(serial_port, O_RDWR | O_NOCTTY);

        if (fd != -1)
        {
            printf("Serial port %s opened successfully.\n", serial_port);
            close(fd);
            break;
        }
        else
        {
            if (errno == ENOENT)
            {
                printf("%s does not exist. Trying the next port...\n", serial_port);
            }
            else
            {
                perror("Error opening serial port");
                break;
            }
        }
        attempts++;
    }

    ctx = modbus_new_rtu(serial_port, 9600, 'N', 8, 1);
    modbus_set_response_timeout(ctx, 5, 0);
    if (ctx == NULL || mc == -1)
    {
        if (ctx == NULL)
        {
            printf("\n\t[-]ERROR!! --> No device connected on %s", serial_port);
            return -2;
        }

        if (mc == -1)
        {
            printf("\n\t[-]ERROR!! --> Can't connect to I/O!!\n");
            return -3;
        }
    }

    // Set the MODBUS slave address
    modbus_set_slave(ctx, SLAVEID);
    mc = modbus_connect(ctx);
    if (mc == -1)
    {
        fprintf(stderr, "\n\t[-]Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        return -3;
    }

    /*int ri = modbus_read_input_registers(ctx, REGIS_START, REGIS_COUNT, input_registers);
    if (ri == -1)
    {
        fprintf(stderr, "\n[-]MODBUS read error: %s\n", modbus_strerror(errno));
        //modbus_close(ctx);
        //modbus_free(ctx);
        //return -3;
    }
    else
    {
        for (int i = 0; i < REGIS_COUNT; i++)
        {
            voltage = ((float)input_registers[i] / 400) * 24;
            printf("\t\t\nInput Voltage CH%d = %d ", i + 1, (int)voltage);
        }
    }*/
    
    coils[0] = reset_output1_enable;
    coils[1] = reset_output2_enable;
    
    int rcw = modbus_write_bits(ctx, REGIS_START, REGIS_COUNT, coils);
    if (rcw == -1)
    {
        fprintf(stderr, "\n[-]MODBUS Write error: %s\n", modbus_strerror(errno));
        //modbus_close(ctx);
        //modbus_free(ctx);
        //return -3;
    }
    // Close the MODBUS RTU connection
    //modbus_close(ctx);
    //modbus_free(ctx);

    return 0;
}