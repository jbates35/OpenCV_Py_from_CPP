#include <iostream>

#include <opencv2/opencv.hpp>
#include <filesystem>

#include "fishMLBase.h"
#include "fishMLWrapper.h"

using namespace cv;
using namespace std;

namespace fs = std::filesystem;

double millis() {
    return 1000*getTickCount()/getTickFrequency();
}

int main(int argc, char** argv) {

    fishMLBase fishml;

    for(int i = 0; i < argc; i++)
    {
        string arg = argv[i];

        if(arg == "-t")
        {
            cout << "Test mode on" << endl;
            fishml.setTestMode(TestMode::ON);
        }
        else if(arg == "-w")
        {
            cout << "Video write mode on" << endl;
            fishml.setVideoWriteMode(VideoWriteMode::ON);
        }
    }
    
    if(fishml.init() < 0)
    {
        cout << "Error initializing fishML from main" << endl;
        return -1;
    }


    while(fishml.getReturnKey() != 27 /*ESC*/ && fishml.getReturnKey() != 'q' && fishml.getReturnKey() != 'Q')
    {
        if(fishml.run() < 0)
        {
            cout << "Exiting run from main" << endl;
            return -1;
        }
    }

    return 0;
}