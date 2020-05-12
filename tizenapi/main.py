import flask
from flask import request
import wakeonlan
import json
import os
import requests

config_file = './config.json'
device_id = ''
api_key = ''
mac_addr = ''
broadcast_addr = ''
port = 0

if 'CONFIG_FILE' in os.environ:
        config_file = os.environ['CONFIG_FILE']

with open(config_file, 'rb') as file:
        data = json.load(file)
        device_id = data['device_id']
        api_key = data['api_key']
        port = data['port']
        mac_addr = data['mac_addr']
        broadcast_addr = data['broadcast_addr']

if api_key == '' or device_id == '' or port == 0 or mac_addr == '' or broadcast_addr == '':
        print("Config file not found or missing required params.")
        sys.exit(1)


REQUEST_HEADERS = {"Authorization": "Bearer " + api_key}
API_BASEURL = "https://api.smartthings.com/v1/devices/" + device_id
API_COMMAND = API_BASEURL + "/commands"
API_STATES = API_BASEURL + "/states"
COMMAND_OFF = {"commands": [{"component": "main","capability": "switch", "command": "off"}]}
COMMAND_ON = {"commands": [{"component": "main","capability": "switch", "command": "on"}]}
COMMAND_REFRESH = {"commands": [{"component": "main","capability": "refresh", "command": "refresh"}]}

app = flask.Flask(__name__)

# Use SmartThings to determine state
@app.route('/status', methods=['GET'])
def getStatus():
        comm_resp = requests.post(API_COMMAND, json=COMMAND_REFRESH, headers=REQUEST_HEADERS)
        print(comm_resp.json())
        resp = requests.get(API_STATES, headers=REQUEST_HEADERS)
        data = resp.json()
        print(data["main"]["switch"])
        if data["main"]["switch"]["value"] == "off":
                return "OFF"
        else:
                return "ON"

# Turn on TV using SmartThings AND WoL
@app.route('/turnOn', methods=['POST'])
def turnOn():
        wakeonlan.send_magic_packet(mac_addr, ip_address=broadcast_addr)
        res = requests.post(API_COMMAND, json=COMMAND_ON, headers=REQUEST_HEADERS)
        print(res.json())
        return "OK"

# Turn off TV via SmartThings API
@app.route('/turnOff', methods=['POST'])
def turnOff():
        print(COMMAND_OFF)
        res = requests.post(API_COMMAND, json=COMMAND_OFF, headers=REQUEST_HEADERS)
        print(res.json())
        return "OK"

# set input to , e.g "HDMI1"
@app.route('/input', methods=['POST'])
def setInput():
        inp = request.args.get("input")
        set_input_command = {"commands": [{"component": "main","capability": "mediaInputSource", "command": "setInputSource", "arguments":[inp]}]}
        requests.post(API_COMMAND, json=set_input_command, headers=REQUEST_HEADERS)
        return "OK"

app.run(host='0.0.0.0')
