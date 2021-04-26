import json
import numpy as np
import cv2 as cv
import re
import copy
import uuid
from functools import reduce
from google.cloud import vision
import io

# Receipt parser based on Google Cloud Vision API output.
# It's modelled off of Zehrs receipts, but should work for Loblaws too (& most of their brands).
# Output is a list of line-items, where each item has:
#   - UPC/PLU
#   - Short description
#   - Price, one of:
#       - $/KG
#       - $/unit

# returns tuple of top-left and bottom-right for a vertex set from Google.
def vertToBounding(box):
  minY = min(box, key = lambda k: k.y).y
  maxY = max(box, key = lambda k: k.y).y
  minX = min(box, key = lambda k: k.x).x
  maxX = max(box, key = lambda k: k.x).x
  return ((minX, minY), (maxX, maxY)) 

def reducer(accum, item):
  print(item)
  accum.append(item)
  return accum


img_file = 'tests/PTDC0005.JPG'
res = None
with io.open(img_file, 'rb') as image_file:
    content = image_file.read()
    client = vision.ImageAnnotatorClient()
    image = vision.Image(content=content)
    print('Detecting text...')
    res = client.document_text_detection(image=image)
    print('Done!')

if res.error.message:
  print('Cloud Vision failed:', res.error.message, res.error)

img = cv.imread(img_file)

words = []

pages = res.full_text_annotation.pages

for page in pages:
  for block in page.blocks:
    for para in block.paragraphs:
      for word in para.words:
        wordtxt = ''
        wordbounding = vertToBounding(word.bounding_box.vertices)
        for sym in word.symbols:
          if 'text' in sym:
            wordtxt += sym.text
        words.append((wordtxt, wordbounding, str(uuid.uuid4())))


isDigit = re.compile('\d?\d[.,]\s?\d\s?\d')
font = cv.FONT_HERSHEY_SIMPLEX
for word in words:
  topLeft = word[1][0]
  bottomRight = word[1][1]

  cv.putText(img, word[0], (topLeft[0],bottomRight[1]), font, 1, (0, 255, 0), 2, cv.LINE_AA)
  color = (0, 255, 0)
  if isDigit.match(word[0]) is not None:
    color = (255, 0, 0)
  cv.rectangle(img, topLeft, bottomRight, color, 2)

# Algorithm:
#  Find the price column - vertical rows of lots of things like \d.\d\d
#  On a long enough receipt, this will usually work fine....
#  Sanity check - it's in the right 3rd of the receipt.
#
#  Separate out line items on the receipt.
#  
#  For each line, determine price & qty
#  If no price, include the next line in the line item until

# Start by finding the "price" column Y index.

xValuesPrices = []

for word in words:
  if isDigit.match(word[0]) is not None:
    # bounding box, topLeft, x coordinate.
    # need some named tuples at some point.
    xValuesPrices.append(word[1][0][0])

priceLine = int(np.median(xValuesPrices)) - 100
cv.line(img, (priceLine, 0), (priceLine, img.shape[0]), (255, 0, 0), 2)

foundWords = {}
foundLines = []



for word in words:
  if word[2] in foundWords:
    continue
  foundWords[word[2]] = True
  # Find any other words that haven't been claimed yet that seem to be on the same horizontal line - 
  # i.e the horizontal line from the midpoint intersects with other words.
  midY = int((word[1][0][1] + word[1][1][1]) / 2)
  wordsInLine = [word]
  for otherWord in words:
    if otherWord[2] in foundWords:
      continue
    if otherWord[1][0][1] <= midY and otherWord[1][1][1] >= midY:
      # Word is on the same line!
      foundWords[otherWord[2]] = True
      wordsInLine.append(otherWord)  
  cv.line(img, (0, midY), (img.shape[1], midY), (0, 0, 255), 2)
  wordsInLine.sort(key=lambda wrd: wrd[1][0][0])
  # Join words in this sentence that are over the priceLine...
  
  sentence= ' '.join([wrd[0] if wrd[1][0][0] < priceLine else '' for wrd in wordsInLine]).strip()
  price = ''
  for wrd in wordsInLine:
    if wrd[1][0][0] >= priceLine:
      price += wrd[0]

  if price == '':
    price = None
  else:
    price = price.replace(' ', '')

  minY = min(wordsInLine, key = lambda k: k[1][0][1])[1][0][1]
  maxY = max(wordsInLine, key = lambda k: k[1][1][1])[1][1][1]
  minX = min(wordsInLine, key = lambda k: k[1][0][0])[1][0][0]
  maxX = max(wordsInLine, key = lambda k: k[1][1][0])[1][1][0]
  bounding = ((minX, minY), (maxX, maxY))

  cv.rectangle(img, bounding[0], bounding[1], (0, 0, 255), 2) 
  cv.putText(img, sentence, bounding[0], font, 1, (0, 0, 255), 2, cv.LINE_AA) 
  foundLines.append((sentence, bounding, price))


foundLines.sort(key=lambda line: line[1][0][1])

subtotalMatch = re.compile('^SUBTOTAL')
categoryMatch = re.compile('^\d\d?\s+-\s+\w+',  re.IGNORECASE)
# Find our first category - everything after is a garbage.
# Everything after and including "SUBTOTAL".
lineItems = []
firstCatFound = False
i = 0

while i < len(foundLines):
  line = foundLines[i]
  if categoryMatch.match(line[0]) is not None:
    firstCatFound = True
    i+=1
    continue
  if not firstCatFound:
    i+=1
    continue
  if subtotalMatch.match(line[0]) is not None:
    break
  if line[2] is not None and 'pts' in line[2].lower():
    i+=1
    continue
  lineItemInfo = []
  lineItemPrice = None
  while i < len(foundLines) and lineItemPrice is None:
    thisLine = foundLines[i]
    if thisLine[2] is not None and 'pts' in thisLine[2].lower():
      i+=1
      continue
    lineItemInfo.append(thisLine[0])
    lineItemPrice = thisLine[2]
    if subtotalMatch.match(thisLine[0]) is not None:
      break
    i+=1
  if lineItemPrice is None:
    break
  lineItems.append(('\n'.join(lineItemInfo), lineItemPrice))


# lineItems is every item on the receipt.
# Prices fall into three categories:
#  - Per-Kilogram
#  - Straight pricing
#  - volume discounts.
#  We treat volume discounts as straight pricing - just divide volume pricing by qty to get the straight price.s
for line in lineItems:
  print(line)
cv.imwrite('out.png', img)


