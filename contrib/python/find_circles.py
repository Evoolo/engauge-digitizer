#!/usr/bin/python3

from collections import defaultdict
from scipy import misc
import numpy as np

def circlePoints (minRadius, maxRadius):
    points = defaultdict (list)
    for radius in range (minRadius, maxRadius + 1):
        rOverRadical2 = int (radius / (2.0 ** 0.5) + 0.5)
        radiusSquared = radius * radius
        pointsAtRadius = []
        # With +x right and +y up. Start out at upper right and go clockwise through the four sides
        x = rOverRadical2
        y = rOverRadical2
        while y > -rOverRadical2:
            pointsAtRadius.append ([y, x])
            y = y - 1
            x = int ((radiusSquared - y * y) ** 0.5 + 0.5)
        while x > -rOverRadical2:
            pointsAtRadius.append ([y, x])
            x = x - 1
            y = -1 * int ((radiusSquared - x * x) ** 0.5 + 0.5)
        while y < rOverRadical2:
            pointsAtRadius.append ([y, x])
            y = y + 1
            x = -1 * int ((radiusSquared - y * y) ** 0.5 + 0.5)
        while x < rOverRadical2:
            pointsAtRadius.append ([y, x])
            x = x + 1
            y = int ((radiusSquared - x * x) ** 0.5 + 0.5)
        points [radius] = pointsAtRadius
    return points

def gradientDelta (imgIn, x, y, width, height, minRadius, maxRadius, circlePointsForRadius):
    maxGradientSquared = 0
    if minRadius < x and minRadius < y and x < width - minRadius and y < height - minRadius:
        circleSumBefore = 0
        hitBoundary = False
        for radius in range (minRadius, maxRadius):
            # Loop in a circle around the point (x,y)
            circleSum = 0
            circlePoints = circlePointsForRadius [radius]
            for circlePoint in circlePoints:
                xCircle = x + circlePoint [1]
                yCircle = y + circlePoint [0]
                if xCircle < 0 or yCircle < 0 or xCircle >= width or yCircle >= height:
                    # Exit out of all loops since the boundary has been reached and remaining circles
                    # are bigger
                    hitBoundary = True 
                    break
                pixel = imgIn [y + circlePoint [0]] [x + circlePoint [1]] [0]
                circleSum = circleSum + pixel
            if hitBoundary:
                break
            circleSum = circleSum / radius # Normalize so larger versus smaller circumference has no effect
            if radius > minRadius: # Make sure circleSumBefore is set
                # Gradient is change in value between last and current radius
                gradient = circleSum - circleSumBefore 
                gradientSquared = gradient * gradient
                if gradientSquared > maxGradientSquared:
                    maxGradientSquared = gradientSquared
            circleSumBefore = circleSum
    return maxGradientSquared ** 0.5

class LinkedList:
    def __init__ (self, maxCount):
        self.head = None
        self.addedCount = 0
        self.maxCount = maxCount

    def insert (self, fom, x, y):
        other = self.head
        previous = None
        if other is None:
            # List is empty so new entry is always made
            self.head = Node (fom, x, y)
            self.addedCount = self.addedCount + 1            
        else:
            # Skip if count limit has been reached and new entry does not measure up
            if self.addedCount < self.maxCount or fom > self.head.fom:
                # Insert
                while other is not None and fom > other.fom:
                    previous = other
                    other = other.next
                if previous is None:
                    self.head = Node (fom, x, y, self.head)
                else:
                    previous.next = Node (fom, x, y, other)
                self.addedCount = self.addedCount + 1
                if self.addedCount >= self.maxCount:
                    # Remove lowest value at start of list
                    self.head = self.head.next
                    self.addedCount = self.addedCount - 1

    def save (self, inImgFilename):
        # Convert singly linked list to doubly linked list
        other = self.head
        while other is not None:
            previous = other
            other = other.next
            if other is not None:
                other.previous = previous
            
        # Save to file. Use doubly linked list to output in decreasing order
        outCsvFilename = inImgFilename.replace (".png", ".csv")
        with open (outCsvFilename, 'w') as fs:
            other = previous
            while other is not None:
                fs.write (str (other.x) + ", " + str (other.y) + ", " + str (other.fom) + "\n")
                other = other.previous
                
def main ():

    # Input parameters in pixels
    minRadius = 10
    maxRadius = 30 # 80
    localMaximaSearchRadius = 5
    maxPeakCount = 200
    
    # Precompute circle points for each radius just once since this is expected to be time consuming
    # so we do not want to redo it at each pixel
    circlePointsForRadius = circlePoints (minRadius, maxRadius)
    
    # Load input image
    inImgFilename = 'circles_nolegend.png'
    imgIn = misc.imread (inImgFilename)
    
    width = len (imgIn [0])
    height = len (imgIn)

    # Populate the output array pixel by pixel
    arrOut = np.zeros ((height, width)) 
    maxGradientDelta = 0
    for x in range (0, width):
        for y in range (0, height):
            gradientDeltaAtXY = gradientDelta (imgIn, x, y, width, height, minRadius,
                                               maxRadius, circlePointsForRadius)
            arrOut [y] [x] = gradientDeltaAtXY
            if gradientDeltaAtXY > maxGradientDelta:
                maxGradientDelta = gradientDeltaAtXY # For normalization
        print (str (int (100 * (x + 1) / width)) + '%')

    # Find local maxima
    arrOut2 = np.zeros ((height, width))
    for x in range (0, width):
        for y in range (0, height):
            foundHigher = False
            # Look around for another pixel with a higher value
            for xDelta in range (- localMaximaSearchRadius, localMaximaSearchRadius):
                for yDelta in range (- localMaximaSearchRadius, localMaximaSearchRadius):
                    xLook = x + xDelta
                    yLook = y + yDelta
                    if 0 <= xLook and 0 <= yLook and xLook < width and yLook < height:
                        if arrOut [yLook] [xLook] > arrOut [y] [x]:
                            foundHigher = True
                            break
                if foundHigher:
                    break
            if foundHigher:
                arrOut2 [y] [x] = 0
            else:
                arrOut2 [y] [x] = arrOut [y] [x]
        print (str (int (100 * (x + 1) / width)) + '%')

    # Isolate best maxima
    ll = LinkedList (maxPeakCount)
    for x in range (0, width):
        for y in range (0, height):
            ll.insert (arrOut2 [y] [x], x, y)
    ll.save (inImgFilename)
    
    # Save output image. Initialize by copying input image. This gives imgOut.dtype=imgIn.dtype
    # which is probably UINT8
    imgOut = imgIn
    for x in range (0, width):
        for y in range (0, height):
            grayLevel = 255 * arrOut2 [y] [x] / maxGradientDelta
            imgOut [y] [x] [0] = grayLevel
            imgOut [y] [x] [1] = grayLevel
            imgOut [y] [x] [2] = grayLevel
    outImgFilename = inImgFilename.replace (".png", "_gradients.png")
    misc.imsave (outImgFilename, imgOut)
            
class Node:
    def __init__ (self, fom, x, y, next=None):
        self.fom = fom
        self.x = x
        self.y = y
        self.next = next
        self.previous = None

    def fom ():
        return self.fom

    def x ():
        return self.x

    def y ():
        return self.y
    
main ()



    
