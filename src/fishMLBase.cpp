#include "fishMLBase.h"

using namespace cv;
using namespace std;
using namespace FishMLHelper;

namespace fs = std::filesystem;

fishMLBase::fishMLBase()
{
}

fishMLBase::~fishMLBase()
{
    destroyAllWindows();
}

int fishMLBase::init()
{
    // List all files
    if(getVideoEntry(_selectedVideo) < 0)
    {   
        cout << "Error getting video entry" << endl;
        return -1;
    }

    // Initialize the video capture
    _cap.open(_selectedVideo, CAP_FFMPEG);   
    if(!_cap.isOpened()) 
    {
        cout << "Error opening video stream or file" << endl;
        return -1;
    }

    if(_testMode == TestMode::ON) _timer = millis();

    // Initialize Python environment and module
    if(_fishMLWrapper.init()<0) 
    {
        cout << "Error initializing fishML" << endl;
        return -1;
    }

    if(_testMode == TestMode::ON) cout << "Time to initialize fishML: " << millis()-_timer << "ms" << endl;

    return 0;
}

int fishMLBase::run()
{
    if(_update() < 0) 
    {
        cout << "Error updating fishML" << endl;
        return -1;
    }

    _draw();

    return 0;
}

int fishMLBase::_update()
{
    // Read frame
    _cap >> _frame;

    // Check if frame is empty
    if(_frame.empty()) 
    {
        cout << "End of video" << endl;
        return -1;
    }

    if(_testMode == TestMode::ON) _timer = millis();

    // Run fishML
    if(_fishMLWrapper.update(_frame, _objData)<0) 
    {
        cout << "Error running fishML" << endl;
        return -1;
    }

    if(_testMode == TestMode::ON) cout << "Time to run fishML: " << millis()-_timer << "ms" << endl;

    return 0;
}

void fishMLBase::_draw()
{
    // Draw rectangles around ROIs and display score
    for(auto obj : _objData)
    {
        string rectPos = "Pos: <" + to_string(obj.ROI.x) + ", " + to_string(obj.ROI.y) + ">";
        string score = "Score: " + to_string(obj.score);
        rectangle(_frame, obj.ROI, Scalar(200, 90, 185), 2);
        putText(_frame, score, Point(obj.ROI.x+5, obj.ROI.y+15), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(155, 255, 255), 1);
        putText(_frame, rectPos, Point(obj.ROI.x+5, obj.ROI.y+30), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(155, 255, 255), 1);
    }

    // Display frame
    imshow("Video: " + _selectedVideo, _frame);

    // Press ESC on keyboard to exit
    _returnKey = waitKey(25);
}