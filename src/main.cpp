#include <iostream>

#include <opencv2/opencv.hpp>
#include <opencv2/video.hpp>
#include <opencv2/highgui.hpp>
#include <filesystem>

#include "fishMLWrapper.h"

using namespace cv;
using namespace std;

namespace fs = std::filesystem;

double millis() {
    return 1000*getTickCount()/getTickFrequency();
}

int main(int argc, char** argv) {

    // Check if video path is given
    string videoPath;
    if(argc > 1) 
    {
        videoPath = "./vid/" + string(argv[1]);
    }
    else 
    {
        cout << "No video path given" << endl;
        return -1;
    }

    cout << "Using video path: " << videoPath << endl;

    // Get current directory
    string currentDir = fs::current_path();
    cout << "Current directory: " << currentDir << endl;
    cout << "Video location: " << currentDir + "/vid/fishVidDark1.mp4" << endl;

    FishMLWrapper fishml;
    vector<FishMLData> objData;
    Mat frame;
    VideoCapture cap(currentDir + "/vid/fishVidDark1.mp4", cv::CAP_FFMPEG);
    cout << cap.getBackendName() << endl;
    
    // Check if video is opened
    if(!cap.isOpened()) {
        cout << "Error opening video stream or file" << endl;
        return -1;
    }

    double timer = millis();

    // Initialize Python environment and module
    if(fishml.init()<0) {
        cout << "Error initializing fishML" << endl;
        return -1;
    }

    cout << "Time to initialize fishML: " << millis()-timer << "ms" << endl;

    char returnKey=0;
    
    while(returnKey != 27)
    {
        if (returnKey == 27) {
            break;
        }

        // Read frame from video
        if(!cap.read(frame)) {
            cout << "Either video finished, or error reading frame" << endl;
            break;
        }
        
        // Check if image is empty
        if(frame.empty()) {
            cout << "Error loading image" << endl;
            return -1;
        }

        // timer = millis();

        // // Connect to Python and find face
        // if(fishml.update(frame, objData)<0) {
        //     cout << "Error updating fishML" << endl;
        //     return -1;
        // }

        // //Write the rectangle to screen for test
        // cout << "Number of rectangles: " << objData.size() << endl;

        // // Draw rectangles around ROIs
        // for(auto obj : objData)
        // {
        //     string rectPos = "Pos: <" + to_string(obj.ROI.x) + ", " + to_string(obj.ROI.y) + ">";
        //     string score = "Score: " + to_string(obj.score);

        //     cout << rectPos << endl;
        //     cout << score << endl;

        //     // Draw rectangle
        //     rectangle(frame, obj.ROI, Scalar(200, 90, 185), 2);

        //     // Put information
        //     putText(frame, score, Point(obj.ROI.x+5, obj.ROI.y+15), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(155, 255, 255), 1);
        //     putText(frame, rectPos, Point(obj.ROI.x+5, obj.ROI.y+30), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(155, 255, 255), 1);
        // }

        // Display image
        imshow(string(argv[1]), frame);
        returnKey = waitKey(100);        
    }
    
    cv::destroyAllWindows();
    return 0;
}