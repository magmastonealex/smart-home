import cv2 as cv
import sys
import numpy as np
import time
import requests

# The strategy here is kinda complicated.
# First, use our two markers to crop down the image.
# Then, mask off the area where we know the lock is.
# Next, threshold it into black and white using a section of the image we know represents "white".
# With the thresholded image, perform contour detection.
# Draw those contours back onto the image to close up any "gaps" in the white area of the latch.
# Re-run contour detection on the image, selecting the largest area to find the actual contour of the lock.
# Fit a line to the contour.
# If the slope of the line is negative (or vertical), then the lock is locked.
# If the slope of the line is positive, then the lock is unlocked.
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
    foundBottom = -1
    for idx, markerId in enumerate(markerIds):
        if markerId[0] == 20:
            foundTop = idx
        if markerId[0] == 3:
            foundBottom = idx

    if foundTop == -1:
        print("Could not find top marker.")
        err = RuntimeError("Failed to find top marker")
        err.image = annotated
        raise err

    if foundBottom == -1:
        print("Could not find bottom marker.")
        err = RuntimeError("Failed to find bottom marker")
        err.image = annotated
        raise err
    
    # Find our top-left and bottom-right corners of our cropped image.
    topCorner = markerCorners[foundTop][0][3]
    bottomCorner = markerCorners[foundBottom][0][0]

    # Crop the image based on those corners.
    crop_img = img[int(topCorner[1]):int(bottomCorner[1]), int(topCorner[0]):int(bottomCorner[0])]

    # Convert the image to black and white based on the corner of the image - this will always be "white" or the closest thing to.    
    grayImage = cv.cvtColor(crop_img, cv.COLOR_BGR2GRAY)
    _, blackAndWhiteImage = cv.threshold(grayImage, int(grayImage[0:5, 0:5].mean()), 255, cv.THRESH_BINARY)

    # With our marker registration, we have confidence in where the lock actually is in the image, so filter it out
    mask = np.zeros((crop_img.shape[0], crop_img.shape[1], 1), dtype=np.uint8)
    cv.rectangle(mask,(10, 10), (38, 24), 255, cv.FILLED)
    cv.rectangle(annotated[int(topCorner[1]):int(bottomCorner[1]), int(topCorner[0]):int(bottomCorner[0])],(10, 10), (38, 24), (0, 0, 255), 1)
    res = cv.bitwise_and(blackAndWhiteImage,blackAndWhiteImage,mask = mask)

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
        print(area)
        if area > largestContourArea:
            largestContour = idx
            largestContourArea = area

    if largestContour is None:
        raise RuntimeError('No contours found')

    if largestContourArea < 80:
        err = RuntimeError('Largest contour not large enough to be lock.')
        cv.drawContours(annotated[int(topCorner[1]):int(bottomCorner[1]), int(topCorner[0]):int(bottomCorner[0])], contours, -1, (255,0,0), cv.FILLED)
        err.image = annotated
        raise err
    
    # Annotate our found contour back onto the image for later display.
    cv.drawContours(annotated[int(topCorner[1]):int(bottomCorner[1]), int(topCorner[0]):int(bottomCorner[0])], contours, largestContour, (255,0,0), cv.FILLED)

    # Fit a line through the largest contour to determine which direction it's pointing.
    [vx,vy,x,y] = cv.fitLine(contours[largestContour], cv.DIST_L2,0,0.01,0.01)

    # Annotate that line on the image for later display.
    rows,cols = crop_img.shape[:2]
    lefty = int((-x*vy/vx) + y)
    righty = int(((cols-x)*vy/vx)+y)
    cv.line(annotated[int(topCorner[1]):int(bottomCorner[1]), int(topCorner[0]):int(bottomCorner[0])],(cols-1,righty),(0,lefty),(0,255,0),2)

    # Technically, slope is vy/vx, but this works just as well.
    if vy[0] < 0:
        return ('unlocked', annotated)
    else:
        return ('locked', annotated)


def get_lock_state_once():
    try :
        z = requests.get('http://root:ismart12@10.102.40.20:8084/cgi-bin/currentpic.cgi', timeout=5)
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


total_runs = 10
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
cv.imshow("Annotated Image", states_dict[winning_state]["image"])
print(winning_state)
print(confidence)
cv.waitKey()