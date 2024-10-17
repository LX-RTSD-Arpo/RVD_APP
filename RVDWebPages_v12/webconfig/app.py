import os
from flask import Flask, jsonify, request, send_from_directory, render_template, redirect, url_for, session, make_response
from werkzeug.security import generate_password_hash, check_password_hash
import configparser
import subprocess
from datetime import datetime, timedelta
import pytz

app = Flask(__name__)

app.secret_key = os.urandom(24)

web_config_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'web_config.ini')

web_config = configparser.ConfigParser()
web_config.read(web_config_path)

eth0_path = web_config.get('filepaths', 'eth0_path')
eth1_path = web_config.get('filepaths', 'eth1_path')
br0_path = web_config.get('filepaths', 'br0_path')
rvd_config_path = web_config.get('filepaths', 'rvd_config_path')
firmware_path = web_config.get('filepaths', 'firmware_path')

app.config['firmware_path'] = firmware_path

users = {
    'admin': generate_password_hash('system')  # Use hashed passwords for security
}

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
    # config['primary_dns'] = "undefined"
    # config['secondary_dns'] = "undefined"

    try:
        for line1 in lines1:
            line1 = line1.strip()
            if line1.startswith('address'):
                config['ip1'] = line1.split()[1].split('/')[0]
            elif line1.startswith('netmask'):
                config['subnet_mask'] = line1.split()[1]
            elif line1.startswith('gateway'):
                config['gateway'] = line1.split()[1]
            elif line1.startswith('dns-nameservers') or line1.startswith('#dns-nameservers'):
                dns_servers = line1.split()[1:]
                if len(dns_servers) > 0:
                    config['primary_dns'] = dns_servers[0]
                if len(dns_servers) > 1:
                    config['secondary_dns'] = dns_servers[1]

        for line2 in lines2:
            line2 = line2.strip()
            if line2.startswith('address'):
                config['ip2'] = line2.split()[1].split('/')[0]
            elif line2.startswith('post-up'):
                parts = line2.split()
                if len(parts) > 4 and parts[3] == 'add':
                    config['radarip'] = parts[4].split('/')[0]

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
    eth0_config = [
        f"auto eth0\n",
        f"iface eth0 inet static\n",
        f"    address {ip_address2}/24\n",
        f"    netmask {subnet_mask}\n",
        f"    gateway {gateway}\n",
        f"    dns-nameservers {primary_dns} {secondary_dns}\n",
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
        f"DEVICE_NETWORK_TIMEOUT=30\n\n",
        f"RESET_OUTPUT1_ENABLE=1\n",
        f"RESET_OUTPUT2_ENABLE=1\n\n",
    ]

    write_file(eth0_path, eth0_config)
    write_file(eth1_path, eth1_config)
    write_file(br0_path, br0_config)
    write_file(rvd_config_path, rvd_config)

    with open('/etc/hostname', 'w') as hostname_file:
        hostname_file.write(host + '\n')
    print("Network settings written successfully.")

def extract_version():
    try:
        files = [f for f in os.listdir(firmware_path) if f.startswith('RVD')]
        
        if not files:
            return None

        files.sort(key=lambda f: os.path.getmtime(os.path.join(firmware_path, f)))
        
        filename = files[-1]
        file_path = os.path.join(firmware_path, filename)

        if filename.endswith('.c'):
            filename = filename[:-2]

        parts = filename.split('_')
        
        if len(parts) < 2:
            return None
        
        version_part = parts[1]
        #version = version_part.split('.')[0]

        #modification_time = os.path.getmtime(file_path)
        
        result = subprocess.run(['stat', '-c', '%y', file_path], stdout=subprocess.PIPE)
        timestamp = result.stdout.decode('utf-8').strip()
        
        return filename, version_part, timestamp
    
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
    
def write_ntp_settings(ntppriserver, ntpautosync, ntptimesync, ntptimeout):
    try:
        web_config['ntpsettings']['ntp_server'] = ntppriserver
        web_config['ntpsettings']['ntp_timesync'] = str(ntptimesync)  # Store as string
        web_config['ntpsettings']['ntp_timeout'] = str(ntptimeout)    # Store as string
        
        # Step 3: Write the updated config back to the file
        with open(web_config_path, 'w') as configfile:
            web_config.write(configfile)
        #subprocess.run(ntpdate_cmd, shell=True, check=True)
        print(ntpautosync)
        ntpautosync = int(ntpautosync)

        if ntpautosync == 1:
            new_crontab_content = f"""SHELL=/bin/sh
HOME=/root
PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin

# * * * * * your_command
# | | | | |
# | | | | +-- Day of the week (0 - 7) (Sunday is 0 or 7)
# | | | +---- Month (1 - 12)
# | | +------ Day of the month (1 - 31)
# | +-------- Hour (0 - 23)
# +---------- Minute (0 - 59)

* * * * * /root/RVD_APP/run.sh # Check the RVD program every 1 minute
*/{ntptimesync} * * * * /usr/sbin/ntpdate {ntppriserver} > /dev/null 2>&1
            """
        else:
            new_crontab_content = f"""SHELL=/bin/sh
HOME=/root
PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin

# * * * * * your_command
# | | | | |
# | | | | +-- Day of the week (0 - 7) (Sunday is 0 or 7)
# | | | +---- Month (1 - 12)
# | | +------ Day of the month (1 - 31)
# | +-------- Hour (0 - 23)
# +---------- Minute (0 - 59)

* * * * * /root/RVD_APP/run.sh # Check the RVD program every 1 minute
#*/{ntptimesync} * * * * /usr/sbin/ntpdate {ntppriserver} > /dev/null 2>&1
            """

        print(new_crontab_content)
        process = subprocess.Popen(["crontab", "-"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        process.communicate(input=new_crontab_content.encode("utf-8"))

        print("NTP settings updated successfully.")
    except subprocess.CalledProcessError as e:
        print(f"Error running the command: {e}")

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
        relay01 = int(data.get('relay01'))
        relay02 = int(data.get('relay02'))

        with open(rvd_config_path, 'r') as file:
            lines = file.readlines()
        
        updates = {
            "RESET_OUTPUT1_ENABLE=": relay01 if relay01 != 2 else 1,
            "RESET_OUTPUT2_ENABLE=": relay02 if relay02 != 2 else 1,
            "TOGGLE1_ENABLE=": 1 if relay01 == 2 else 0,
            "TOGGLE2_ENABLE=": 1 if relay02 == 2 else 0
        }

        for i in range(len(lines)):
            for key, value in updates.items():
                if lines[i].startswith(key):
                    lines[i] = f"{key}{value}\n"

        # if (relay01 == 2) and (relay02 != 2):
        #     # print(f"Relay 1: {relay01}, Relay 2: {relay02}")
        #     for i in range(len(lines)):
        #         if lines[i].startswith("RESET_OUTPUT1_ENABLE="):
        #             lines[i] = f"RESET_OUTPUT1_ENABLE=1\n"
        #         elif lines[i].startswith("RESET_OUTPUT2_ENABLE="):
        #             lines[i] = f"RESET_OUTPUT2_ENABLE={relay02}\n"
        #         elif lines[i].startswith("TOGGLE1_ENABLE="):
        #             lines[i] = f"TOGGLE1_ENABLE=1\n"
        #         elif lines[i].startswith("TOGGLE2_ENABLE="):
        #             lines[i] = f"TOGGLE2_ENABLE=0\n"
        # elif (relay01 != 2) and (relay02 == 2):
        #     for i in range(len(lines)):
        #         if lines[i].startswith("RESET_OUTPUT1_ENABLE="):
        #             lines[i] = f"RESET_OUTPUT1_ENABLE={relay01}\n"
        #         elif lines[i].startswith("RESET_OUTPUT2_ENABLE="):
        #             lines[i] = f"RESET_OUTPUT2_ENABLE=1\n"
        #         elif lines[i].startswith("TOGGLE1_ENABLE="):
        #             lines[i] = f"TOGGLE1_ENABLE=0\n"
        #         elif lines[i].startswith("TOGGLE2_ENABLE="):
        #             lines[i] = f"TOGGLE2_ENABLE=1\n"
        # elif (relay01 == 2) and (relay02 == 2):
        #     for i in range(len(lines)):
        #         if lines[i].startswith("RESET_OUTPUT1_ENABLE="):
        #             lines[i] = f"RESET_OUTPUT1_ENABLE=1\n"
        #         elif lines[i].startswith("RESET_OUTPUT2_ENABLE="):
        #             lines[i] = f"RESET_OUTPUT2_ENABLE=1\n"
        #         elif lines[i].startswith("TOGGLE1_ENABLE="):
        #             lines[i] = f"TOGGLE1_ENABLE=1\n"
        #         elif lines[i].startswith("TOGGLE2_ENABLE="):
        #             lines[i] = f"TOGGLE2_ENABLE=1\n"
        # else:
        #     for i in range(len(lines)):
        #         if lines[i].startswith("RESET_OUTPUT1_ENABLE="):
        #             lines[i] = f"RESET_OUTPUT1_ENABLE={relay01}\n"
        #         elif lines[i].startswith("RESET_OUTPUT2_ENABLE="):
        #             lines[i] = f"RESET_OUTPUT2_ENABLE={relay02}\n"
        #         elif lines[i].startswith("TOGGLE1_ENABLE="):
        #             lines[i] = f"TOGGLE1_ENABLE=0\n"
        #         elif lines[i].startswith("TOGGLE2_ENABLE="):
        #             lines[i] = f"TOGGLE2_ENABLE=0\n"
            
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
    
@app.route('/get-ntp-settings', methods=['GET'])
def get_ntp_settings():
    try:
        ntp_server = web_config.get('ntpsettings', 'ntp_server', fallback=None)
        ntp_timesync = web_config.get('ntpsettings', 'ntp_timesync', fallback=None)
        ntp_timeout = web_config.get('ntpsettings', 'ntp_timeout', fallback=None)

        try:
            ntpdate_output = subprocess.check_output(["ntpdate", "-q", ntp_server], stderr=subprocess.STDOUT)
            ntpdate_output = ntpdate_output.decode("utf-8")

            if "no server suitable" in ntpdate_output:
                status = "NTP server unreachable"
            elif ("adjust time server" in ntpdate_output) or ("step time server" in ntpdate_output):
                status = "NTP synchronized"
            else:
                status = "NTP status unknown"
        except subprocess.CalledProcessError as e:
            status = f"NTP check failed: {e.output.decode('utf-8').strip()}"

        crontab_output = subprocess.check_output(["crontab", "-l"], stderr=subprocess.STDOUT)
        crontab_lines = crontab_output.decode("utf-8").splitlines()

        ntp_autosync = "OFF"  # Default value if ntpautosync is not found

        for line in crontab_lines:
            if 'ntpdate' in line:
                if line.strip().startswith('#'):
                    ntp_autosync = "OFF"  # ntpdate line is commented
                else:
                    ntp_autosync = "ON"  # ntpdate line is not commented
                break

        ntp_current_time = datetime.now()

        return jsonify({
                "ntp_priserver": ntp_server,
                "ntp_status": status,
                "ntp_current_time": ntp_current_time,
                "ntp_autosync": ntp_autosync,
                "ntp_timesync": ntp_timesync,
                "ntp_timeout": ntp_timeout
            }), 200

    except Exception as e:
        return jsonify({"error": str(e)}), 500
    
@app.route('/set-ntp-settings', methods=['POST'])
def set_ntp_settings():
    data = request.get_json()

    ntppriserver = data.get('ntppriserver')
    ntpautosync = data.get('ntpautosync')
    ntptimesync = data.get('ntptimesync')
    ntptimeout = data.get('ntptimeout')
    
    try:
        write_ntp_settings(ntppriserver, ntpautosync, ntptimesync, ntptimeout)
        return jsonify(success=True)
    except Exception as e:
        print(f"Error setting network settings: {e}")
        return jsonify(error=str(e)), 500
    
@app.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        username = request.form['username']
        password = request.form['password']
        if username in users and check_password_hash(users[username], password):
            session['loggedin'] = True
            session['last_activity'] = datetime.now(pytz.timezone('Asia/Bangkok'))
            return redirect(url_for('index'))
        else:
            return "ชื่อผู้ใช้หรือรหัสผ่านไม่ถูกต้อง", 401
    return render_template('login.html')

@app.route('/ConnectCommand', methods=['GET', 'POST'])
def ConnectCommand():
    try:
        subprocess.run(['/root/RVD_APP/tools/bridge_on.sh'], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return jsonify({"message": "Connecting to Radar..."}), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500
    
@app.route('/logout', methods=['GET', 'POST'])
def logout():
    session.pop('loggedin', None)  # ลบ session
    return redirect(url_for('login'))  # Redirect ไปที่หน้า login

@app.route('/reboot', methods=['POST'])
def reboot_device():
    try:
        os.system('sudo reboot')
        return jsonify({"message": "Reboot command sent."}), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500
    
@app.route('/')
def index():
    if 'loggedin' not in session:
        return redirect(url_for('login'))
    return send_from_directory('static', 'index.html')

@app.route('/static/<path:path>')
def static_files(path):
    if 'loggedin' not in session:
        return redirect(url_for('login'))
    return send_from_directory('static', path)

@app.route('/get-login-status', methods=['GET'])
def get_login_status():
    return jsonify(loggedin='loggedin' in session)

@app.route('/dashboard')
def dashboard():
    if 'loggedin' not in session:
        return redirect('/login')

    response = make_response(render_template('dashboard.html'))
    response.headers['Cache-Control'] = 'no-store, no-cache, must-revalidate, max-age=0'
    response.headers['Pragma'] = 'no-cache'
    response.headers['Expires'] = '0'
    return response

@app.before_request
def require_login():
    if 'loggedin' not in session and request.endpoint != 'login' and request.endpoint != 'static':
        return redirect('/login')

if __name__ == '__main__':
    port = web_config.getint('settings', 'port', fallback=5000)
    print(f"Starting the Flask server on port {port}...")
    app.run(host="0.0.0.0", debug=True, port=port)
