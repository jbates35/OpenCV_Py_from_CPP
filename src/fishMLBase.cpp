#include "fishMLBase.h"

#include <thread>

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
    if (_videoWriteMode == VideoWriteMode::ON)
    {
        _videoWriter.release();
    }

    _cap.release();
    destroyAllWindows();
}

int fishMLBase::init()
{
    _returnKey = 0;
    _fishIncremented = 0;
    _fishDecremented = 0;

    _updateRunning = false;
    _drawRunning = false;

    _videoCanRun = true;
    _trackerCanRun = false;
    _trackerRunning = false;

    // List all files
    if (getVideoEntry(_selectedVideo) < 0)
    {
        cout << "Error getting video entry" << endl;
        return -1;
    }

    // Initialize the video capture
    _cap.open(_selectedVideo, CAP_FFMPEG);
    if (!_cap.isOpened())
    {
        cout << "Error opening video stream or file" << endl;
        return -1;
    }

    // Resize frame to fit screen
    _cap >> _frame;
    int frameHeight = _frame.size().height;
    _scaleFactor = (double)FRAME_MAX_HEIGHT / (double)frameHeight;
    resize(_frame, _frame, Size(), _scaleFactor, _scaleFactor);

    // Get frame size
    _frameSize = Size(_frame.cols, _frame.rows);
    _frameCount = 0;
    _frameTotal = _cap.get(CAP_PROP_FRAME_COUNT);

    // Return to beginning of video
    _cap.set(CAP_PROP_POS_FRAMES, 0);

    if (_testMode == TestMode::ON)
        _timer = millis();

    // Initialize Python environment and module
    if (_fishMLWrapper.init() < 0)
    {
        cout << "Error initializing fishML" << endl;
        return -1;
    }

    if (_testMode == TestMode::ON)
        cout << "Time to initialize fish object detection: " << millis() - _timer << "ms" << endl;

    // Initialize fish tracker
    if (_testMode == TestMode::ON)
        _timer = millis();

    if (_fishTracker.init(_frameSize) < 0)
    {
        cout << "Error initializing fish tracker" << endl;
        return -1;
    }

    if (_testMode == TestMode::ON)
        cout << "Time to initialize fish tracker: " << millis() - _timer << "ms" << endl;

    // Initialize video writer
    if (_videoWriteMode == VideoWriteMode::ON)
    {
        string videoName = _selectedVideo.substr(_selectedVideo.find_last_of("/\\") + 1);
        string videoPath = "./vid/" + videoName.substr(0, videoName.find_last_of(".")) + "_out.avi";
        _videoWriter.open(videoPath, VideoWriter::fourcc('M', 'J', 'P', 'G'), 30, _frameSize);
        if (!_videoWriter.isOpened())
        {
            cout << "Error opening video writer" << endl;
            return -1;
        }
    }

    // initialize timers
    _fishTimers["draw"] = millis();
    _fishTimers["update"] = millis();

    return 0;
}

int fishMLBase::run()
{
    // Start threads
    if (!_drawRunning && millis() - _fishTimers["draw"] > 1000/DRAW_FPS)
    {
        _fishTimers["draw"] = millis();

        thread drawThread(_drawThread, this);
        drawThread.detach();
    }

    if(_trackerCanRun && !_updateRunning)
    {
        thread updateThread(_updateThread, this);
        updateThread.detach();
    }

    return 0;
}

int fishMLBase::_update()
{
    std::lock_guard<std::mutex> updateGuard(_updateMutex); 

    //cout << "Entering update" << endl;
    if(millis() - _fishTimers["update"] > 5)
    {
        cout << "Millis since last running: " << millis() - _fishTimers["update"] << endl;
    }
    _fishTimers["update"] = millis(); 

    Mat frameTrack;
    vector<FishMLData> localObjDetectData;
    vector<FishTrackerStruct> localTrackedData;

    {
        scoped_lock<mutex> frameLock(_frameMutex);
        frameTrack = _frame.clone();
    }
    
    if(frameTrack.empty())
    {
        cout << "Error cloning frame" << endl;
        _trackerRunning = false;
        return -1;
    }
    
    if (_testMode == TestMode::ON)
        _timer = millis();


    // Run fishML
    {
        scoped_lock<mutex> roiGuard(_roiLock);

        if (_fishMLWrapper.update(frameTrack, localObjDetectData) < 0)
        {
            cout << "Error running fishML" << endl;
            _videoCanRun = false;
            return -1;
        }
    }
    if (_testMode == TestMode::ON)
        cout << "Time to run fishML: " << millis() - _timer << "ms" << endl;
    if (_testMode == TestMode::ON)
        _timer = millis();

    

    // Run fish tracker
    if (_fishTracker.run(frameTrack, _trackerMutex, _fishIncremented, _fishDecremented, _objDetectData, _trackedData) < 0)
    {
        cout << "Error running fish tracker" << endl;
        _videoCanRun = false;
        return -1;
    }

    if (_testMode == TestMode::ON)
        cout << "Time to run fish tracker: " << millis() - _timer << "ms" << endl;
    
    
    return 0;
}

void fishMLBase::_updateThread(fishMLBase *fishMLBasePtr)
{
    fishMLBasePtr-> _updateRunning = true;
    //scoped_lock<mutex> updateLock(fishMLBasePtr->_updateMutex);
    fishMLBasePtr->_update();
    fishMLBasePtr-> _updateRunning = false;
}

int fishMLBase::_getFrame()
{
    {
        scoped_lock<mutex> lock(_frameMutex);
        // Read frame
        _cap >> _frame;

        // Check if frame is empty
        if (_frame.empty())
        {
            cout << "End of video" << endl;
            _videoCanRun = false;
            return -1;
        }

        resize(_frame, _frame, Size(), _scaleFactor, _scaleFactor);
    }

    _trackerCanRun = true;
    _frameCount++;

    return 0;
}

void fishMLBase::_draw()
{
    Mat frameDraw;

    {
        scoped_lock<mutex> frameLock(_frameMutex);
        // Make copy of frame for drawing
        frameDraw = _frame.clone();
    }
    // Draw rectangles around ROIs and display score
    {
        scoped_lock<mutex> roiGuard(_roiLock);
        for (auto obj : _objDetectData)
        {
            string rectPos = "Pos: <" + to_string(obj.ROI.x) + ", " + to_string(obj.ROI.y) + ">";
            string score = "Score: " + to_string(obj.score);
            cv::rectangle(frameDraw, obj.ROI, Scalar(200, 90, 185), 1);
            cv::putText(frameDraw, score, Point(obj.ROI.x + 5, obj.ROI.y + 15), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(155, 255, 255), 1);
            cv::putText(frameDraw, rectPos, Point(obj.ROI.x + 5, obj.ROI.y + 30), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(155, 255, 255), 1);
        }
    }

    // Draw tracked fish
    {
        scoped_lock lock(_trackerMutex);
        for (auto fish : _trackedData)
        {
            Point centerPoint = Point(fish.roi.x + fish.roi.width / 2, fish.roi.y + fish.roi.height / 2);

            string fishPos = "Pos: <" + to_string(centerPoint.x) + ", " + to_string(centerPoint.y) + ">";
            string fishTimer = "Tracked timer: " + to_string(fish.currentTime);

            cv::rectangle(frameDraw, fish.roi, Scalar(0, 255, 0), 1);

            cv::putText(frameDraw, fishTimer, Point(fish.roi.x + 5, fish.roi.y + fish.roi.height - 15), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(155, 255, 255), 1);
            cv::putText(frameDraw, fishPos, Point(fish.roi.x + 5, fish.roi.y + fish.roi.height - 30), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(155, 255, 255), 1);
        }
    }

    // Write fish count
    string fishIncrementedStr = "Fish moving forward: " + to_string(_fishIncremented);
    cv::putText(frameDraw, fishIncrementedStr, Point(_frameSize.width / 2 + 70, 20), FONT_HERSHEY_COMPLEX, 0.35, Scalar(255, 255, 255), 1);

    string fishDecrementedStr = "Fish moving backward: " + to_string(_fishDecremented);
    cv::putText(frameDraw, fishDecrementedStr, Point(70, 20), FONT_HERSHEY_COMPLEX, 0.35, Scalar(255, 255, 255), 1);

    // Draw line down middle
    cv::line(frameDraw, Point(_frameSize.width / 2, 0), Point(_frameSize.width / 2, _frameSize.height), Scalar(150, 255, 150), 1);

    // Write frame to video
    if (_videoWriteMode == VideoWriteMode::ON)
    {
        _videoWriter.write(frameDraw);

        // Write progress bar to screen
        int barWidth = 70;
        float progress = (float)_frameCount / (float)_frameTotal;
        int pos = barWidth * progress;
        cout << "[";
        for (int i = 0; i < barWidth; ++i)
        {
            if (i < pos)
                cout << "=";
            else if (i == pos)
                cout << ">";
            else
                cout << " ";
        }
        cout << "] " << int(progress * 100.0) << " %\r";
    }
    else
    {
        // Display frame
        imshow("Video: " + _selectedVideo, frameDraw);
        // Press ESC on keyboard to exit
        _returnKey = waitKey(25);
    }
}

void fishMLBase::_drawThread(fishMLBase *fishMLBasePtr)
{
    fishMLBasePtr -> _drawRunning = true;
    scoped_lock<mutex> lock(fishMLBasePtr->_runLock);
    fishMLBasePtr->_getFrame();
    fishMLBasePtr->_draw();
    fishMLBasePtr -> _drawRunning = false;
}
