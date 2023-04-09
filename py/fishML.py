import cv2
import tensorflow as tf
import numpy as np
import os
import time

cwd = os.getcwd()

label_rel_path = 'labels/mscoco_label_map.pbtxt.txt'
label_path = os.path.join(cwd, label_rel_path)

model_rel_path = 'models/fishModel1' # this one works best
model = tf.saved_model.load(os.path.join(cwd, model_rel_path))

# Define the object detection function
def fishML(image):
    # This will get returned at the end
    returnRects = []
    
    # Convert the image to a tensor
    image = tf.convert_to_tensor(image)
    image = tf.convert_to_tensor(image, dtype=tf.uint8)
    image = tf.expand_dims(image, axis=0)
    
    # Run the object detection model on the image
    output_dict = model(image)
    
    # Extract the detection results from the output dictionary
    boxes = output_dict['detection_boxes'][0].numpy()
    scores = output_dict['detection_scores'][0].numpy()
    
    frame_width, frame_height = image.shape[2], image.shape[1]
    
    objects_found = False
    
    for (box, score) in zip(boxes, scores):
        # We need to filter out the results that are below the confidence threshold
        if(score < 0.5):
            continue
        
        objects_found = True
        
        # Now that we have the results, we need to convert them to the format that OpenCV uses
        ymin, xmin, ymax, xmax = box
        x = int(xmin * frame_width)
        y = int(ymin * frame_height)
        w = int((xmax - xmin) * frame_width)
        h = int((ymax - ymin) * frame_height)
        returnRects.append([x, y, w, h, score])
    
    if not objects_found:
        returnRects.append([-1, -1,-1, -1, -1])
    
    return returnRects

if __name__ == '__main__':
    cap = cv2.VideoCapture(cwd + '/vid/fishVidDark1.mp4')
    testing = False
    
    # Parse the label map file
    with open(label_path, 'r') as f:
        lines = f.readlines()

    # Create a dictionary that maps class IDs to their names
    class_dict = {}
    for line in lines:
        if 'id:' in line:
            class_id = int(line.split(':')[1])
        elif 'display_name:' in line:
            class_name = line.split(':')[1].strip().replace("'", "")
            class_dict[class_id] = class_name

    while (cap.isOpened()):
        
        #Start timer
        startTimer = time.time()
        
        ret, frame = cap.read()
        
        endTimer = time.time()
        if testing:
            print("Time to read frame: ", 1000*(endTimer - startTimer), "ms")
        
        if ret == True:
            startTimer = time.time()
            
            # Run the object detection on the frame
            objects_found = fishML(frame)
            
            endTimer = time.time()
            
            if testing:
                print("Time to detect objects: ", 1000*(endTimer - startTimer), "ms")
            
            # Display the detection results on the frame
            startTimer = time.time()

            for x, y, w, h, score in objects_found:
                if(x == -1):
                    break

                # Draw a bounding box around the detected object
                cv2.rectangle(frame, (x, y), (x+w, y+h), (0, 255, 0), 2)
                cv2.putText(frame, f'{"Fish"}:{score:.2f}', (x, y-5), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

            # Display the frame
            cv2.imshow('Object Detection', frame)
            
            # Exit the loop if the 'q' key is pressed
            if cv2.waitKey(1) == ord('q'):
                break
            
            endTimer = time.time()
            if testing:
                print("Time to display frame: ", 1000*(endTimer - startTimer), "ms")

    # Release the video capture object and close the display window
    cap.release()
    cv2.destroyAllWindows()
