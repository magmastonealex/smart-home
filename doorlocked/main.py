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
    dictionary = cv.aruco.Dictionary_get(cv.aruco.DICT_6X6_50)
    parameters =  cv.aruco.DetectorParameters_create()
    markerCorners, markerIds, rejectedCandidates = cv.aruco.detectMarkers(img, dictionary, parameters=parameters)
    cv.aruco.drawDetectedMarkers(annotated, markerCorners, markerIds)
    
    foundTop = -1
    for idx, markerId in enumerate(markerIds):
        if markerId[0] == 20:
            foundTop = idx

    if foundTop == -1:
        print("Could not find top marker.")
        err = RuntimeError("Failed to find top marker")
        err.image = annotated
        raise err

    # Find our top-left and bottom-right corners of our cropped image.
    topCorner = markerCorners[foundTop][0][3]
    
    synthCorners = np.array([[0, 0], [500, 0], [500, 500], [0, 500]], np.float32)
    transformation = cv.getPerspectiveTransform(markerCorners[foundTop][0], synthCorners)
    resImg = cv.warpPerspective(img, transformation, (900, 1600))

    # Crop the image based on those corners.
    crop_img = resImg[710:1256, 0:542]

    # Convert the image to black and white based on the corner of the image - this will always be "white" or the closest thing to.    
    grayImage = cv.cvtColor(crop_img, cv.COLOR_BGR2GRAY)
    _, res = cv.threshold(grayImage, 30, 255, cv.THRESH_BINARY)

    # Find contours in the image. There will be several found, but most are overlapping...
    contours, hierarchy = cv.findContours(res, cv.RETR_TREE, cv.CHAIN_APPROX_SIMPLE)
    # Draw those contours back onto the image to "harden" the lines.
    cv.drawContours(res, contours, -1, 255, 3)

    # Use the "hardened" contours to find the actual lock contour.
    contours, hierarchy = cv.findContours(res, cv.RETR_TREE, cv.CHAIN_APPROX_SIMPLE)

    largestContour = None
    largestContourArea = -1
    for idx, contour in enumerate(contours):
        area = cv.contourArea(contour)
        if area > largestContourArea:
            largestContour = idx
            largestContourArea = area

    if largestContour is None:
        raise RuntimeError('No contours found')

    if largestContourArea < 80:
        err = RuntimeError('Largest contour not large enough to be lock.')
        cv.drawContours(crop_img, contours, -1, (255,0,0), cv.FILLED)
        err.image = crop_img
        raise err

    # Fit a line through the largest contour to determine which direction it's pointing.
    [vx,vy,x,y] = cv.fitLine(contours[largestContour], cv.DIST_L2,0,0.01,0.01)

    # Annotate that line on the image for later display.
    rows,cols = crop_img.shape[:2]
    lefty = int((-x*vy/vx) + y)
    righty = int(((cols-x)*vy/vx)+y)
    cv.line(crop_img,(cols-1,righty),(0,lefty),(0,255,0),2)

    slope = vx/vy
    if slope > 0 and slope < 0.60:
        return ('locked', crop_img)
    else:
        return ('unlocked', crop_img)


def get_lock_state_once():
    try :
        z = requests.get(stillimg_url, timeout=5)
        image = np.frombuffer(z.content, np.uint8)
        image = cv.imdecode(image, cv.IMREAD_COLOR)
        state = process_image_for_state(image)
        return state
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
                    states_dict[state[0]] = {
                        "count": 1,
                        "image": state[1]
                    }
                else:
                    states_dict[state[0]]['count'] += 1
                time.sleep(0.100)

            winning_state = None
            winning_state_count = -1
            for k, v in states_dict.items():
                if v["count"] > winning_state_count:
                    winning_state = k
                    winning_state_count = v["count"]

            confidence = float(winning_state_count)/float(total_runs)
            print(winning_state)
            print(confidence)
            return winning_state
        except Exception as e:
            print('failed to fetch')
            print(e)
            return 'unknown'

app.run(host='0.0.0.0', port=port)
