#! /bin/bash

/usr/bin/python /bin/sendsmart.py "$(cat /etc/hostname) Disk ERROR: $SMARTD_FAILTYPE $SMARTD_MESSAGE"
