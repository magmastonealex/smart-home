import cv2 as cv
import sys
import numpy as np
import os
import time
import requests
import flask
from flask import request
import json

config_file = './config.json'
stillimg_url = ''
total_runs = 0
port = 0

if 'CONFIG_FILE' in os.environ:
        config_file = os.environ['CONFIG_FILE']

with open(config_file, 'rb') as file:
        data = json.load(file)
        stillimg_url = data['imgurl']
        total_runs = data['retryTimes']
        port = data['port']

if stillimg_url == '' or total_runs == 0 or port == 0:
        print("Config file not found or missing required params.")
        sys.exit(1)

app = flask.Flask(__name__)

# The strategy here is kinda complicated.
# Use our marker to perform perspective correction on the image.
# With our perspective-shifted image, crop out the section which contains the lock
# Next, threshold it into black and white.
# With the thresholded image, perform contour detection.
# Draw those contours back onto the image to close up any "gaps" in the white area of the latch.
# Re-run contour detection on the image, selecting the largest area to find the actual contour of the lock.
# Fit a line to the contour.
# Judge the slope of the line to determine if the lock is locked or unlocked.
# If feature detection fails, the state of the lock is unknown.
def process_image_for_state(img):
    # Apply annotations to a copy of the original image (then we can make whatever transforms on the original.)
    annotated = img.copy()

    # Find
    dictionary = cv.aruco.Dictionary_get(cv.aruco.DICT_ARUCO_ORIGINAL)
    parameters =  cv.aruco.DetectorParameters_create()
    markerCorners, markerIds, rejectedCandidates = cv.aruco.detectMarkers(img, dictionary, parameters=parameters)
    cv.aruco.drawDetectedMarkers(annotated, markerCorners, markerIds)
    print(markerIds)
    markerIdsFin = {}
    for idx, markerId in enumerate(markerIds):
        markerIdsFin[markerId[0]] = True
    print(markerIdsFin)
    return markerIdsFin

expectedMarkers = [864, 1004, 40, 245, 844]

def get_lock_state_once():
    try :
        z = requests.get(stillimg_url, timeout=5)
        image = np.frombuffer(z.content, np.uint8)
        image = cv.imdecode(image, cv.IMREAD_COLOR)
        foundMarkers = process_image_for_state(image)
        numFound = 0
        for k, v in foundMarkers.items():
            if k in expectedMarkers:
                numFound += 1
        if numFound > 2:
            return 'closed'
        else:
            return 'open'
    except Exception as e:
        print(e)
        img = np.zeros((100, 100, 1), np.uint8)
        if hasattr(e, 'image'):
            img = e.image
        return ('unknown', img)


@app.route('/status', methods=['GET'])
def getStatus():
        try:
            states_dict = {}
            for i in range(0, total_runs):
                state = get_lock_state_once()
                if state[0] not in states_dict:
                    states_dict[state] = 1
                else:
                    states_dict[state] += 1
                time.sleep(0.100)

            winning_state = None
            winning_state_count = -1
            for k, v in states_dict.items():
                if v > winning_state_count:
                    winning_state = k
                    winning_state_count = v

            confidence = float(winning_state_count)/float(total_runs)
            print(winning_state)
            print(confidence)
            return winning_state
        except Exception as e:
            print('failed to fetch')
            print(e)
            return 'unknown'

app.run(host='0.0.0.0', port=port)
