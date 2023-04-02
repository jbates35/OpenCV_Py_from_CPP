import cv2 as cv
import numpy as np
import os
import pathlib

from PIL import Image
from PIL import ImageDraw

from pycoral.adapters import common
from pycoral.adapters import detect
from pycoral.utils.dataset import read_label_file
from pycoral.utils.edgetpu import make_interpreter

# Specify the TensorFlow model, labels, and image
script_dir = pathlib.Path(__file__).parent.absolute()
model_file = os.path.join(script_dir, 'ssd_mobilenet_v2_face_quant_postprocess_edgetpu.tflite')

# Initialize the TF interpreter
interpreter = make_interpreter(model_file)
interpreter.allocate_tensors()

    
def fishML(mat):      
    # Convert the image to RGB format and resize it
    mat = cv.cvtColor(mat, cv.COLOR_BGR2RGB)
    size = common.input_size(interpreter)
    _, scale = common.set_resized_input(interpreter, mat.shape[:2], lambda size: cv.resize(mat, size))
    
    # Run an object detection
    interpreter.invoke()    
    objs = detect.get_objects(interpreter, 0.3, scale)

    # Create an empty list to store the rois
    returnRects = []
    
    # Print the result
    if not objs:
        print ("No objects detected")
        returnRects.append([-1, -1, -1, -1, -1])
        return returnRects
        
    for obj in objs:        
        # Get the bounding box
        xmin, ymin, xmax, ymax = obj.bbox
        returnRects.append([
            xmin, 
            ymin, 
            (xmax-xmin), 
            (ymax-ymin),
            obj.score
            ]) 
            
    # Return the list of rois
    return returnRects
    

#If we are in main program
if __name__ == "__main__":
    # Define the key variable assigned to waitKey
    key = ''
    
    abs_path = os.path.dirname(__file__)
    rel_path = 'img/arnie.png'
    full_path = os.path.join(abs_path, rel_path)
    
    img = cv.imread(full_path)
    img_clone = img.copy()
    
    returnList = fishML(img)
    print(returnList)
        
    while(key != ord('q')):
        # Parse through the list of faces
        for objInfo in returnList:
            x = objInfo[0]
            y = objInfo[1]
            w = objInfo[2]
            h = objInfo[3]            
            cv.rectangle(img, (x, y), (x+w, y+h), (0, 255, 0), 2)
            cv.putText(img, "Score: " + str(objInfo[4]), (x+10, y+40), cv.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 0), 2)
        
        # Display the image
        cv.imshow('Image', img)

        # Wait for a key press
        key = cv.waitKey(0)

    cv.destroyAllWindows()
    

