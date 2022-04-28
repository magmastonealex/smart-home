#!/usr/bin/python

import sys
import json
import subprocess

NAGIOS_OK=0
NAGIOS_WARN=1
NAGIOS_CRIT=2
NAGIOS_UNKN=3

maxtemp = 0
maxsensor = "unknown"
try:
    res = subprocess.run(["sensors", "-j"], check=True, stdout=subprocess.PIPE)
    sensors = json.loads(res.stdout)
    cores=sensors["coretemp-isa-0000"]
    if cores["Package id 0"]["temp1_input"] > maxtemp:
        maxtemp = cores["Package id 0"]["temp1_input"]
        maxsensor = "Package"
    if cores["Core 0"]["temp2_input"] > maxtemp:
        maxtemp = cores["Core 0"]["temp2_input"]
        maxsensor = "Core 0"
    if cores["Core 1"]["temp3_input"] > maxtemp:
        maxtemp = cores["Core 1"]["temp3_input"]
        maxsensor = "Core 1"
    if cores["Core 2"]["temp4_input"] > maxtemp:
        maxtemp = cores["Core 2"]["temp4_input"]
        maxsensor = "Core 2"
    if cores["Core 3"]["temp5_input"] > maxtemp:
        maxtemp = cores["Core 3"]["temp5_input"]
        maxsensor = "Core 3"

    print("Max temp is " + str(maxtemp) + " from " + maxsensor)
except:
    print("Failed - " + str(sys.exc_info()[0]))
    sys.exit(NAGIOS_UNKN)

if maxtemp >= 72:
    sys.exit(NAGIOS_CRIT)
elif maxtemp >= 69:
    sys.exit(NAGIOS_WARN)
else:
    sys.exit(NAGIOS_OK)


