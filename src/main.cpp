#include <iostream>

#include <opencv2/opencv.hpp>

#include "fishMLWrapper.h"

FishMLWrapper fishml;

using namespace std;

double millis() {
    return 1000*getTickCount()/getTickFrequency();
}

int main() {
    
    vector<FishMLData> objData;

    double timer = millis();

    // Initialize Python environment and module
    if(fishml.init()<0) {
        cout << "Error initializing fishML" << endl;
        return -1;
    }

    cout << "Time to initialize fishML: " << millis()-timer << "ms" << endl;

    vector<string> imgPaths = {
        "img/arnie_large.png",
        "img/arnie_large_2.png",
        "img/arnie_large_3.png"
    };

    Mat img;
    char returnKey=0;
    
    double imgTimer = millis();

    int numberOfIterations = 0;
    int i = 0;
    int iMax = imgPaths.size();

    while(returnKey != 27)
    {
        numberOfIterations++;
        cout << "Number of iterations: " << numberOfIterations << endl;
        cout << "Current image " << imgPaths[i] << endl;

        if (returnKey == 27) {
            break;
        }

        // Increment image every 500ms
        if(millis() - imgTimer > 1500) {
            cout << "Increasing image" << endl;
            imgTimer = millis();

            i++;

            if(i >= iMax) {
                i = 0;
            }
        }

        img = imread(imgPaths[i]);

        // Express size of image
        cout << "Image width = " << img.size().width << endl;
        cout << "Image height = " << img.size().height << endl;

        // Check if image is empty
        if(img.empty()) {
            cout << "Error loading image" << endl;
            return -1;
        }

        timer = millis();

        // Connect to Python and find face
        if(fishml.update(img, objData)<0) {
            cout << "Error updating fishML" << endl;
            return -1;
        }

        //Write the rectangle to screen for test
        cout << "Number of rectangles: " << objData.size() << endl;

        // Draw rectangles around ROIs
        for(auto obj : objData)
        {
            string rectPos = "Pos: <" + to_string(obj.ROI.x) + ", " + to_string(obj.ROI.y) + ">";
            string score = "Score: " + to_string(obj.score);

            cout << rectPos << endl;
            cout << score << endl;

            // Draw rectangle
            rectangle(img, obj.ROI, Scalar(200, 90, 185), 2);

            // Put information
            putText(img, score, Point(obj.ROI.x+5, obj.ROI.y+15), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(155, 255, 255), 1);
            putText(img, rectPos, Point(obj.ROI.x+5, obj.ROI.y+30), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(155, 255, 255), 1);
        }

        // Display image
        imshow("Image", img);
        returnKey = waitKey(100);
        
    }
    return 0;
}