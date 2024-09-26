import os
from flask import Flask, jsonify, request, send_from_directory
import configparser
import subprocess

app = Flask(__name__)

web_config_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'web_config.ini')
eth0_path = '/etc/network/interfaces.d/eth0'
eth1_path = '/etc/network/interfaces.d/eth1'
br0_path = '/etc/network/interfaces.d/br0'
rvd_config_path = '/root/RVD_APP/config.txt'

firmware_path = '/root/RVD_APP/sources'
app.config['firmware_path'] = firmware_path

web_config = configparser.ConfigParser()
web_config.read(web_config_path)

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

def parse_eth_config(lines1, lines2, lines3):
    global config
    config = {}
    config['host'] = subprocess.check_output(['hostname']).strip().decode('utf-8')
    config['primary_dns'] = "undefined"
    config['secondary_dns'] = "undefined"

    try:
        for line1 in lines1:
            line1 = line1.strip()
            if line1.startswith('address'):
                config['ip1'] = line1.split()[1].split('/')[0]
            elif line1.startswith('netmask'):
                config['subnet_mask'] = line1.split()[1]
            elif line1.startswith('gateway'):
                config['gateway'] = line1.split()[1]

        for line2 in lines2:
            line2 = line2.strip()
            if line2.startswith('address'):
                config['ip2'] = line2.split()[1].split('/')[0]
            elif line2.startswith('post-up'):
                parts = line2.split()
                if len(parts) > 4 and parts[3] == 'add':
                    config['radarip'] = parts[4].split('/')[0]  # Get the IP address

        for line3 in lines3:
            line3 = line3.strip()
            if line3.startswith('RESPONSE_SENDER'):
                config['device_id'] = line3.split('=')[1].strip()

    except KeyError as e:
        print(f"Error: Missing key {e} in 'config' section.")
        raise
    return config

def write_file(file_path, lines):
    try:
        with open(file_path, 'w') as file:
            file.writelines(lines)
    except Exception as e:
        return str(e)
    
def write_network_settings(host, rvd_address, device_id, ip_address1, ip_address2, gateway, subnet_mask, primary_dns, secondary_dns):
    #print(f"Writing network settings to config.ini: Host={host_address} IP={ip_address}, Gateway={gateway}, Subnet Mask={subnet_mask}, Primary DNS={primary_dns}, Secondary DNS={secondary_dns}")
    eth0_config = [
        f"auto eth0\n",
        f"iface eth0 inet static\n",
        f"    address {ip_address2}/24\n",
        f"    netmask {subnet_mask}\n",
        f"    gateway {gateway}\n",
        f"    #dns-nameservers {primary_dns} {secondary_dns}\n",
        f"    post-up ip route add {gateway}/32 dev eth0\n",
        f"    pre-down ip route del {gateway}/32 dev eth0\n",
    ]

    eth1_config = [
        f"auto eth1\n",
        f"iface eth1 inet static\n",
        f"    address {ip_address1}/24\n",
        f"    netmask {subnet_mask}\n",
        f"    #dns-nameservers {primary_dns} {secondary_dns}\n",
        f"    post-up ip route add {rvd_address}/32 dev eth1\n",
        f"    pre-down ip route del {rvd_address}/32 dev eth1\n",
    ]
    
    br0_config = [
        f"auto eth0\n",
        f"iface eth0 inet manual\n",
        f"auto eth1\n",
        f"iface eth1 inet manual\n",
        f"auto br0\n",
        f"iface br0 inet static\n",
        f"        address {ip_address2}\n",
        f"        netmask {subnet_mask}\n",
        f"        gateway {gateway}\n",
        f"        #post-up ip route add {gateway}/32 dev br0\n",
        f"   #pre-down ip route del {gateway}/32 dev br0\n",
        f"        bridge_ports eth0 eth1\n",
        f"        bridge_stp off\n",
        f"        bridge_fd 0\n",
    ]

    rvd_config = [
        f"RADAR_IP={rvd_address}\n",
        f"RADAR_PORT=55555\n",
        f"RADAR_ALIVE_PORT=60000\n\n",
        f"SERVER_PORT= 50002\n\n",
        f"RESPONSE_SENDER={device_id}\n",
        f"RESPONSE_RECEIVER=0\n\n",
        f"DEVICE_NETWORK_TIMEOUT=30\n",
        f"RESET_OUTPUT1_ENABLE=1\n",
        f"RESET_OUTPUT2_ENABLE=1\n",
    ]

    write_file(eth0_path, eth0_config)
    write_file(eth1_path, eth1_config)
    write_file(br0_path, br0_config)
    write_file(rvd_config_path, rvd_config)
    print("Network settings written successfully.")

def extract_version():
    try:
        # Get list of files that start with 'RVD'
        files = [f for f in os.listdir(firmware_path) if f.startswith('RVD')]
        
        if not files:
            return None

        # Sort files by their modification time (latest last)
        files.sort(key=lambda f: os.path.getmtime(os.path.join(firmware_path, f)))
        
        # Get the latest file
        filename = files[-1]
        file_path = os.path.join(firmware_path, filename)

        # Extract the version from the filename
        parts = filename.split('_')
        
        if len(parts) < 2:
            # If there is no part after '_', return None
            return None
        
        version_part = parts[1]  # The part after '_'
        version = version_part.split('.')[0]  # Get version before first dot

        # Optionally get the file's last modification time
        modification_time = os.path.getmtime(file_path)
        
        # Return the filename, version, and formatted modification time
        result = subprocess.run(['stat', '-c', '%y', file_path], stdout=subprocess.PIPE)
        timestamp = result.stdout.decode('utf-8').strip()
        
        return filename, version, timestamp
    
    except FileNotFoundError:
        print(f"Error: The path {firmware_path} does not exist.")
        return None
    except IndexError:
        # Return None if filename format is incorrect
        print("Error: Filename format is incorrect.")
        return None
    except Exception as e:
        # Catch any other exceptions and print the error
        print(f"An unexpected error occurred: {e}")
        return None
    
@app.route('/get-firmware-detail', methods=['GET'])
def get_firmware_detail():
    try:
        # Extract version using the extract_version function
        filename, version, moddate = extract_version()

        if filename and version:
            return jsonify({
                "filename": filename,
                "version": version,
                "moddate": moddate
            }), 200
        else:
            return jsonify({"error": "Invalid filename format"}), 400
    except Exception as e:
        return jsonify({"error": str(e)}), 500
    
@app.route('/upload-firmware', methods=['POST'])
def upload_firmware():
    try:
        if 'file' not in request.files:
            return jsonify({"error": "No file part in the request"}), 400

        file = request.files['file']

        if file.filename == '':
            return jsonify({"error": "No selected file"}), 400

        # Save the file to the specified folder
        file_path = os.path.join(app.config['firmware_path'], file.filename)
        file.save(file_path)
        
        #output_filename = f"rvd_{version}"
        #output_file_path = os.path.join(app.config['firmware_path'], output_filename)
        gcc_command = f"gcc {file_path} -o /root/RVD_APP/sources/rvd-v1.0.0b1 -lpthread -lmodbus"

        process = subprocess.run(gcc_command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        if process.returncode != 0:
            return jsonify({"error": process.stderr.decode('utf-8')}), 500

        return jsonify({"message": f"{file.filename} uploaded and compiled successfully"}), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500
    
@app.route('/get-network-settings', methods=['GET'])
def get_network_settings():
    print("GET request received for network settings.")

    try:
        eth0_lines = read_file(eth0_path)
        eth1_lines = read_file(eth1_path)
        rvd_lines = read_file(rvd_config_path)

        eth_config = parse_eth_config(eth0_lines, eth1_lines, rvd_lines)

        print(f"Network settings retrieved: {eth_config}")
        return jsonify(eth_config)
    except Exception as e:
        print(f"Error retrieving network settings: {e}")
        return jsonify(error=str(e)), 500

@app.route('/set-network-settings', methods=['POST'])
def set_network_settings():
    print("POST request received to set network settings.")
    data = request.get_json()
    host_address = data.get('host')
    radarip = data.get('radarip')
    device_id = data.get('device_id')
    ip_address1 = data.get('ip1')
    ip_address2 = data.get('ip2')
    gateway = data.get('gateway')
    subnet_mask = data.get('subnet_mask')
    primary_dns = data.get('primary_dns')
    secondary_dns = data.get('secondary_dns')
    # firmware_version = data.get('firmware_version')
    #print(f"Setting network settings to: Host={host_address} IP={ip_address}, Gateway={gateway}, Subnet Mask={subnet_mask}, Primary DNS={primary_dns}, Secondary DNS={secondary_dns}")
    try:
        write_network_settings(host_address, radarip, device_id, ip_address1, ip_address2, gateway, subnet_mask, primary_dns, secondary_dns)
        return jsonify(success=True)
    except Exception as e:
        print(f"Error setting network settings: {e}")
        return jsonify(error=str(e)), 500

@app.route('/set-relay-control', methods=['POST'])
def set_relay_control():
    try:
        data = request.json
        relay01 = data.get('relay01')
        relay02 = data.get('relay02')

        # Process the relay settings here
        print(f"Relay 1: {relay01}, Relay 2: {relay02}")

        with open(rvd_config_path, 'r') as file:
            lines = file.readlines()

        for i in range(len(lines)):
            if lines[i].startswith("RESET_OUTPUT1_ENABLE="):
                lines[i] = f"RESET_OUTPUT1_ENABLE={relay01}\n"
            elif lines[i].startswith("RESET_OUTPUT2_ENABLE="):
                lines[i] = f"RESET_OUTPUT2_ENABLE={relay02}\n"
        
        with open(rvd_config_path, 'w') as file:
            file.writelines(lines)

        return jsonify({'message': 'Relay settings updated successfully'}), 200
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/test-relay', methods=['POST'])
def test_relay():
    try:
        result = subprocess.run(['/root/RVD_APP/tests/iotest'], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return jsonify({"message": "Test command sent."}), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500
    
@app.route('/reboot', methods=['POST'])
def reboot_device():
    try:
        os.system('sudo reboot')
        return jsonify({"message": "Reboot command sent."}), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500
    
@app.route('/')
def index():
    print("Serving index.html")
    return send_from_directory('static', 'index.html')

if __name__ == '__main__':
    port = web_config.getint('settings', 'port', fallback=5000)
    print(f"Starting the Flask server on port {port}...")
    app.run(host="0.0.0.0", debug=True, port=port)
