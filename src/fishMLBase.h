#pragma once

#include <iostream>

#include <opencv2/opencv.hpp>
#include <opencv2/video.hpp>

#include <mutex>
#include <filesystem>

#include "fishMLWrapper.h"
#include "helperfunc.h"
#include "fishTracker.h"

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

private:
    int _update();
    void _draw();

    FishMLWrapper _fishMLWrapper;
    vector<FishMLData> _objDetectData;
    Mat _frame;
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

    mutex _trackerMutex;
};