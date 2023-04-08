

# Load the pre-trained object detection model
# this is the one that works best
# model = tf.saved_model.load('/Users/tomkuzma/Downloads/centernet_resnet50v1_fpn_512x512_1')
# model = tf.saved_model.load('/Users/tomkuzma/Downloads/ssd_mobilenet_v2_2')
# model = tf.saved_model.load('/Users/tomkuzma/Downloads/centernet_hourglass_512x512_1')
# model = tf.saved_model.load('/Users/tomkuzma/Downloads/faster_rcnn_resnet152_v1_640x640_1')
# model = tf.saved_model.load('/Users/tomkuzma/Downloads/ssd_mobilenet_v2_fpnlite_320x320_1')
# model = tf.saved_model.load('/Users/tomkuzma/Downloads/content/inference_graph/saved_model')
# model = tf.saved_model.load('/Users/tomkuzma/Downloads/content 3/inference_graph/saved_model')
# model = tf.saved_model.load('/Users/tomkuzma/Downloads/mobilenet_v1/inference_graph/saved_model')

# # Define the video capture object
# cap = cv2.VideoCapture(0)
#
# # Set the resolution of the video capture object
# cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
# cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

# cap = cv2.VideoCapture('/Users/tomkuzma/Documents/BCIT/Term_8/ELEX 7890 - Capstone/Test Footage/drive-download-20230317T171226Z-001/plastic_and_blue_fish_night_1.avi')
# cap = cv2.VideoCapture('/Users/tomkuzma/Documents/BCIT/Term_8/ELEX 7890 - Capstone/Test Footage/drive-download-20230317T171226Z-001/silver_fish_daytime_1.avi')
# cap = cv2.VideoCapture('/Users/tomkuzma/Documents/BCIT/Term_8/ELEX 7890 - Capstone/Test Footage/drive-download-20230317T171226Z-001/blue_fish_daytime_1.avi')
# cap = cv2.VideoCapture('/Users/tomkuzma/Documents/BCIT/Term_8/ELEX 7890 - Capstone/Test Footage/grey_fish_night.mov')
# cap = cv2.VideoCapture('/Users/tomkuzma/Documents/BCIT/Term_8/ELEX 7890 - Capstone/Test Footage/blue_fish_night_trim.mov')
# cap = cv2.VideoCapture('/Users/tomkuzma/Documents/BCIT/Term_8/ELEX 7890 - Capstone/Hatchery Photos/fishBump_noLED2.mp4')
# cap = cv2.VideoCapture('/Users/tomkuzma/Documents/BCIT/Term_8/ELEX 7890 - Capstone/Hatchery Photos/fishDirect_LED.mp4')
# cap = cv2.VideoCapture('/Users/tomkuzma/Downloads/fiush tirme.mov')
# cap = cv2.VideoCapture('/Users/tomkuzma/Downloads/fish_dark.mov')
# cap = cv2.VideoCapture('/Users/tomkuzma/Documents/BCIT/Term_8/ELEX 7890 - Capstone/hatcheryVid_Apr5/2023_04_05_10h39m33s(good back and forth).mov')
# cap = cv2.VideoCapture('/Users/tomkuzma/Documents/BCIT/Term_8/ELEX 7890 - Capstone/hatcheryVid_Apr5/2023_04_05_10h49m40s.mov')
# cap = cv2.VideoCapture('/Users/tomkuzma/Documents/BCIT/Term_8/ELEX 7890 - Capstone/hatcheryVid_Apr5/2023_04_05_10h58m22s.avi')
# cap = cv2.VideoCapture('/Users/tomkuzma/Documents/BCIT/Term_8/ELEX 7890 - Capstone/hatcheryVid_Apr5/2023_04_05_10h48m38s.mov')
# cap = cv2.VideoCapture('/Users/tomkuzma/Documents/BCIT/Term_8/ELEX 7890 - Capstone/hatcheryVid_Apr5/2023_04_05_10h36m00s.avi')

# cap = cv2.VideoCapture('/Users/tomkuzma/Documents/BCIT/Term_8/ELEX 7890 - Capstone/Test Footage/Trimmed/fish_dark_trimmed_2.mov')
# cap = cv2.VideoCapture('/Users/tomkuzma/Documents/BCIT/Term_8/ELEX 7890 - Capstone/Test Footage/Trimmed/fish_light_trimmed_1.mov')

############################################################################################

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

video_rel_path = 'vid/fishVid_Dark1.mp4'
video_path = os.path.join(cwd, video_rel_path)
cap = cv2.VideoCapture(video_path)

testing = True

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



# Define the object detection function
def detect_objects(image, model):
    # Convert the image to a tensor
    image = tf.convert_to_tensor(image)

    image = tf.convert_to_tensor(image, dtype=tf.uint8)
    image = tf.expand_dims(image, axis=0)
    # image = tf.cast(image, dtype=tf.uint8)

    # Run the object detection model on the image
    output_dict = model(image)
    # Extract the detection results from the output dictionary
    boxes = output_dict['detection_boxes'][0].numpy()
    scores = output_dict['detection_scores'][0].numpy()
    classes = output_dict['detection_classes'][0].numpy().astype(np.int32)
    # Return the detection results
    return boxes, scores, classes

# Run the object detection on each frame of the video stream
# while True:
    # Read a frame from the video stream
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
        boxes, scores, classes = detect_objects(frame, model)
        endTimer = time.time()
        
        if testing:
            print("Time to detect objects: ", 1000*(endTimer - startTimer), "ms")
        
        # Display the detection results on the frame
        startTimer = time.time()
        for i in range(len(boxes)):
            if scores[i] > 0.45:
                ymin, xmin, ymax, xmax = boxes[i]
                xmin = int(xmin * frame.shape[1])
                xmax = int(xmax * frame.shape[1])
                ymin = int(ymin * frame.shape[0])
                ymax = int(ymax * frame.shape[0])
                cv2.rectangle(frame, (xmin, ymin), (xmax, ymax), (0, 255, 0), 2)
                # cv2.putText(frame, f'{class_dict[classes[i]]}:{scores[i]:.2f}', (xmin, ymin-5), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)
                cv2.putText(frame, f'{"Fish"}:{scores[i]:.2f}', (xmin, ymin-5), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

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
