#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include "../include/lib/modbus/modbus.h"
#include <linux/if_link.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define MAX_QUEUE 10000
#define BUF_SIZ 1024
#define ALIVE_MSG_DURATION 1

#define SLAVEID 1
#define MAX_SERIAL_PORT_ATTEMPTS 5
#define REGIS_START 0
#define REGIS_COUNT 2
#define COILS_REGIS 4
#define POLL_INTERVAL 2

#define CRC16_INIT 0xFFFF

#define MAX_LINE_LENGTH 256

FILE *storefile;

unsigned char start[30] = {0x7e, 0x01, 0x0c, 0x00, 0x10, 0x07, 0x00, 0x00, 0x00, 0x00, 0x80, 0x88,
                           0x03, 0x00, 0x00, 0x00,
                           0x01, 0x00, 0x00, 0x01, // Client ID
                           0xc0, 0xa8, 0x0b, 0x08, // NanoPi IP
                           0xd9, 0x03,             // Port
                           0x00, 0x00,
                           0xf4, 0x49}; // CRC
const char kHeader[16] = {0x7e, 0x01, 0x10, 0x00, 0x21, 0x04, 0x00, 0x00, 0x00, 0x08, 0x01, 0x00, 0x00, 0x01, 0xE5, 0xCD};
const char kMsgAct[5] = {0x03, 0xfb, 0x08, 0xE8, 0x03};
char dev_addr[2] = {0x02, 0x65};
const char *kTargetInterface1 = "eth0";
const char *kTargetInterface2 = "eth1";
const uint16_t kDevice_version = 0x00C8;
const uint32_t kDevice_DNS = 0x00000000;
uint8_t dev_ntimeout = 30;
const uint16_t kCRC_Table[] = {0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
                              0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
                              0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
                              0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
                              0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
                              0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
                              0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
                              0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
                              0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
                              0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
                              0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
                              0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
                              0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
                              0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
                              0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
                              0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
                              0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
                              0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
                              0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
                              0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
                              0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
                              0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
                              0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
                              0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
                              0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
                              0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
                              0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
                              0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
                              0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
                              0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
                              0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
                              0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0};
const char* kConfig_File = "/root/RVD_APP/RVDWebPages_v12/webconfig/web_config.ini";

char serial_port[20];
int data_size, len, leng, msg_counter, receiver_size, rbc, sock_mcast, sock_raw, sock_serv;
uint8_t ntp_attempt = 0;
long long tm;
time_t t;
socklen_t addrlen;
struct ifaddrs *ifa, *ifap;
struct ifreq mac;
struct sockaddr_in mcast, mcast_dest, receiver, *sa, server;
struct timeval tv, net_tv, stimeout;
pthread_t udpth, aliveth, stimeth, ntp_thread;

int a, b, i, j, k, q, val, Crc_Byte, Crc_Time, Crc_UDP;
unsigned char backup_buffer[BUF_SIZ], checker[BUF_SIZ], Tchecker[BUF_SIZ], udp_checker[BUF_SIZ], client_buf[BUF_SIZ], crc_buf[2], crc_tbuf[2], crc_ubuf[2], dummy1[22],
    dummy2[33], len_buf[2], recv_buf[BUF_SIZ], packet[BUF_SIZ], seq_buf[3], tcp_pack[BUF_SIZ], tm_buf[8], val_buf[BUF_SIZ], ver_buf[2];

int mc, fd, idx;
modbus_t *ctx;
uint16_t holding_registers[REGIS_COUNT] = {0};
uint16_t input_registers[REGIS_COUNT] = {0};
uint8_t coils[COILS_REGIS] = {0};

typedef struct
{
    char *ipAddress1;
    char *ipAddress2;
    char *gateway[10];
} NetworkInfo;

typedef struct
{
    void *(*thread_function)(void *);
    NetworkInfo *networkInfo;
    int radar_port;
} ThreadArgs;

typedef struct
{
    unsigned char qbuffer[MAX_QUEUE][BUF_SIZ];
    int front, rear;
    int count;
} CircularBufferMatrix;
CircularBufferMatrix cb1;

typedef struct
{
    char radar_ip[INET_ADDRSTRLEN];
    char pi_ip[INET_ADDRSTRLEN];
    int radar_alive_port;
} alive_thread_args;

typedef struct {
    char ntp_server[50];
    int ntp_timesync;
    int ntp_timeout;
    //int ntp_timesync_wait;
} NTPSettings;

uint16_t checksum(unsigned char *, int, const uint16_t *);
void SetTime();
void initBuffer(CircularBufferMatrix *);
int isFull(CircularBufferMatrix *);
int isEmpty(CircularBufferMatrix *);
void enqueue(CircularBufferMatrix *, unsigned char *);
void dequeue(CircularBufferMatrix *, unsigned char *);
NetworkInfo *ExtractNetwork(const char *, const char *);
// void read_ntp_settings(const char*, NTPSettings*);
// int check_ntp_sync(const char*);
// time_t get_file_mod_time(const char*);

int parse_config(const char *filename,
                 char *radar_ip,
                 int *radar_port,
                 int *radar_alive_port,
                 int *server_port,
                 uint8_t *response_sender,
                 uint8_t *response_receiver,
                 uint8_t *dev_ntimeout,
                 uint8_t *reset_output1_enable,
                 uint8_t *reset_output2_enable)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Unable to open configuration filee");
        return -1;
    }

    char line[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), file))
    {
        char *key = strtok(line, "=");
        char *value = strtok(NULL, "\n");

        if (key && value)
        {
            if (strcmp(key, "RADAR_IP") == 0)
                strncpy(radar_ip, value, INET_ADDRSTRLEN);
            else if (strcmp(key, "RADAR_PORT") == 0)
                *radar_port = atoi(value);
            else if (strcmp(key, "RADAR_ALIVE_PORT") == 0)
                *radar_alive_port = atoi(value);
            else if (strcmp(key, "SERVER_PORT") == 0)
                *server_port = atoi(value);
            else if (strcmp(key, "RESPONSE_SENDER") == 0)
                *response_sender = atoi(value);
            else if (strcmp(key, "RESPONSE_RECEIVER") == 0)
                *response_receiver = atoi(value);
            else if (strcmp(key, "DEVICE_NETWORK_TIMEOUT") == 0)
                *dev_ntimeout = atoi(value);
            else if (strcmp(key, "RESET_OUTPUT1_ENABLE") == 0)
                *reset_output1_enable = atoi(value);
            else if (strcmp(key, "RESET_OUTPUT2_ENABLE") == 0)
                *reset_output2_enable = atoi(value);
        }
    }

    fclose(file);
    return 0;
}

void *Alive(void *args)
{
    struct timespec sleepTime = {ALIVE_MSG_DURATION, 0};
    /*char **arguments = (char **)args;
    //int mport = atoi(arguments[2]);
    int mport = 60000;*/

    alive_thread_args *arguments = (alive_thread_args *)args;
    char *radar_ip = arguments->radar_ip;
    char *pi_ip = arguments->pi_ip;
    int radar_alive_port = arguments->radar_alive_port;

    sock_mcast = socket(AF_INET, SOCK_DGRAM, 0);
    mcast.sin_family = AF_INET;
    mcast.sin_port = htons(radar_alive_port);
    mcast.sin_addr.s_addr = inet_addr(pi_ip);
    if (bind(sock_mcast, (struct sockaddr *)&mcast, sizeof(mcast)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("\n[+]ALIVE Socket Activate");

    mcast_dest.sin_family = AF_INET;
    mcast_dest.sin_port = htons(radar_alive_port);    // Port of ALIVE Radar
    mcast_dest.sin_addr.s_addr = inet_addr(radar_ip); // IP Address of Radar

    unsigned char *bytes = (unsigned char *)&mcast.sin_addr;
    for (int ip_byte = 0; ip_byte < 4; ++ip_byte)
        start[ip_byte + 20] = bytes[ip_byte];

    uint16_t m_crc = checksum((unsigned char *)&start[12], 16, kCRC_Table);

    // Put the CRC value into start[28] and start[29]
    start[28] = (m_crc >> 8) & 0xFF; // High byte
    start[29] = m_crc & 0xFF;        // Low byte

    while (1)
    {
        sendto(sock_mcast, start, sizeof(start), 0, (struct sockaddr *)&mcast_dest, sizeof(mcast_dest));
        nanosleep(&sleepTime, NULL);
    }
}

void *udpsocket(void *args)
{
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    NetworkInfo *networkInfo = threadArgs->networkInfo;
    int radar_port = threadArgs->radar_port;
    /*
    t = time(NULL);
    struct tm *tm_info = localtime(&t);
    */
    storefile = fopen("trace.txt", "a");
    // fprintf(storefile, "\n\nLog profile of %s", asctime(tm_info));
    struct stat stre;
    int stre_size;

    initBuffer(&cb1);
    sock_raw = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock_raw == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("\n[+]UDP Socket Active");
    }

    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(radar_port);
    receiver.sin_addr.s_addr = inet_addr(networkInfo->ipAddress2);

    if (bind(sock_raw, (struct sockaddr *)&receiver, sizeof(receiver)) == -1)
    {
        perror("bind");
        close(sock_raw);
        exit(EXIT_FAILURE);
    }

    printf("\nInterface: eth1");
    printf("\b\n\t |-Source IP: %s\n", inet_ntoa(receiver.sin_addr));
    SetTime();

    while (1)
    {
        stat("trace.txt", &stre);
        stre_size = stre.st_size;

        memset(recv_buf, '\0', BUF_SIZ);
        memset(udp_checker, '\0', BUF_SIZ);

        receiver_size = sizeof(receiver);
        data_size = recvfrom(sock_raw, recv_buf, BUF_SIZ, 0, (struct sockaddr *)&receiver, &receiver_size);

        unsigned int message_type1 = (recv_buf[recv_buf[2]] << 8) | recv_buf[recv_buf[2] + 1];
        unsigned int message_type2 = (recv_buf[recv_buf[recv_buf[2] + 2] + recv_buf[2] + 3] << 8) | recv_buf[recv_buf[recv_buf[2] + 2] + recv_buf[2] + 4];

        if (((data_size >= 34 && message_type1 == 0x0785) && message_type2 == 0x0785) || (data_size >= 200 && message_type1 == 0x0780))
        {
            //            printf("\n\t[+]Message Type1: 0x%04X", message_type1);
            //            printf("\n\t[+]Message Type2: 0x%04X", message_type2);
            if (rbc >= 0xFFFFFF)
            {
                rbc = 0;
                fprintf(storefile, "\nResequence\n");
            }
            rbc++;
            len = data_size + 21;
            leng = 0;

            memset(packet, '\0', BUF_SIZ);
            packet[leng++] = 0x02;
            memcpy(&len_buf, &len, 2);
            for (a = 2; a > 0; a--)
                packet[leng + 2 - a] = len_buf[a - 1];
            leng += 2;
            packet[leng++] = dev_addr[0];
            packet[leng++] = dev_addr[1];

            memcpy(&tm_buf, &tm, 8);
            for (a = 8; a > 0; a--)
                packet[leng + 8 - a] = tm_buf[a - 1];
            leng += 8;

            packet[leng++] = 0x03;

            memcpy(&seq_buf, &rbc, 3);
            for (a = 3; a > 0; a--)
                packet[leng + 3 - a] = seq_buf[a - 1];
            leng += 3;

            packet[leng++] = 0x01;

            for (a = leng; a < (18 + data_size); a++)
                packet[a] = recv_buf[a - leng];
            leng += data_size;
            // printf("\ndata size: %d", data_size);
            memcpy(udp_checker, &packet[1], leng - 1);
            Crc_UDP = checksum(udp_checker, leng - 1, kCRC_Table);

            memcpy(&crc_ubuf, &Crc_UDP, 2);
            for (a = 2; a > 0; a--)
                packet[leng + 2 - a] = crc_ubuf[a - 1];
            leng += 2;
            packet[leng++] = 0x03;
            printf("\n\t[+]Incoming %d byte data: \n", leng);

            t = time(NULL);
            struct tm *tm_info = localtime(&t);
            fprintf(storefile, "%s: ", asctime(tm_info));

            for (a = 0; a < leng; a++)
            {
                if (a % 8 == 0)
                    printf("\t");
                if (a % 16 == 0)
                    printf("\n");
                printf("%02X ", packet[a]);
                fprintf(storefile, "%02X", packet[a]);
            }
            printf("\n");
            fprintf(storefile, "\n");

            if (isFull(&cb1) == 1)
            {
                printf("\n\t[-]Buffer is full. Enqueue operation failed --> Overwrite\n");
                dequeue(&cb1, packet);
                printf("CB count : %d", cb1.count);
            }
            enqueue(&cb1, packet);
        }

        if (stre_size >= 5000000000)
        {
            storefile = freopen("trace.txt", "r+", fopen("trace.txt", "a"));
            if (storefile == NULL)
            {
                perror("fopen");
                // return -1;
            }
        }
    }
}

void *SetTimeth(void *args)
{
    sleep(15);
    SetTime();
    pthread_exit(NULL);
}

void read_ntp_settings(const char*, NTPSettings*);
int check_ntp_sync(const char*);
time_t get_file_mod_time(const char*);

void* ntp_sync_thread(void* arg) {
    NTPSettings* settings = (NTPSettings*)arg;

    read_ntp_settings(kConfig_File, settings);  // Initial settings load
    time_t last_mod_time = get_file_mod_time(kConfig_File);  // Get initial modification time
    settings->ntp_timesync *= 60;
    
    while (1) {
        time_t current_mod_time = get_file_mod_time(kConfig_File);
        int timewait = 0;

        if (current_mod_time > last_mod_time) {
            printf("\nSettings file modified. Reloading configuration...\n");
            read_ntp_settings(kConfig_File, settings);  // Reload settings
            settings->ntp_timesync *= 60;
            last_mod_time = current_mod_time;  // Update last modification time
        }

        if (check_ntp_sync(settings->ntp_server)) {
            printf("\nNTP sync successful\n");

            if (ntp_attempt > 0) {
                if (pthread_create(&stimeth, NULL, SetTimeth, NULL) != 0)
                {
                    perror("Set time  thread creation failed");
                    exit(EXIT_FAILURE);
                }
            }
            ntp_attempt = 0;
        } else {
            printf("\nNTP sync failed\n");
            ++ntp_attempt;
        }

        if (ntp_attempt >= settings->ntp_timeout) {
            printf("\nNTP sync timeout\n");
            timewait = settings->ntp_timesync *3;
            --ntp_attempt;
            printf("Timesync = %d\n", timewait);
            sleep(timewait);
        }
        else {
            printf("Timesync = %d\n", settings->ntp_timesync);
            sleep(settings->ntp_timesync);
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    NetworkInfo *networkInfo = ExtractNetwork(kTargetInterface1, kTargetInterface2);
    ThreadArgs threadArgs;
    alive_thread_args alive_args;
    threadArgs.thread_function = udpsocket;
    threadArgs.networkInfo = networkInfo;

    uint8_t response_sender = 0;
    uint8_t response_receiver = 0;
    uint8_t reset_output1_enable = 1;
    uint8_t reset_output2_enable = 1;
    uint8_t toggle1_enable = 1;
    uint8_t toggle2_enable = 1;
    int server_port = 50002;

    threadArgs.radar_port = 55555;

    memset(alive_args.radar_ip, 0, sizeof(alive_args.radar_ip));
    memset(alive_args.pi_ip, 0, sizeof(alive_args.pi_ip));
    alive_args.radar_alive_port = 60000;

    if (parse_config(argv[1],
                     alive_args.radar_ip,
                     &threadArgs.radar_port,
                     &alive_args.radar_alive_port,
                     &server_port,
                     &response_sender,
                     &response_receiver,
                     &dev_ntimeout,
                     &reset_output1_enable,
                     &reset_output2_enable) != 0)
    {
        fprintf(stderr, "Failed to parse configuration file\n");
        pthread_exit(NULL);
    }

    strncpy(alive_args.pi_ip, argv[2], INET_ADDRSTRLEN);

    dev_addr[0] = response_sender;
    dev_addr[1] = response_receiver;

    net_tv.tv_sec = dev_ntimeout;
    net_tv.tv_usec = 0;

    int attempts = 0;
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
        attempts++;
    }

    if (fd == -1)
    {
        attempts = 0;
        while (attempts < MAX_SERIAL_PORT_ATTEMPTS)
        {
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
    }

    ctx = modbus_new_rtu(serial_port, 9600, 'N', 8, 1);
    if (ctx == NULL)
    {
        fprintf(stderr, "Unable to create MODBUS context\n");
    }
    else
    {
        modbus_set_slave(ctx, SLAVEID);
        mc = modbus_connect(ctx);
        if (mc == -1)
        {
            fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
            modbus_free(ctx);
        }
    }

    holding_registers[0] = 50;
    int rh = modbus_write_registers(ctx, REGIS_START, 1, holding_registers);

    modbus_set_indication_timeout(ctx, 1, 0);
    sock_serv = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_raw == -1)
    {
        perror("socket");
        return -1;
    }
    else
    {
        printf("\n[+]Server Socket Active");
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);                                  /* Server-side port */
    server.sin_addr.s_addr = inet_addr(networkInfo->ipAddress1);           /* Server-side address */
    if (bind(sock_serv, (struct sockaddr *)&server, sizeof(server)) == -1) /* Bind to eth1 */
    {
        perror("bind");
        close(sock_serv);
        return -1;
    }

    if (setsockopt(sock_serv, SOL_SOCKET, SO_RCVTIMEO, &net_tv, sizeof(net_tv)) < 0)
    {
        perror("\nSet socket option error!");
        close(sock_serv);
        exit(EXIT_FAILURE);
    }

    strcpy(mac.ifr_name, kTargetInterface2);
    ioctl(sock_serv, SIOCGIFHWADDR, &mac);
    getifaddrs(&ifap);
    for (ifa = ifap; ifa; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr->sa_family == AF_INET)
            sa = (struct sockaddr_in *)ifa->ifa_netmask;
    }

    NTPSettings settings;
    
    read_ntp_settings(kConfig_File, &settings);  // Initial settings load

    // pthread_t udpth, aliveth, stimeth, ntp_thread;
    if (pthread_create(&udpth, NULL, threadArgs.thread_function, (void *)&threadArgs) != 0)
    {
        perror("UDP receive thread creation failed");
        exit(EXIT_FAILURE);
    }
    usleep(2000);

    if (pthread_create(&aliveth, NULL, Alive, &alive_args) != 0)
    {
        perror("Alive thread creation failed");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&ntp_thread, NULL, ntp_sync_thread, (void*)&settings) != 0) {
        perror("Error creating thread");
        return EXIT_FAILURE;
    }

    printf("\nInterface: eth0");
    printf("\n\t |-MAC Address: %02X", (unsigned char)mac.ifr_addr.sa_data[0]);
    for (i = 1; i < 6; ++i)
        printf("-%02X", (unsigned char)mac.ifr_addr.sa_data[i]);

    printf("\b\n\t |-Source IP: %s", inet_ntoa(server.sin_addr));
    printf("\n\t |-Subnet Mask: %s", inet_ntoa(sa->sin_addr));
    printf("\n\t |-Port: %d", ntohs(server.sin_port));

    fd_set read_fds;
    int newfd, nbytes, nready;
    listen(sock_serv, 5);
    printf("\nListening...");

    while (1)
    {
        FD_ZERO(&read_fds);
        FD_SET(sock_serv, &read_fds);
        net_tv.tv_sec = dev_ntimeout;
        net_tv.tv_usec = 0;
        printf("\n%ld seconds left until network timeout", net_tv.tv_sec);
        nready = select(sock_serv + 1, &read_fds, NULL, NULL, &net_tv);
        usleep(100);

        if (nready == -1)
        {
            perror("Select error");
            exit(EXIT_FAILURE);
        }
        else if (nready == 0)
        {
            printf("\nNo new connections for %d seconds. Timing out...\n", dev_ntimeout);
            close(newfd);
            printf("\nListening...\n");
        }

        if (FD_ISSET(sock_serv, &read_fds))
        {
            addrlen = sizeof(server);
            newfd = accept(sock_serv, (struct sockaddr *)&server, &addrlen);
            printf("\n\t[+]Client accepted");

            while (1)
            {
                stimeout.tv_sec = 0;
                stimeout.tv_usec = 80000;
                gettimeofday(&tv, NULL);
                tm = ((long long)tv.tv_sec * 1000) + (tv.tv_usec / 1000);

                while (!isEmpty(&cb1))
                {
                    int connected;
                    socklen_t connected_len = sizeof(connected);
                    if (getsockopt(newfd, SOL_SOCKET, SO_ERROR, &connected, &connected_len) == -1)
                    {
                        perror("getsockopt");
                        close(newfd);
                        exit(EXIT_FAILURE);
                    }

                    if (connected != 0)
                    {
                        printf("\nConnection lost\n");
                        close(newfd);
                        break;
                    }
                    else
                    {
                        memset(backup_buffer, '\0', BUF_SIZ);
                        printf("\n\t[+]We have unsent data");
                        dequeue(&cb1, backup_buffer);
                        send(newfd, backup_buffer, backup_buffer[1] << 8 | backup_buffer[2], 0);
                        printf(" --> SENT!!\nCB count : %d\n", cb1.count);
                    }
                }

                if (setsockopt(newfd, SOL_SOCKET, SO_RCVTIMEO, &stimeout, sizeof(stimeout)) < 0)
                {
                    perror("\n[-]Error: Unable to set socket timeout");
                    break;
                }

                memset(client_buf, '\0', BUF_SIZ);
                nbytes = recv(newfd, client_buf, sizeof(client_buf), 0);
                if (nbytes > 0)
                {
                    memset(checker, '\0', BUF_SIZ);
                    memset(tcp_pack, '\0', BUF_SIZ);
                    memset(val_buf, '\0', BUF_SIZ);
                    for (i = 1; i < (nbytes - 3); i++)
                        val_buf[i - 1] = client_buf[i];

                    val = checksum(val_buf, nbytes - 4, kCRC_Table);
                    memcpy(&crc_buf, &val, 2);
                    printf("\n\t |-CRC: %02X %02X : ", crc_buf[1], crc_buf[0]);
                    for (i = 0; i < nbytes; i++)
                        printf("%02X ", client_buf[i]);

                    tcp_pack[0] = 0x02;
                    tcp_pack[1] = 0x00;
                    tcp_pack[3] = dev_addr[0];
                    tcp_pack[4] = dev_addr[1];
                    if ((crc_buf[1] == client_buf[nbytes - 3]) && (crc_buf[0] == client_buf[nbytes - 2]))
                    {
                        if (msg_counter >= 0xFFFFFF)
                            msg_counter = 0;

                        printf(" --> CRC Correct!!");
                        printf("\n\t[+]Command %02X", client_buf[13]);
                        // tcp_pack[4] = client_buf[3];
                        switch (client_buf[13])
                        {
                        case 1:
                        case 2:
                        {
                            int isDeviceInfoRequest = (client_buf[13] == 01);
                            printf(" --> %s request", isDeviceInfoRequest ? "Device info" : "Life message");
                            tcp_pack[2] = isDeviceInfoRequest ? 0x32 : 0x15;
                            b = 5;
                            memcpy(&tm_buf, &tm, 8);
                            for (i = 8; i > 0; i--)
                                tcp_pack[b + (8 - i)] = tm_buf[i - 1];
                            b += 8;
                            tcp_pack[b++] = isDeviceInfoRequest ? 0x01 : 0x02;
                            msg_counter++;
                            memcpy(&seq_buf, &msg_counter, 3);
                            for (i = 3; i > 0; i--)
                                tcp_pack[b + (3 - i)] = seq_buf[i - 1];
                            b += 3;

                            if (!isDeviceInfoRequest)
                            {
                                stimeout.tv_sec = 0;
                                stimeout.tv_usec = 80000;
                                if (setsockopt(sock_raw, SOL_SOCKET, SO_RCVTIMEO, &stimeout, sizeof(stimeout)) < 0)
                                {
                                    perror("\n[-]Error: Unable to set socket timeout");
                                    close(sock_raw);
                                    exit(EXIT_FAILURE);
                                }

                                int online = recvfrom(sock_raw, recv_buf, BUF_SIZ, 0, (struct sockaddr *)&receiver, &receiver_size);
                                if (online < 0)
                                {
                                    tcp_pack[b++] = 0x03;
                                    printf(" --> Radar Offline\n");
                                }
                                else
                                {
                                    tcp_pack[b++] = 0x02;
                                    printf(" --> Radar Online\n");
                                }
                            }
                            else
                            {
                                tcp_pack[b++] = 0x01;
                                printf("\t\t |-Version: ");
                                memcpy(&ver_buf, &kDevice_version, 2);
                                for (i = 2; i > 0; i--)
                                    tcp_pack[b + (2 - i)] = ver_buf[i - 1];
                                printf("%d ", tcp_pack[b] << 8 | tcp_pack[b + 1]);
                                b += 2;
                                memcpy(&tcp_pack[b], &mac.ifr_addr.sa_data, 6);
                                printf("\n\t\t |-MAC Address: %02X", (unsigned char)mac.ifr_addr.sa_data[0]);
                                for (i = 1; i < 6; ++i)
                                    printf("-%02X", (unsigned char)mac.ifr_addr.sa_data[i]);
                                b += 6;
                                printf("\b\n\t\t |-Source IP: %s", inet_ntoa(server.sin_addr));
                                memcpy(&tcp_pack[b], &server.sin_addr.s_addr, 4);
                                b += 4;
                                printf("\n\t\t |-Subnet Mask: %s", inet_ntoa(sa->sin_addr));
                                memcpy(&tcp_pack[b], &sa->sin_addr.s_addr, 4);
                                b += 4;
                                for (i = 0; i < 4; i++)
                                    tcp_pack[b + i] = atoi(networkInfo->gateway[i]);
                                printf("\n\t\t |-Gateway: %d.%d.%d.%d", tcp_pack[b], tcp_pack[b + 1], tcp_pack[b + 2], tcp_pack[b + 3]);
                                b += 4;
                                memcpy(&tcp_pack[b], &kDevice_DNS, 4);
                                printf("\n\t\t |-DNS: %d", tcp_pack[b + 3]);
                                for (i = b + 2; i >= b; i--)
                                    printf(".%d", tcp_pack[i]);
                                b += 4;
                                memcpy(&tcp_pack[b], &server.sin_port, 2);
                                printf("\n\t\t |-Port: %d", ntohs(server.sin_port));
                                b += 2;
                                printf("\n\t\t |-Network Timeout: %d\n", dev_ntimeout);
                                tcp_pack[b++] = dev_ntimeout;
                            }

                            memcpy(checker, &tcp_pack[1], b - 1);
                            Crc_Byte = checksum(checker, b - 1, kCRC_Table);
                            memcpy(&crc_buf, &Crc_Byte, 2);
                            for (i = 2; i > 0; i--)
                                tcp_pack[b + (2 - i)] = crc_buf[i - 1];
                            b += 2;
                            tcp_pack[b++] = 0x03;
                            send(newfd, tcp_pack, b, 0);
                            break;
                        }
                        case 4:
                            printf(" --> Input voltage monitoring request");
                            tcp_pack[2] = 0x17;
                            b = 5;
                            memcpy(&tm_buf, &tm, 8);
                            for (i = 8; i > 0; i--)
                                tcp_pack[b + 8 - i] = tm_buf[i - 1];
                            b += 8;
                            tcp_pack[b++] = 0x04;
                            msg_counter++;
                            memcpy(&seq_buf, &msg_counter, 3);
                            for (i = 3; i > 0; i--)
                                tcp_pack[b + 3 - i] = seq_buf[i - 1];
                            b += 3;
                            if (ctx == NULL || mc == -1)
                            {
                                if (ctx == NULL)
                                    printf("\n\t[-]ERROR!! --> No device connected on %s", serial_port);

                                if (mc == -1)
                                    printf("\n\t[-]ERROR!! --> Can't connect to I/O!!\n");

                                tcp_pack[b++] = 0x07;
                                tcp_pack[b++] = 0x00;
                                tcp_pack[b++] = 0x00;
                                attempts = 0;
                                while (attempts < MAX_SERIAL_PORT_ATTEMPTS)
                                {
                                    snprintf(serial_port, sizeof(serial_port), "/dev/ttyUSB%d", attempts);
                                    fd = open(serial_port, O_RDWR | O_NOCTTY);
                                    if (fd != -1)
                                    {
                                        printf("\n\t[+]Serial port %s opened successfully.", serial_port);
                                        break;
                                    }
                                    else
                                    {
                                        if (errno == ENOENT)
                                            printf("\n\t | %s does not exist. Trying the next port...", serial_port);
                                        else
                                        {
                                            perror("Error opening serial port");
                                            break;
                                        }
                                    }
                                    attempts++;
                                }

                                if (fd == -1)
                                {
                                    attempts = 0;
                                    while (attempts < MAX_SERIAL_PORT_ATTEMPTS)
                                    {
                                        snprintf(serial_port, sizeof(serial_port), "/dev/ttyACM%d", attempts);
                                        fd = open(serial_port, O_RDWR | O_NOCTTY);

                                        if (fd != -1)
                                        {
                                            printf("Serial port %s opened successfully.\n", serial_port);
                                            close(fd); // Close the file descriptor if needed
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
                                }

                                ctx = modbus_new_rtu(serial_port, 9600, 'N', 8, 1);
                                if (ctx == NULL)
                                    fprintf(stderr, "Unable to create MODBUS context\n");
                                else
                                {
                                    modbus_set_slave(ctx, SLAVEID);
                                    mc = modbus_connect(ctx);
                                    if (mc == -1)
                                    {
                                        fprintf(stderr, "\n\t[-]Connection failed: %s\n", modbus_strerror(errno));
                                        modbus_free(ctx);
                                    }
                                }
                            }

                            int ri = modbus_read_input_registers(ctx, REGIS_START, REGIS_COUNT, input_registers);
                            if (ri == -1)
                            {
                                fprintf(stderr, "\n[-]MODBUS read error: %s\n", modbus_strerror(errno));
                                tcp_pack[b++] = 0x06;
                                tcp_pack[b++] = 0x00;
                                tcp_pack[b++] = 0x00;
                                ctx = NULL;
                            }
                            else
                            {
                                printf("\n\t |-Received data from registers: ");
                                tcp_pack[b++] = 0x01;
                                float voltage = 0;
                                for (int i = 0; i < REGIS_COUNT; i++)
                                {
                                    voltage = ((float)input_registers[i] / 400) * 24;
                                    // voltage = (voltage/ 1024) * 61;
                                    printf("\t\t\nInput Voltage CH%d = %u ", i + 1, (int)voltage);
                                    tcp_pack[b++] = (int)voltage;
                                }
                            }

                            memcpy(checker, &tcp_pack[1], b - 1);
                            Crc_Byte = checksum(checker, b - 1, kCRC_Table);
                            memcpy(&crc_buf, &Crc_Byte, 2);
                            for (i = 2; i > 0; i--)
                                tcp_pack[b + 2 - i] = crc_buf[i - 1];
                            b += 2;
                            tcp_pack[b++] = 0x03;
                            send(newfd, tcp_pack, b, 0);
                            break;
                        case 5:
                            printf(" --> Power reset request");
                            tcp_pack[2] = 0x16;
                            b = 5;
                            memcpy(&tm_buf, &tm, 8);
                            for (i = 8; i > 0; i--)
                                tcp_pack[b + 8 - i] = tm_buf[i - 1];
                            b += 8;
                            tcp_pack[b++] = 0x05;
                            msg_counter++;
                            memcpy(&seq_buf, &msg_counter, 3);
                            for (i = 3; i > 0; i--)
                                tcp_pack[b + 3 - i] = seq_buf[i - 1];
                            b += 3;

                            if (ctx == NULL || mc == -1)
                            {
                                if (ctx == NULL)
                                    printf("\n\t[-]ERROR!! --> No device connected on /dev/ttyUSB");

                                if (mc == -1)
                                    printf("\n\t[-]ERROR!! --> Can't connect to I/O!!\n");

                                tcp_pack[b++] = 0x07;
                                tcp_pack[b++] = 0x00;

                                attempts = 0;
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
                                            printf("%s does not exist. Trying the next port...\n", serial_port);
                                        else
                                        {
                                            perror("Error opening serial port");
                                            break;
                                        }
                                    }
                                    attempts++;
                                }

                                if (fd == -1)
                                {
                                    attempts = 0;
                                    while (attempts < MAX_SERIAL_PORT_ATTEMPTS)
                                    {
                                        snprintf(serial_port, sizeof(serial_port), "/dev/ttyACM%d", attempts);
                                        fd = open(serial_port, O_RDWR | O_NOCTTY);

                                        if (fd != -1)
                                        {
                                            printf("Serial port %s opened successfully.\n", serial_port);
                                            close(fd); // Close the file descriptor if needed
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
                                }

                                ctx = modbus_new_rtu(serial_port, 9600, 'N', 8, 1);
                                if (ctx == NULL)
                                {
                                    fprintf(stderr, "Unable to create MODBUS context\n");
                                }
                                else
                                {
                                    modbus_set_slave(ctx, SLAVEID);
                                    mc = modbus_connect(ctx);
                                    if (mc == -1)
                                    {
                                        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
                                        modbus_free(ctx);
                                    }
                                    modbus_set_indication_timeout(ctx, 1, 0);
                                }
                            }

                            FILE *file = fopen("/root/RVD_APP/config.txt", "r");
                            if (!file)
                            {
                                perror("Unable to open configuration filee");
                                return -1;
                            }

                            char line[MAX_LINE_LENGTH];

                            while (fgets(line, sizeof(line), file))
                            {
                                char *key = strtok(line, "=");
                                char *value = strtok(NULL, "\n");

                                if (key && value)
                                {
                                    if (strcmp(key, "RESET_OUTPUT1_ENABLE") == 0)
                                        reset_output1_enable = atoi(value);
                                    else if (strcmp(key, "RESET_OUTPUT2_ENABLE") == 0)
                                        reset_output2_enable = atoi(value);
                                    else if (strcmp(key, "TOGGLE1_ENABLE") == 0)
                                        toggle1_enable = atoi(value);
                                    else if (strcmp(key, "TOGGLE2_ENABLE") == 0)
                                        toggle2_enable = atoi(value);
                                }
                            }

                            fclose(file);
                            coils[0] = reset_output1_enable;
                            coils[1] = reset_output2_enable;
                            coils[2] = toggle1_enable;
                            coils[3] = toggle2_enable;
                            int rcw = modbus_write_bits(ctx, REGIS_START, COILS_REGIS, coils);
                            if (rcw == -1)
                            {
                                fprintf(stderr, "\n[-]MODBUS Write error: %s\n", modbus_strerror(errno));
                                tcp_pack[b++] = 0x06;
                                tcp_pack[b++] = 0x00;
                                ctx = NULL;
                            }
                            else
                            {
                                tcp_pack[b++] = 0x01;
                                printf("\n\t |-Write data to coils: ");
                                for (int i = 0; i < REGIS_COUNT; i++)
                                    printf("Write Status %d :%u ", i + 1, coils[i]);

                                tcp_pack[b++] = coils[1] << 1 | coils[0];
                            }

                            memcpy(checker, &tcp_pack[1], b - 1);
                            Crc_Byte = checksum(checker, b - 1, kCRC_Table);
                            memcpy(&crc_buf, &Crc_Byte, 2);
                            for (i = 2; i > 0; i--)
                                tcp_pack[b + (2 - i)] = crc_buf[i - 1];
                            b += 2;
                            tcp_pack[b++] = 0x03;
                            send(newfd, tcp_pack, b, 0);

                            if (pthread_create(&stimeth, NULL, SetTimeth, NULL) != 0)
                            {
                                perror("Set time  thread creation failed");
                                exit(EXIT_FAILURE);
                            }
                            break;
                        default:
                            printf(" --> Command Unmatched??\n");
                            tcp_pack[2] = 0x15;
                            b = 5;
                            memcpy(&tm_buf, &tm, 8);
                            for (i = 8; i > 0; i--)
                                tcp_pack[b + 8 - i] = tm_buf[i - 1];
                            b += 8;

                            tcp_pack[b++] = client_buf[13];
                            msg_counter++;
                            memcpy(&seq_buf, &msg_counter, 3);
                            for (i = 3; i > 0; i--)
                                tcp_pack[b + 3 - i] = seq_buf[i - 1];
                            b += 3;

                            tcp_pack[b++] = 0x08;

                            memcpy(checker, &tcp_pack[1], b - 1);
                            Crc_Byte = checksum(checker, b - 1, kCRC_Table);
                            memcpy(&crc_buf, &Crc_Byte, 2);
                            for (i = 2; i > 0; i--)
                                tcp_pack[b + (2 - i)] = crc_buf[i - 1];
                            b += 2;
                            tcp_pack[b++] = 0x03;
                            send(newfd, tcp_pack, b, 0);
                            break;
                        }
                    }
                    else
                    {
                        printf(" --> [-]CRC Invalid!!\n\n");
                        tcp_pack[2] = 0x15;
                        b = 5;
                        memcpy(&tm_buf, &tm, 8);
                        for (i = 8; i > 0; i--)
                            tcp_pack[b + (8 - i)] = tm_buf[i - 1];
                        b += 8;
                        tcp_pack[b++] = 0x02;
                        memcpy(&seq_buf, &msg_counter, 3);
                        for (i = 3; i > 0; i--)
                            tcp_pack[b + (3 - i)] = seq_buf[i - 1];
                        b += 3;
                        tcp_pack[b++] = 0x05;
                        memcpy(checker, &tcp_pack[1], b - 1);
                        Crc_Byte = checksum(checker, b - 1, kCRC_Table);
                        memcpy(&crc_buf, &Crc_Byte, 2);
                        for (i = 2; i > 0; i--)
                            tcp_pack[b + (2 - i)] = crc_buf[i - 1];
                        b += 2;
                        tcp_pack[b++] = 0x03;
                        send(newfd, tcp_pack, b, 0);
                        break;
                    }
                }
                else if (nbytes == 0)
                    break;
            }
        }
    }
    close(sock_serv);
    return 0;
}

/*---------------------------------FUNCTIONS------------------------------------*/
uint16_t checksum(unsigned char *data, int length, const uint16_t *table)
{
    uint16_t crc = CRC16_INIT;

    for (size_t c = 0; c < length; c++)
        crc = (crc << 8) ^ table[((crc >> 8) ^ data[c]) & 0xFF];

    return crc;
}

void SetTime()
{
    unsigned char time_conf[51] = {0};
    int d, e = 0;

    printf("\nMessage Action:2000 (Set UNIX Time)");
    ;
    for (d = 0; d < sizeof(kHeader); d++)
        time_conf[e + d] = kHeader[d];
    e += sizeof(kHeader);

    for (d = 0; d < sizeof(kMsgAct); d++)
        time_conf[e + d] = kMsgAct[d];
    e += sizeof(kMsgAct);

    time_conf[e++] = 0x00;
    time_conf[e++] = 0x04;
    time_conf[e++] = 0xFF;
    time_conf[e++] = 0x01;
    e += 2;

    for (d = 0; d < sizeof(kMsgAct); d++)
        time_conf[e + d] = kMsgAct[d];
    e += sizeof(kMsgAct);

    time_conf[e++] = 0x01;
    time_conf[e++] = 0x00;
    time_conf[e++] = 0x5B;
    time_conf[e++] = 0x01;
    time_conf[e++] = 0x00;
    time_conf[e++] = 0x00;

    for (d = 0; d < sizeof(kMsgAct); d++)
        time_conf[e + d] = kMsgAct[d];
    e += sizeof(kMsgAct);

    time_conf[e++] = 0x02;
    time_conf[e++] = 0x00;

    t = time(NULL);
    memcpy(&time_conf[e], &t, 4);
    e += 4;
    printf("\nUNIX Time (second)\n\tIn decimal: %ld s\n\tIn hexadecimal: %02X %02X %02X %02X s\n", t, time_conf[48], time_conf[47], time_conf[46], time_conf[45]);

    for (d = 0; d < 6; d++)
        dummy1[d] = time_conf[sizeof(kHeader) + d + 3];

    for (d = 6; d < 14; d++)
        dummy1[d] = time_conf[sizeof(kHeader) + d + 8];

    for (d = 14; d < 22; d++)
        dummy1[d] = time_conf[sizeof(kHeader) + d + 11];

    memset(Tchecker, '\0', BUF_SIZ);
    memcpy(Tchecker, &dummy1[0], sizeof(dummy1));
    Crc_Time = checksum(Tchecker, sizeof(dummy1), kCRC_Table);
    memcpy(&crc_tbuf, &Crc_Time, 2);

    time_conf[25] = crc_tbuf[0];
    time_conf[26] = crc_tbuf[1];

    for (d = 0; d < 33; d++)
        dummy2[d] = time_conf[sizeof(kHeader) + d];

    memset(Tchecker, '\0', BUF_SIZ);
    memcpy(Tchecker, &dummy2[0], sizeof(dummy2));
    Crc_Time = checksum(Tchecker, sizeof(dummy2), kCRC_Table);
    memcpy(&crc_tbuf, &Crc_Time, 2);

    time_conf[49] = crc_tbuf[1];
    time_conf[50] = crc_tbuf[0];

    printf("Message TX\n");
    for (d = 0; d < sizeof(time_conf); d++)
    {
        printf("%02X ", time_conf[d]);
        if ((d == 15) || (d == 26) || (d == 37))
            printf("\n");
    }
    sendto(sock_raw, time_conf, sizeof(time_conf), 0, (struct sockaddr *)&receiver, sizeof(receiver));
}

void initBuffer(CircularBufferMatrix *cb)
{
    cb->front = 0;
    cb->rear = -1;
    cb->count = 0;
}

int isFull(CircularBufferMatrix *cb)
{
    return (cb->count == MAX_QUEUE);
}

int isEmpty(CircularBufferMatrix *cb)
{
    return (cb->count == 0);
}

void enqueue(CircularBufferMatrix *cb, unsigned char matrix[BUF_SIZ])
{
    if (isFull(cb))
    {
        printf("\n\t[-]Buffer is full. Enqueue operation failed\n\n");
    }
    else
    {
        cb->rear = (cb->rear + 1) % MAX_QUEUE;

        for (int f = 0; f < ((matrix[1] << 8) | matrix[2]); f++)
            cb->qbuffer[cb->rear][f] = matrix[f];
        cb->count++;
    }
    printf("\nCB count : %d\n", cb1.count);
}

void dequeue(CircularBufferMatrix *cb, unsigned char matrix[BUF_SIZ])
{
    if (!isEmpty(cb))
    {
        for (int g = 0; g < ((cb->qbuffer[cb->front][1] << 8) | cb->qbuffer[cb->front][2]); g++)
            matrix[g] = cb->qbuffer[cb->front][g];

        cb->front = (cb->front + 1) % MAX_QUEUE;
        cb->count--;
    }
    else
    {
        printf("\n\t[-]Buffer is empty. Dequeue operation failed ");
    }
}

// Function to extract the IP address from ifconfig output
NetworkInfo *ExtractNetwork(const char *interfaceName1, const char *interfaceName2)
{
    const char *keyword = "inet ";
    char *token, word[BUF_SIZ], *interfaceStart, *ipStart, *ipEnd, *output;
    NetworkInfo *networkInfo = NULL;
    FILE *pfp, *gfp;

    // Run "ifconfig" to get network information
    pfp = popen("ifconfig", "r");
    if (pfp == NULL)
    {
        perror("popen");
        return NULL;
    }
    fread(word, 1, sizeof(word), pfp);
    pclose(pfp);

    int ipLength;
    idx = 0;
    networkInfo = (NetworkInfo *)malloc(sizeof(NetworkInfo));
    if (strstr(word, interfaceName1))
    {
        interfaceStart = strstr(word, interfaceName1);
        ipStart = strstr(interfaceStart, keyword);

        if (ipStart)
        {
            ipStart += strlen(keyword);
            ipEnd = strchr(ipStart, ' ');

            if (ipEnd)
            {
                ipLength = ipEnd - ipStart;
                if (networkInfo)
                {
                    networkInfo->ipAddress1 = (char *)malloc(ipLength + 1);
                    strncpy(networkInfo->ipAddress1, ipStart, ipLength);
                    networkInfo->ipAddress1[ipLength] = '\0';
                }
            }
        }
    }

    if (strstr(word, interfaceName2))
    {
        interfaceStart = strstr(word, interfaceName2);
        ipStart = strstr(interfaceStart, keyword);

        if (ipStart)
        {
            ipStart += strlen(keyword);
            ipEnd = strchr(ipStart, ' ');

            if (ipEnd)
            {
                ipLength = ipEnd - ipStart;
                if (networkInfo)
                {
                    networkInfo->ipAddress2 = (char *)malloc(ipLength + 1);
                    strncpy(networkInfo->ipAddress2, ipStart, ipLength);
                    networkInfo->ipAddress2[ipLength] = '\0';
                }
            }
        }
    }

    // Run "ip route" to get default gateway
    gfp = popen("ip route", "r");
    if (gfp == NULL)
    {
        perror("popen");
        return NULL;
    }
    memset(word, '\0', BUF_SIZ);
    fread(word, 1, BUF_SIZ, gfp);
    pclose(gfp);

    if (networkInfo)
    {
        token = strtok(word, " ");

        while (token != NULL)
        {
            if (strcmp(token, "via") == 0)
            {
                token = strtok(NULL, " ");
                output = strdup(token); // Store the default gateway
                token = strtok(output, ".");

                do
                {
                    networkInfo->gateway[idx++] = token;
                } while (token = strtok(NULL, "."));
                break;
            }
            token = strtok(NULL, " ");
        }
    }
    return networkInfo;
}

// Thread function for NTP synchronization
void read_ntp_settings(const char* filename, NTPSettings* settings) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening config file");
        exit(EXIT_FAILURE);
    }

    char line[100];
    memset(line, 0, sizeof(line));

    while (fgets(line, sizeof(line), file) != NULL) {
        if (strncmp(line, "ntp_server", 10) == 0) {
            sscanf(line, "ntp_server = %s", settings->ntp_server);
        } else if (strncmp(line, "ntp_timesync", 12) == 0) {
            sscanf(line, "ntp_timesync = %d", &settings->ntp_timesync);
        } else if (strncmp(line, "ntp_timeout", 11) == 0) {
            sscanf(line, "ntp_timeout = %d", &settings->ntp_timeout);
        } //else if (strncmp(line, "ntp_timesync_wait", 17) == 0) {
        //     sscanf(line, "ntp_timesync_wait = %d", &settings->ntp_timesync_wait);
        // }

        memset(line, 0, sizeof(line)); // Clear line for next iteration
    }

    fclose(file);
}

int check_ntp_sync(const char* ntp_server) {
    char command[100];
    memset(command, 0, sizeof(command));

    snprintf(command, sizeof(command), "ntpdate -q %s", ntp_server);

    FILE* fp = popen(command, "r");
    if (fp == NULL) {
        perror("Error executing ntpdate");
        return 0;
    }

    char output[200];
    memset(output, 0, sizeof(output));

    int sync_success = 0;
    while (fgets(output, sizeof(output), fp) != NULL) {
        if (strstr(output, "adjust time") != NULL) {
            sync_success = 1; // Success if "adjust time" is in the output
        }
        memset(output, 0, sizeof(output)); // Clear output for next iteration
    }

    pclose(fp);
    return sync_success;
}

time_t get_file_mod_time(const char* filename) {
    struct stat file_stat;
    if (stat(filename, &file_stat) == 0) {
        return file_stat.st_mtime;
    }
    return 0; // Return 0 if the file doesn't exist or can't be accessed
}
