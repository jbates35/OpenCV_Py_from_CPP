#pragma once

#include <iostream>

#include <opencv2/opencv.hpp>
#include <opencv2/video.hpp>

#include <mutex>
#include <filesystem>

#include "fishMLWrapper.h"
#include "helperfunc.h"
#include "fishTracker.h"

const int FRAME_MAX_HEIGHT = 300;
const double DRAW_FPS = 30;

using namespace std;

enum class TestMode
{
    OFF,
    ON
};

enum class VideoWriteMode
{
    OFF,
    ON
};

class fishMLBase
{
public:
    fishMLBase();
    ~fishMLBase();

    int init();
    int run();

    // Getters and setters
    void setTestMode(TestMode testMode) { _testMode = testMode; }
    void setVideoWriteMode(VideoWriteMode videoWriteMode) { _videoWriteMode = videoWriteMode; }
    char getReturnKey() { return _returnKey; }
    bool videoCanRun() { return _videoCanRun; }

private:
    int _update();
    static void _updateThread(fishMLBase* fishMLBasePtr);

    int _getFrame();
    void _draw();
    static void _drawThread(fishMLBase* fishMLBasePtr);

    void _trackerUpdate();
    static void _trackerUpdateThread(fishMLBase* fishMLBasePtr);

    void _MLUpdate();
    static void _MLUpdateThread(fishMLBase* fishMLBasePtr);

    FishMLWrapper _fishMLWrapper;
    vector<FishMLData> _objDetectData;

    map<string, double> _fishTimers;

    std::mutex _frameMutex, _drawMutex, _trackerMutex, _updateMutex, _runMutex, _roiMutex, _throwawayMutex;
    std::mutex _singletonTracker, _singletonML;
    
    Mat _frame, _framePrev;
    double _scaleFactor;
    VideoCapture _cap;
    VideoWriter _videoWriter;
    int _frameCount;
    int _frameTotal;

    FishTracker _fishTracker;
    vector<FishTrackerStruct> _trackedData;
    vector<Rect> _detectedFishROI;
    int _fishIncremented;
    int _fishDecremented;

    string _selectedVideo;

    double _timer;
    char _returnKey;

    TestMode _testMode;
    VideoWriteMode _videoWriteMode;

    Size _frameSize;

    bool _videoCanRun;
    bool _trackerCanRun;
    bool _trackerRunning;

    bool _updateRunning;
    bool _drawRunning;
};