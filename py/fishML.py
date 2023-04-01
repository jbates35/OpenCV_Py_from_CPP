import cv2 as cv
import numpy as np
import os

def fishML():
    
    cwd = os.getcwd()
    
    # Create key binding
    key = ''
    
    # Load the image
    img = cv.imread(os.path.join(cwd, 'img/arnie.png'))

    while(key != ord('q')):
        # Display the image
        cv.imshow('Image', img)

        # Wait for a key press
        key = cv.waitKey(0)

    cv.destroyAllWindows()
    
    # Return 0 to indicate success
    return 0
    

#If we are in main program
if __name__ == "__main__":
    fishML()
