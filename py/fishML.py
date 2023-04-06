import cv2 as cv
import numpy as np
import os

# Load the pre-trained Haar cascade for face detection
face_cascade = cv.CascadeClassifier('models/haarcascade_frontalface_default.xml')
    
def fishML(mat):    
    # Convert the image to grayscale
    gray = cv.cvtColor(mat, cv.COLOR_BGR2GRAY)
    
    # Detect objects in the image using the face cascade classifier
    faces = face_cascade.detectMultiScale(gray, scaleFactor=1.1, minNeighbors=5, minSize=(30, 30))    
    
    returnRects = []
    
    for (x, y, w, h) in faces:
        returnRects.append([x, y, w, h, 0.15])    
    
    # Return 0 to indicate success
    return returnRects
    

#If we are in main program
if __name__ == "__main__":
    # Define the key variable assigned to waitKey
    key = ''
    
    abs_path = os.path.dirname(__file__)
    rel_path = 'img/arnie_large.png'
    full_path = os.path.join(abs_path, rel_path)
        
    img = cv.imread(full_path)
    
    returnList = fishML(img)
    print(returnList)
        
    while(key != ord('q')):
        # Parse through the list of faces
        for face in returnList:
            x = face[0]
            y = face[1]
            w = face[2]
            h = face[3]            
            cv.rectangle(img, (x, y), (x+w, y+h), (0, 255, 0), 2)
        
        # Display the image
        cv.imshow('Image', img)

        # Wait for a key press
        key = cv.waitKey(0)

    cv.destroyAllWindows()
    

