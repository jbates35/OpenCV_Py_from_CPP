#include <iostream>

#include <opencv2/opencv.hpp>

#include "fishMLWrapper.h"

FishMLWrapper fishml;

using namespace std;

int main() {
    
    vector<Rect> ROIs;

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
    if(fishml.update(img, ROIs)<0) {
        cout << "Error updating fishML" << endl;
        return -1;
    }

    // Draw rectangles around ROIs
    for(auto ROI : ROIs)
    {
        string rectPos = "Pos: <" + to_string(ROI.x) + ", " + to_string(ROI.y) + ">";
        putText(img, rectPos, Point(ROI.x, ROI.y-10), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(155, 255, 255), 1);
        rectangle(img, ROI, Scalar(200, 90, 185), 2);
    }

    // Display image
    char keyPress = 0;
    while(keyPress != 27) {
        imshow("Image", img);
        keyPress = waitKey(1);
    }

    return 0;
}