#include <iostream>

#include <opencv2/opencv.hpp>

#include "fishMLWrapper.h"

FishMLWrapper fishml;

using namespace std;

int main() {
    
    vector<FishMLData> objData;

    // Read image arnie.png
    Mat img = imread("img/arnie.png");

    // Check if image is empty
    if(img.empty()) {
        cout << "Error loading image" << endl;
        return -1;
    }

    // Initialize Python environment and module
    if(fishml.init()<0) {
        cout << "Error initializing fishML" << endl;
        return -1;
    }

    // Connect to Python and find face
    if(fishml.update(img, objData)<0) {
        cout << "Error updating fishML" << endl;
        return -1;
    }

    // Draw rectangles around ROIs
    for(auto obj : objData)
    {
        string rectPos = "Pos: <" + to_string(obj.ROI.x) + ", " + to_string(obj.ROI.y) + ">";
        string score = "Score: " + to_string(obj.score);

        // Draw rectangle
        rectangle(img, obj.ROI, Scalar(200, 90, 185), 2);

        // Put information
        putText(img, score, Point(obj.ROI.x+5, obj.ROI.y+15), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(155, 255, 255), 1);
        putText(img, rectPos, Point(obj.ROI.x+5, obj.ROI.y+30), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(155, 255, 255), 1);
    }

    // Display image
    char keyPress = 0;
    while(keyPress != 27) {
        imshow("Image", img);
        keyPress = waitKey(1);
    }

    return 0;
}