import socket
#import crcmod
import time
import random
from collections import defaultdict
import numpy as np
import datetime

table = [ 
        0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
        0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6, 0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
        0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485, 0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
        0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4, 0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
        0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823, 0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
        0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12, 0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
        0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41, 0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
        0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70, 0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
        0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F, 0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
        0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E, 0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
        0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D, 0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
        0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C, 0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
        0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB, 0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
        0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A, 0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
        0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9, 0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
        0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8, 0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
    ]
classes = [1, 2, 3, 4, 6, 7, 8]
stat_hdr = ["7e0112012d0400000009b4dd0003566d235d", "7e0112012d0400000009b4e20003566d4a4a", "7e0112012d0400000009b4e70003566d691d"]
info_part_0_full = "0780040c093f10"

def calculate_crc(data: bytes):
    crc = 0xFFFF
    for byte in data:
        crc = (crc << 8) ^ table[(crc >> 8) ^ byte]
        crc &= 0xFFFF

    return crc

def gettime():
    current_time_seconds = int(time.time())
    hex_time = format(current_time_seconds, '08x')
    rearranged_hex_time = hex_time[6:] + hex_time[4:6] + hex_time[2:4] + hex_time[:2]
    rearranged_hex_time_milli = rearranged_hex_time + "0000"

    return rearranged_hex_time_milli

class PvrStatistics:
    def __init__(self):
        self.average_speed_data = defaultdict(lambda: defaultdict(list))
        self.percentile_85_speed_data = defaultdict(lambda: defaultdict(list))
        self.volume_data = defaultdict(lambda: defaultdict(int))
        self.vehicle_length_data = defaultdict(lambda: defaultdict(list))

    def pvr_maker(self, transaction, count, numzone):
        self.random_class = random.choice(classes)
        self.obj_count_bin = format(count, '06b').zfill(6)
        self.obj_id_bin = format(transaction, '08b').zfill(8)
        self.zone = format(random.randint(0, numzone), '05b').zfill(5)
        self.true_zone = int(self.zone, 2)
        self.class_ = format(self.random_class, '04b').zfill(4)
        self.speed = format(random.randint(240, 1000), '12b').zfill(12)
        self.true_speed = int(self.speed, 2)
        self.heading = "000000001110"

        self.car_length = format(
            random.randint(5, 7) if self.random_class == 1 else
            random.randint(8, 12) if self.random_class == 2 else
            random.randint(13, 22) if self.random_class == 3 else
            random.randint(23, 27) if self.random_class == 4 else
            random.randint(28, 44) if self.random_class == 6 else
            random.randint(45, 69) if self.random_class == 7 else
            int("1000110", 2), '07b').zfill(7)

        self.true_length = int(self.car_length, 2) * 0.2
        self.add_data(self.true_zone, self.true_speed, self.random_class, self.true_length)

        self.pvr_full_bin = self.car_length + self.heading + self.speed + self.class_ + self.zone + self.obj_id_bin + self.obj_count_bin + "1"
        self.pvr_full_bin = self.pvr_full_bin.replace(" ", "0").zfill(56)

        self.pvr_full_hex = format(int(self.pvr_full_bin, 2), '014x')
        self.rearranged_pvr_full_hex =  self.pvr_full_hex[12:] + self.pvr_full_hex[10:12] + self.pvr_full_hex[8:10] + self.pvr_full_hex[6:8] + self.pvr_full_hex[4:6] + self.pvr_full_hex[2:4] + self.pvr_full_hex[:2]

        return self.rearranged_pvr_full_hex

    def add_data(self, zone, true_speed, classes, length):
        self.average_speed_data[zone][classes].append(true_speed)
        self.volume_data[zone][classes] += 1
        self.vehicle_length_data[zone][classes].append(length)
        
        #print(f"speed = {self.average_speed_data[zone][classes]}")
        #print(f"Volume class/zone {classes}/{zone} = {self.volume_data[zone][classes]}")
        #print(f"length = {self.vehicle_length_data[zone][classes]}")
        
    def calculate_statistics(self, zones, classes):
        self.result = {}
        self.avg_speed = 0
        self.spd_85th = 0
        self.total_count = 0
        self.percent_occ = 0
        
        if zones in self.average_speed_data and classes in self.average_speed_data[zones]:
            self.detection_zone_length = 30
            self.interval_time = 300
        
            #for zone in self.average_speed_data:
            self.result[zones] = {}
            self.avg_speed = round((np.mean(self.average_speed_data[zones][classes]) * 0.025), 2)
            self.spd_85th = round((np.percentile(self.average_speed_data[zones][classes], 85) * 0.025), 2)
            self.total_count = self.volume_data[zones][classes]
            self.percent_occ = round((100 * (((self.detection_zone_length + np.mean(self.vehicle_length_data[zones][classes])) / self.avg_speed) * self.total_count) / self.interval_time), 1)
            
            #print(f"report = {total_count}, {avg_speed}, {percent_occ}, {spd_85th}")
            
            self.result[zones][classes] = {
                'average_speed': self.avg_speed,
                'percentile_85_speed': self.spd_85th,
                'total_count': self.total_count,
                'occupancy': self.percent_occ,
            }
            
        return self.total_count, self.avg_speed, self.percent_occ, self.spd_85th

    def statistic_report(self):
        self.stat_interval_time_bin = format(300, '016b').zfill(16)
        self.Serial_Num = format(218733, '028b').zfill(28)
        self.msg_782_bin = self.Serial_Num + self.stat_interval_time_bin + "0100"
        self.msg_782_bin = self.msg_782_bin.zfill(64)
        self.msg_782_hex = format(int(self.msg_782_bin, 2), '016x').zfill(16)
        self.rearranged_msg_782_hex = self.msg_782_hex[14:] + self.msg_782_hex[12:14] + self.msg_782_hex[10:12] + self.msg_782_hex[8:10] + self.msg_782_hex[6:8] + self.msg_782_hex[4:6] + self.msg_782_hex[2:4] + self.msg_782_hex[:2]
        self.msg_782_full = "078208" + self.rearranged_msg_782_hex

        self.msg_781_num0 = ["0781050000e000000781050040e0000007810500c0e00000078105000001c304078105004001b9040781050080000000",
                        "0781050802e000000781050842e0000007810508c2e000000781050802017c03078105084201bd040781050882000000",
                        "0781051004e000000781051044e0000007810510c4e00000078105100401a4090781051044019a090781051084000000"]

        self.msg_781_full = [""] * 3
        self.temp_msg_bin = [""] * 8
        for zone_num in range(3):
            self.msg_781_without_hdr = [""] * 3  # Use a list of lists instead of strings
            self.msg_781_without_hdr_with_crc = [""] * 3
            self.counter = 0
            self.count_num = 0

            for msg_num in range(8):
                self.utc_timestamp_milliseconds = int(self.time.time() * 1000)
                self.last_three_digits = format(self.utc_timestamp_milliseconds % 1000, '010b').zfill(10)
                self.info_part_1_bin = format(int(self.utc_timestamp_milliseconds/1000), '032b') + "00000000000000000000" + self.last_three_digits + "01"
                self.info_part_1_hex = format(int(self.info_part_1_bin, 2), '016x').zfill(16)
                self.rearranged_info_part_1_hex = self.info_part_1_hex[14:] + self.info_part_1_hex[12:14] + self.info_part_1_hex[10:12] + self.info_part_1_hex[8:10] + self.info_part_1_hex[6:8] + self.info_part_1_hex[4:6] + self.info_part_1_hex[2:4] + self.info_part_1_hex[:2]
                self.rearranged_info_part_1_hex = self.rearranged_info_part_1_hex.zfill(8)
                self.info_part_1_full = "078008" + self.rearranged_info_part_1_hex

                class_num = msg_num
        
                if msg_num >= 5:
                    class_num += 1
                if zone_num == 1:
                    count_num = msg_num + 8
                elif zone_num == 2:
                    count_num = msg_num + 16

                self.msg_num_bin = format(count_num, '09b').zfill(9)
                self.zone_num_bin = format(zone_num, '05b').zfill(5)

                if class_num != 0:
                    self.results = self.calculate_statistics(zone_num, class_num)
                    print(f"zone:{zone_num} class: {class_num} (speed:{self.results[1]} m/s, 85th_speed:{self.results[3]} m/s, count:{self.results[0]}, %Occ:{self.results[2]}%)")
                    
                    self.volume_bin = format(self.results[0], '016b').zfill(16) + "000" + format(class_num, '04b').zfill(4) + "000" + self.zone_num_bin + self.msg_num_bin
                    self.AvgSpeed_bin = format(int(self.results[1]/0.01), '016b').zfill(16) + "000" + format(class_num, '04b').zfill(4) + "001" + self.zone_num_bin + self.msg_num_bin
                    self.Occ_bin = format(int(self.results[2]/0.05), '016b').zfill(16) + "000" + format(class_num, '04b').zfill(4) + "011" + self.zone_num_bin + self.msg_num_bin
                    self.Percentile_85th_bin = format(int(self.results[3]/0.01), '016b').zfill(16) + "000" + format(class_num, '04b').zfill(4) + "010" + self.zone_num_bin + self.msg_num_bin

                    self.volume_hex = format(int(self.volume_bin, 2), '10x')
                    self.AvgSpeed_hex = format(int(self.AvgSpeed_bin, 2), '10x')
                    self.Occ_hex = format(int(self.Occ_bin, 2), '10x')
                    self.Percentile_85th_hex = format(int(self.Percentile_85th_bin, 2), '10x')

                    self.volume_hex = self.volume_hex.replace(" ", "0")
                    self.AvgSpeed_hex= self.AvgSpeed_hex.replace(" ", "0")
                    self.Occ_hex = self.Occ_hex.replace(" ", "0")
                    self.Percentile_85th_hex = self.Percentile_85th_hex.replace(" ", "0")

                    self.rearranged_volume_hex = self.volume_hex[8:] + self.volume_hex[6:8] + self.volume_hex[4:6] + self.volume_hex[2:4] + self.volume_hex[:2]
                    self.rearranged_AvgSpeed_hex = self.AvgSpeed_hex[8:] + self.AvgSpeed_hex[6:8] + self.AvgSpeed_hex[4:6] + self.AvgSpeed_hex[2:4] + self.AvgSpeed_hex[:2]
                    self.rearranged_Occ_hex = self.Occ_hex[8:] + self.Occ_hex[6:8] + self.Occ_hex[4:6] + self.Occ_hex[2:4] + self.Occ_hex[:2]               
                    self.rearranged_Percentile_85th_hex = self.Percentile_85th_hex[8:] + self.Percentile_85th_hex[6:8] + self.Percentile_85th_hex[4:6] + self.Percentile_85th_hex[2:4] + self.Percentile_85th_hex[:2]
                    
                    self.rearranged_volume_hex = self.rearranged_volume_hex.replace(" ", "0")
                    self.rearranged_AvgSpeed_hex= self.rearranged_AvgSpeed_hex.replace(" ", "0")
                    self.rearranged_Occ_hex = self.rearranged_Occ_hex.replace(" ", "0")
                    self.rearranged_Percentile_85th_hex = self.rearranged_Percentile_85th_hex.replace(" ", "0")

                    self.temp_msg_bin[counter] = "078105" + self.rearranged_volume_hex + "078105" + self.rearranged_AvgSpeed_hex + "078105" + self.rearranged_Occ_hex + "078105" + self.rearranged_Percentile_85th_hex
                    counter += 1
                
            self.temp_msg_str = ''.join(self.temp_msg_bin)
            self.msg_781_without_hdr[zone_num] = self.info_part_0_full + self.info_part_1_full + self.msg_782_full + self.msg_781_num0[zone_num] + self.temp_msg_str

            self.byte_msg_781 = bytes.fromhex("".join(self.msg_781_without_hdr[zone_num]))
            self.crc_781 = calculate_crc(self.byte_msg_781)
                
            self.msg_781_without_hdr_with_crc[zone_num] = self.byte_msg_781 + self.crc_781.to_bytes(2, 'big')
            self.msg_781_full[zone_num] = self.stat_hdr[zone_num] + self.msg_781_without_hdr_with_crc[zone_num].hex()
    
        return self.msg_781_full
    
    def reset_data(self):
        self.average_speed_data = defaultdict(lambda: defaultdict(list))
        self.percentile_85_speed_data = defaultdict(lambda: defaultdict(list))
        self.volume_data = defaultdict(lambda: defaultdict(int))
        self.vehicle_length_data = defaultdict(lambda: defaultdict(list))

    def send_udp_message(self, host, port, no_car, no_zone):
        pvr_header = "078507"
        car = ["02", "04"]
        time = gettime()
        num_zone = no_zone - 1

        if no_car == 0:
            pvr2_msg = self.pvr_maker(1, 1, num_zone)
            pvr_msg = pvr_header + car[no_car] + time + pvr_header + pvr2_msg
        else:
            pvr2_msg1 = self.pvr_maker(1, 2, num_zone)
            pvr2_msg2 = self.pvr_maker(2, 2, num_zone)
            pvr_msg = pvr_header + car[no_car] + time + pvr_header + pvr2_msg1 + pvr_header + pvr2_msg2
        byte_pvr_msg = bytes.fromhex(pvr_msg)
        msg_len = len(pvr_msg)/2
        hex_msg_len = hex(int(msg_len))[2:]
        to_str = str(hex_msg_len)
        pvr_crc = calculate_crc(byte_pvr_msg)
        pvr_with_crc = byte_pvr_msg + pvr_crc.to_bytes(2, 'big')

        hex_msg = "7e010c00" + to_str + "0400000000"
        byte_message = bytes.fromhex(hex_msg)
        crc = calculate_crc(byte_message)
        header_msg_with_crc = byte_message + crc.to_bytes(2, 'big')
        full_msg = header_msg_with_crc + pvr_with_crc
        
        #print(f"Extended Hex Message: {full_msg.hex()}")

        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            #sock.bind(("192.168.11.55", 55555))
            sock.sendto(full_msg, (host, port))

udp_host = '192.168.11.8'
udp_port = 55555
pvr_data = PvrStatistics()
no_zone = 3

while True:
    random_no_car = random.randint(0, 1)
    
    pvr_data.send_udp_message(udp_host, udp_port, 0, no_zone=no_zone)
    random_sleep = random.uniform(0.1,1.0)

    current_time = datetime.datetime.now()
    current_minute = current_time.minute
    current_second = current_time.second

    if (current_minute % 5 == 0 or current_minute == 0) and current_second == 0:
        summary = pvr_data.statistic_report()

        for messages in range(no_zone):
            print(f"Report {messages} : {summary[messages]}")
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
                #sock.bind(("192.168.11.55", 55555))
                sock.sendto(bytes.fromhex(summary[messages]), (udp_host, udp_port))
            time.sleep(0.1)
        pvr_data.reset_data()
        time.sleep(1)
        
    time.sleep(random_sleep)