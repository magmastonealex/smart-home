import cv2 as cv
import numpy as np
import time


dictionary = cv.aruco.Dictionary_get(cv.aruco.DICT_6X6_50)
markerImage = np.zeros((1000, 1000), dtype=np.uint8)
markerImage = cv.aruco.drawMarker(dictionary, 3, 1000, markerImage, 1)
cv.imshow("marker", markerImage)
cv.imwrite('marker.jpg', markerImage)
cv.waitKey(0)
