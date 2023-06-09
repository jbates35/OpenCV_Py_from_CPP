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

    _MLReady=false;

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

    _videoCanRun = true;

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

    thread updateThread(_updateThread, this);   
    updateThread.detach();
   
    return 0;

}

int fishMLBase::run()
{
    // Start threads
    if (millis() - _fishTimers["draw"] > (double) 1000.0/DRAW_FPS)
    {
        _fishTimers["draw"] = millis();
	    _getFrame();

        thread drawThread(_drawThread, this);
        drawThread.detach();
    }

    return 0;
}

int fishMLBase::_update()
{
    // double localTimer = millis();
	while(1)
	{
    //     cout << "Loop time: " << millis() - localTimer << endl;
    //     localTimer = millis();

        if (millis() - _fishTimers["update"] > (double) 1000.0/UPDATE_FPS)
        {
            _fishTimers["update"] = millis();

            thread trackerThread(_trackerUpdateThread, this);
            trackerThread.detach();

            thread MLThread (_MLUpdateThread, this);
            MLThread.detach();
        }


        // _trackerUpdate();
        // _MLUpdate();

        if(!_videoCanRun)
        {
            break;
        }
    }

    return 0;
}

void fishMLBase::_updateThread(fishMLBase *fishMLBasePtr)
{
    fishMLBasePtr->_update();
}

int fishMLBase::_getFrame()
{
    Mat localFrame;
    {
        //scoped_lock lock(_frameMutex);
        // Read frame
        _cap >> localFrame;

        // Check if frame is empty
        if (localFrame.empty())
        {
            if (_testMode == TestMode::ON) cout << "End of video" << endl;
            _videoCanRun = false;
            return -1;
        }

        resize(localFrame, localFrame, Size(), _scaleFactor, _scaleFactor);
    }

    {
        unique_lock lock(_frameMutex, try_to_lock);
        if (lock.owns_lock())
        {
            _frame = localFrame;
        }
        else
        {
            cout << "Frame skipped" << endl;
        }
    }
    _trackerCanRun = true;
    _frameCount++;

    return 0;
}

void fishMLBase::_draw()
{
    std::unique_lock<std::mutex> singleLock(_singletonDraw);
    
    if(!singleLock.owns_lock())
    {
        return;
    }

    // Make copy of frame for drawing
    Mat frameDraw;
    {
        scoped_lock frameLock(_frameMutex);
        frameDraw = _frame.clone();
    }

    // Draw rectangles around ROIs and display score
    vector<FishMLData> localObjDetectData;
    {
        scoped_lock trackerLock(_roiMutex);
        localObjDetectData = _objDetectData;
    }

    // Draw tracked fish
    vector<TrackedObjectData> localTrackedData;
    int localFishInc, localFishDec;
    {
        scoped_lock trackerLock(_trackerMutex);
        localTrackedData = _trackedData;
        localFishInc = _fishIncremented;
        localFishDec = _fishDecremented;
    }

    if(_testMode == TestMode::ON)
        _timer = millis();

    for (auto obj : localObjDetectData)
    {
        string rectPos = "Pos: <" + to_string(obj.ROI.x) + ", " + to_string(obj.ROI.y) + ">";
        string score = "Score: " + to_string(obj.score);
        string rectArea = "Area: " + to_string(obj.ROI.area());

        cv::rectangle(frameDraw, obj.ROI, Scalar(200, 90, 185), 1);
        
        cv::putText(frameDraw, score, Point(obj.ROI.x + 5, obj.ROI.y + 15), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(155, 255, 255), 1);
        cv::putText(frameDraw, rectPos, Point(obj.ROI.x + 5, obj.ROI.y + 30), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(155, 255, 255), 1);
        cv::putText(frameDraw, rectArea, Point(obj.ROI.x + 5, obj.ROI.y + 45), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(155, 255, 255), 1);
    }

    for (auto fish : localTrackedData)
    {
        Point centerPoint = Point(fish.roi.x + fish.roi.width / 2, fish.roi.y + fish.roi.height / 2);

        string fishPos = "Pos: <" + to_string(centerPoint.x) + ", " + to_string(centerPoint.y) + ">";
        string fishTimer = "Tracked timer: " + to_string(fish.currentTime);

        cv::rectangle(frameDraw, fish.roi, Scalar(0, 255, 0), 1);

        cv::putText(frameDraw, fishTimer, Point(fish.roi.x + 5, fish.roi.y + fish.roi.height - 15), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(155, 255, 255), 1);
        cv::putText(frameDraw, fishPos, Point(fish.roi.x + 5, fish.roi.y + fish.roi.height - 30), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(155, 255, 255), 1);
    }

    // Write fish count
    string fishIncrementedStr = "Fish moving forward: " + to_string(localFishInc);
    cv::putText(frameDraw, fishIncrementedStr, Point(_frameSize.width / 2 + 70, 20), FONT_HERSHEY_COMPLEX, 0.35, Scalar(255, 255, 255), 1);

    string fishDecrementedStr = "Fish moving backward: " + to_string(localFishDec);
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

    if(_testMode == TestMode::ON)
        cout << "Time to draw frame: " << millis() - _timer << "ms" << endl;


}

void fishMLBase::_drawThread(fishMLBase *fishMLBasePtr)
{
//    fishMLBasePtr->_getFrame();
    fishMLBasePtr->_draw();
}

void fishMLBase::_trackerUpdate()
{
    std::unique_lock<std::mutex> singleLock(_singletonTracker, std::try_to_lock);

    if(!singleLock.owns_lock())
    {
        return;
    }
    
    if(_testMode == TestMode::ON)
        cout << "Tracker loop time: " << millis() -  _fishTimers["Tracker"] << "ms" << endl;
        
    _fishTimers["Tracker"] = millis();

    // Copy parameters to local variables to avoid problems in concurrency
    Mat localFrame;
    {
        scoped_lock frameLock(_frameMutex);
        localFrame = _frame.clone();
    }

    vector<TrackedObjectData> localTrackedData;
    int localFishInc, localFishDec;    
    {
        scoped_lock trackerLock(_trackerMutex);
        localTrackedData = _trackedData;
        localFishInc = _fishIncremented;
        localFishDec = _fishDecremented;
    }

    if(_testMode == TestMode::ON)
        _timer = millis();

    if(_fishTracker.update(localFrame, localFishInc, localFishDec, localTrackedData) < 0)
    {
        if (_testMode == TestMode::ON) cout << "Error running fish tracker updater" << endl;
    }

    if(_testMode == TestMode::ON)
        cout << "Time to update tracker: " << millis() - _timer << "ms" << endl;

    if (_testMode == TestMode::ON)
        _timer = millis();

    vector<FishMLData> localObjDetectData;
    bool localMLReady;
    {
        scoped_lock objDetectLock(_roiMutex);
        localObjDetectData = _objDetectData;
        localMLReady = _MLReady;
        _MLReady=false;
    }

    // Run fish tracker
    if (localMLReady)
        if (_fishTracker.generate(localFrame, localObjDetectData) < 0 && _testMode == TestMode::ON)
        {
            cout << "Error running fish tracker" << endl;
        }

    if (_testMode == TestMode::ON)
        cout << "Time to generate fish trackers: " << millis() - _timer << "ms" << endl;


    //Store back into class variables
    {
        scoped_lock trackerLock(_trackerMutex);
        _trackedData = localTrackedData;
        _fishIncremented = localFishInc;
        _fishDecremented = localFishDec;
    }
}

void fishMLBase::_trackerUpdateThread(fishMLBase *fishMLBasePtr)
{
    fishMLBasePtr->_trackerUpdate();
}

void fishMLBase::_MLUpdate()
{
    std::unique_lock<std::mutex> singleLock(_singletonML, std::try_to_lock);

    if(!singleLock.owns_lock())
    {
        return;
    }
    
    if(_testMode == TestMode::ON)
        cout << "ML update loop time: " << millis() -  _fishTimers["ML"] << "ms" << endl;
    
    _fishTimers["ML"] = millis();

    // Copy parameters to local variables to avoid problems in concurrency
    Mat localFrame;
    {
        scoped_lock frameLock(_frameMutex);
        localFrame = _frame.clone();
    }
    
    vector<FishMLData> localObjDetectData;
    {
        scoped_lock objDetectLock(_roiMutex);
        localObjDetectData = _objDetectData;
    }

    if(_testMode == TestMode::ON)
        _timer = millis();

    // Run fishML
    if (_fishMLWrapper.update(localFrame, localObjDetectData) < 0)
    {
        if (_testMode == TestMode::ON) cout << "Error running fishML" << endl;
    }
    
    if (_testMode == TestMode::ON)
        cout << "Time to run fishML: " << millis() - _timer << "ms" << endl;

    // Store data back into class variable
    {
        scoped_lock objDetectLock(_roiMutex);
        _objDetectData = localObjDetectData;
        _MLReady=true;
    }
}

void fishMLBase::_MLUpdateThread(fishMLBase *fishMLBasePtr)
{
    fishMLBasePtr->_MLUpdate();
}