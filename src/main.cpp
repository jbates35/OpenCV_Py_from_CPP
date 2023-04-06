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

    vector<Mat> imgs;

    Mat img = imread("img/many_faces_4.jpg");
    imgs.push_back(img.clone());
    int i = 0;

    for(Mat img : imgs)
    {
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

        cout << "Time to update fishML for image " + to_string(++i) + ": " << millis()-timer << "ms" << endl;

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
    }

    for(Mat img : imgs)
    {
        // Show image
        imshow("Image", img);
    }
    
    waitKey(0);
    return 0;
}