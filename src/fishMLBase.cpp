#include "fishMLBase.h"

using namespace cv;
using namespace std;
using namespace FishMLHelper;

namespace fs = std::filesystem;

fishMLBase::fishMLBase()
{
    _testMode = TestMode::OFF;
    _videoWriteMode = VideoWriteMode::OFF;
}

fishMLBase::~fishMLBase()
{
    if(_videoWriteMode == VideoWriteMode::ON)
    {
        _videoWriter.release();
    }

    _cap.release(); 
    destroyAllWindows();
}

int fishMLBase::init()
{
    _returnKey  = 0;
    _fishIncremented = 0;
    _fishDecremented = 0;

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

    // Get frame size
    _cap >> _frame;
    _frameSize = Size(_frame.cols, _frame.rows);
    _frameCount = 0;
    _frameTotal = _cap.get(CAP_PROP_FRAME_COUNT);

    // Return to beginning of video
    _cap.set(CAP_PROP_POS_FRAMES, 0);

    if(_testMode == TestMode::ON) _timer = millis();

    // Initialize Python environment and module
    if(_fishMLWrapper.init()<0) 
    {
        cout << "Error initializing fishML" << endl;
        return -1;
    }   

    if(_testMode == TestMode::ON) cout << "Time to initialize fish object detection: " << millis()-_timer << "ms" << endl;

    // Initialize fish tracker
    if(_testMode == TestMode::ON) _timer = millis();

    if(_fishTracker.init(_frameSize) < 0)
    {
        cout << "Error initializing fish tracker" << endl;
        return -1;
    }

    if(_testMode == TestMode::ON) cout << "Time to initialize fish tracker: " << millis()-_timer << "ms" << endl;


    // Initialize video writer
    if(_videoWriteMode == VideoWriteMode::ON)
    {
        string videoName = _selectedVideo.substr(_selectedVideo.find_last_of("/\\")+1);
        string videoPath = "./vid/" + videoName.substr(0, videoName.find_last_of(".")) + "_out.avi";
        _videoWriter.open(videoPath, VideoWriter::fourcc('M','J','P','G'), 30, _frameSize);
        if(!_videoWriter.isOpened())
        {
            cout << "Error opening video writer" << endl;
            return -1;
        }
    }

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
    _frameCount++;

    // Check if frame is empty
    if(_frame.empty()) 
    {
        cout << "End of video" << endl;
        return -1;
    }

    if(_testMode == TestMode::ON) _timer = millis();

    // Run fishML
    if(_fishMLWrapper.update(_frame, _objDetectData)<0) 
    {
        cout << "Error running fishML" << endl;
        return -1;
    }

    if(_testMode == TestMode::ON) cout << "Time to run fishML: " << millis()-_timer << "ms" << endl;
    if(_testMode == TestMode::ON) _timer = millis();

    // Run fish tracker
    if(_fishTracker.run(_frame, _trackerMutex, _fishIncremented, _fishDecremented, _objDetectData, _trackedData) < 0)
    {
        cout << "Error running fish tracker" << endl;
        return -1;
    }

    if(_testMode == TestMode::ON) cout << "Time to run fish tracker: " << millis()-_timer << "ms" << endl;
    return 0;
}

void fishMLBase::_draw()
{
    // Draw rectangles around ROIs and display score
    for(auto obj : _objDetectData)
    {
        string rectPos = "Pos: <" + to_string(obj.ROI.x) + ", " + to_string(obj.ROI.y) + ">";
        string score = "Score: " + to_string(obj.score);
        rectangle(_frame, obj.ROI, Scalar(200, 90, 185), 1);
        putText(_frame, score, Point(obj.ROI.x+5, obj.ROI.y+15), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(155, 255, 255), 1);
        putText(_frame, rectPos, Point(obj.ROI.x+5, obj.ROI.y+30), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(155, 255, 255), 1);
    }

    // Draw tracked fish
    for(auto fish : _trackedData)
    {
        scoped_lock lock(_trackerMutex);

        Point centerPoint = Point(fish.roi.x + fish.roi.width/2, fish.roi.y + fish.roi.height/2);

        string fishPos = "Pos: <" + to_string(centerPoint.x) + ", " + to_string(centerPoint.y) + ">";
        string fishTimer = "Tracked timer: " + to_string(fish.currentTime);

        rectangle(_frame, fish.roi, Scalar(0, 255, 0), 1);

        putText(_frame, fishTimer, Point(fish.roi.x+5, fish.roi.y+fish.roi.height - 15), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(155, 255, 255), 1);
        putText(_frame, fishPos, Point(fish.roi.x+5,  fish.roi.y+fish.roi.height - 30), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(155, 255, 255), 1);
    }

    // Write fish count
    string fishIncrementedStr = "Fish moving forward: " + to_string(_fishIncremented);
    putText(_frame, fishIncrementedStr, Point(_frameSize.width/2 + 70, 20), FONT_HERSHEY_COMPLEX, 0.65, Scalar(255, 255, 255), 2);

    string fishDecrementedStr = "Fish moving backward: " + to_string(_fishDecremented);
    putText(_frame, fishDecrementedStr, Point(70, 20), FONT_HERSHEY_COMPLEX, 0.65, Scalar(255, 255, 255), 2);

    //Draw line down middle
    line(_frame, Point(_frameSize.width/2, 0), Point(_frameSize.width/2, _frameSize.height), Scalar(150, 255, 150), 1);

    // Write frame to video
    if(_videoWriteMode == VideoWriteMode::ON)
    {
        _videoWriter.write(_frame);

        //Write progress bar to screen
        int barWidth = 70;
        float progress = (float)_frameCount/(float)_frameTotal;
        int pos = barWidth * progress;
        cout << "[";
        for (int i = 0; i < barWidth; ++i) 
        {
            if (i < pos) cout << "=";
            else if (i == pos) cout << ">";
            else cout << " ";
        }
        cout << "] " << int(progress * 100.0) << " %\r";
    }
    else 
    {
        // Display frame
        imshow("Video: " + _selectedVideo, _frame);
        // Press ESC on keyboard to exit
        _returnKey = waitKey(25);
    }
}