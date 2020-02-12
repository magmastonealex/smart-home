from datetime import datetime,timedelta
from icalevents.icalevents import events
import json
import requests
import sys
import os
import time

bearer_token = '';
host='10.102.40.20'
port='9123'
entity_id = ''
calendar_url = ''
config_file = './config.json'

if 'CONFIG_FILE' in os.environ:
	config_file = os.environ['CONFIG_FILE']

with open(config_file, 'rb') as file:
	data = json.load(file)
	print(data)
	bearer_token = data['bearer_token']
	host = data['host']
	port = data['port']
	entity_id = data['entity_id']
	calendar_url = data['calendar_url']

if bearer_token == '' or entity_id == '' or calendar_url == '':
	print("Config file not found or did not include bearer token.")
	sys.exit(1)

while True:
	try:
		start = datetime.now().date()
		end = (datetime.now() + timedelta(days=6)).date()

		es = events(url=calendar_url, start=start, end=end)

		es.sort(key=lambda e: int(e.start.timestamp()))
		evstr = ""
		for e in es[:3]:
			evstr = evstr + e.start.strftime("%a@%I:%M") + e.end.strftime("-%I:%M:\n  ") + e.summary + "\n"

		print(json.dumps({"str":evstr}))
		r = requests.post('http://{host}:{port}/api/services/input_text/set_value'.format(host=host, port=port),
			json={"value": evstr, "entity_id": entity_id},
			headers={"Authorization": "Bearer " + bearer_token, "Content-Type": "application/json"})

		if r.status_code != 200:
			print("Failed:", r.status_code, r.text)
		else:
			print("Success:", r.text)
		time.sleep(5 * 60)
	except Exception as e:
		print("Failed:", e)
		time.sleep(30)
