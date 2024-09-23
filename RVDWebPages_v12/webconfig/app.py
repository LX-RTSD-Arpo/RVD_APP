import os
from flask import Flask, jsonify, request, send_from_directory
import configparser

app = Flask(__name__)

# กำหนด path สำหรับ config.ini ที่อยู่ในโฟลเดอร์เดียวกับ app.py
config_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'config.ini')
eth0_path = '/etc/network/interfaces.d/eth0'
eth1_path = '/etc/network/interfaces.d/eth1'

# สร้างตัวแปรสำหรับเก็บค่า config
#config = configparser.ConfigParser()
#config.read(config_path)

def read_file(file_path):
    try:
        with open(file_path, 'r') as file:
            content = file.readlines()
        return content
    except FileNotFoundError:
        return []
    except PermissionError:
        return []
    except Exception as e:
        return []

def parse_eth_config(lines):
    config = {}
    config['host'] = "RVDAPO"
    config['primary_dns'] = "0.0.0.0"
    config['secondary_dns'] = "0.0.0.0"

    for line in lines:
        line = line.strip()
        if line.startswith('address'):
            config['ip'] = line.split()[1]
        elif line.startswith('netmask'):
            config['subnet_mask'] = line.split()[1]
        elif line.startswith('gateway'):
            config['gateway'] = line.split()[1]
    return config

"""def read_network_settings():
    print("Reading network settings from config.ini")
    network_settings = {}
    try:
        network_settings['host'] = config['network']['host']
        network_settings['ip'] = config['network']['ip']
        network_settings['gateway'] = config['network']['gateway']
        network_settings['subnet_mask'] = config['network']['subnet_mask']
        network_settings['primary_dns'] = config['network']['primary_dns']
        network_settings['secondary_dns'] = config['network']['secondary_dns']
        #network_settings['firmware_version'] = config['deviceinfo']['firmware_version']
    except KeyError as e:
        print(f"Error: Missing key {e} in 'network' section.")
        raise
    return network_settings"""

"""def write_network_settings(host_address, ip_address, gateway, subnet_mask, primary_dns, secondary_dns):
    print(f"Writing network settings to config.ini: Host={host_address} IP={ip_address}, Gateway={gateway}, Subnet Mask={subnet_mask}, Primary DNS={primary_dns}, Secondary DNS={secondary_dns}")
    if 'network' not in config:
        config['network'] = {}
    config['network']['host'] = host_address
    config['network']['ip'] = ip_address
    config['network']['gateway'] = gateway
    config['network']['subnet_mask'] = subnet_mask
    config['network']['primary_dns'] = primary_dns
    config['network']['secondary_dns'] = secondary_dns
    # config['network']['firmware_version'] = firmware_version
    with open(config_path, 'w') as configfile:
        config.write(configfile)
    print("Network settings written successfully.")"""

@app.route('/get-network-settings', methods=['GET'])
def get_network_settings():
    print("GET request received for network settings.")
    eth0_path = '/etc/network/interfaces.d/eth0'
    eth1_path = '/etc/network/interfaces.d/eth1'

    try:
        eth0_lines = read_file(eth0_path)
        eth1_lines = read_file(eth1_path)

        eth0_config = parse_eth_config(eth0_lines)
        eth1_config = parse_eth_config(eth1_lines)
        #settings = read_network_settings()
        print(f"Network settings retrieved: {eth0_config}")
        return jsonify(eth0_config)
    except Exception as e:
        print(f"Error retrieving network settings: {e}")
        return jsonify(error=str(e)), 500

@app.route('/set-network-settings', methods=['POST'])
def set_network_settings():
    print("POST request received to set network settings.")
    data = request.get_json()
    host_address = data.get('host')
    ip_address = data.get('ip')
    gateway = data.get('gateway')
    subnet_mask = data.get('subnet_mask')
    primary_dns = data.get('primary_dns')
    secondary_dns = data.get('secondary_dns')
    # firmware_version = data.get('firmware_version')
    print(f"Setting network settings to: Host={host_address} IP={ip_address}, Gateway={gateway}, Subnet Mask={subnet_mask}, Primary DNS={primary_dns}, Secondary DNS={secondary_dns}")
    try:
        write_network_settings(host_address, ip_address, gateway, subnet_mask, primary_dns, secondary_dns)
        return jsonify(success=True)
    except Exception as e:
        print(f"Error setting network settings: {e}")
        return jsonify(error=str(e)), 500

@app.route('/')
def index():
    print("Serving index.html")
    return send_from_directory('static', 'index.html')

if __name__ == '__main__':
    port = config.getint('settings', 'port', fallback=5000)
    print(f"Starting the Flask server on port {port}...")
    app.run(debug=True, port=port)
