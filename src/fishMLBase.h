#pragma once

#include <iostream>

#include <opencv2/opencv.hpp>
#include <filesystem>

#include "fishMLWrapper.h"
#include "helperfunc.h"

enum class TestMode
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
    char getReturnKey() { return _returnKey; }

private:
    int _update();
    void _draw();

    FishMLWrapper _fishMLWrapper;
    vector<FishMLData> _objData;
    Mat _frame;
    VideoCapture _cap;

    string _selectedVideo;

    double _timer;
    char _returnKey;

    TestMode _testMode;
};