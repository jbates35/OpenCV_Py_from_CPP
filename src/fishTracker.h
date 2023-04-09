#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>

#include <mutex>

#include <thread>

#include "fishMLWrapper.h"
#include "helperfunc.h"

using namespace cv;
using namespace std;


//Minimum area to initiate tracker to
#define DEFAULT_MIN_AREA 400
#define DEFAULT_MAX_AREA 25000

//Minimum area of combined rect before it's considered "overlapping" 
#define DEFAULT_COMBINED_RECT_AREA 500

//Default region of ROI 
#define DEFAULT_RECT_SCALE 0.7

//Proportional margin of the camera frame that will be 
#define DEFAULT_MARGIN 0.05

//When an object is lost, the amount of pixels surrounding the last ROI it will look for an untracked object
#define DEFAULT_RETRACK_PIXELS 150

//Amount of frames occlusion will try to re-track an object for, before deleting from the tracking vector
#define DEFAULT_RETRACK_FRAMES 10

//File path logger gets saved to by default
#define DEFAULT_FILE_PATH "./"

//Maximum amount of time object can be tracked before it's dropped from the tracker
#define MAX_TRACKER_DURATION 30 //(seconds)

enum
{
	RANGE_MIN,
	RANGE_MAX
};

enum
{
	OFF,
	ON		
};

enum class ftMode
{   
	TRACKING,
	CALIBRATION
};

enum class imgMode
{
	BGR,
	HSV,
	BINARY
};

namespace _ft
{		
	//Max size of the vector that holds all the data, to prevent wildly large files
	const int MAX_DATA_SIZE = 5000;
	
	//Extra X vals to test reflection of
	const int EXTRA_REFLECT_WIDTH = 10;
	
	//Amount of time tracker can last before deleting object
	const double TRACKER_TIMEOUT_MILLIS = 10000;

	//Default area of combined rect before it's considered "overlapping"
	const int DEFAULT_COMBINED_RECT_AREA_PROPORTION = 0.3;
		
	//Struct keeping track of parameters of fish
	struct FishTrackerStruct
	{
		Ptr<Tracker> tracker;
		vector<int> posX;
		bool isTracked;
		Rect roi;
		int lostFrameCount;
		double startTime;
		double currentTime;
	};

	//Struct for passing additional mats
	struct returnMatsStruct
	{
		string title;
		Mat mat;
		imgMode colorMode;
	};
}

using namespace _ft;

/**
 * @brief Fish Tracking class for Fish-CenS
 * @details Class that  you can pass a CV MAT and then allows you to track objects using KCF.
 * @author Fish-CenS - Jimmy Bates
 * @date January 29 2023
 *
 **/
class FishTracker
{
public:
    /**
     * @brief Constructor, does nothing, as init() is used
     **/
	FishTracker();

    /**
     * @brief Destructor does nothing currently
     **/
	~FishTracker();
	
    /**
     * @brief Run thread for parent class
     * @param im - Main image from VideoCapture (do video >> frame and pass that)
     * @param imProcessed Image that's processed, mainly for calibrating/testing (think of this as a return)
     * @param lock Mutex passed from parent class
     * @param fishCount Amount of fish counted via 
     * @param ROIRects Vector of ROI rects parent class can have updated
     * @return True if success, false if im is empty 
     **/
	int run(Mat& im, mutex& lock, int& fishIncrement, int& fishDecrement, vector<FishMLData>& detectedObjects, vector<FishTrackerStruct>& trackedObjects);
	
    /**
     * @brief Initializer (And reinitializer) for most important parameters of class
     * @param video_width Width of video
     * @param video_height Height of video
     * @param rangeMin optional scalar for min HSV Values to be threshold'd
     * @param rangeMax optional scalar for max HSV values to be threshold'd
     * @return True if success, false if no frame was passed or empty frame was passed
     **/
	int init(Size frameSize);
	

	/**
    * @brief Setter for margin of the camera shot that occlusion detection works on
    * @param marginProportion Proportion of frame that gets considered edge (default 0.1)
    **/
	void setMargin(float marginProportion);
	
    /**
    * @brief Getter of margin of the camera shot that occlusion detection works on
    * @return Proportion of frame that is considered edge
    **/
	float getMargin();
	
	/**
    * @brief Setter for retrack region - When an object is lost in the middle of the frame, the tracker will attempt to find an untracked object that is THIS amount of pixels around the latest ROI
    * @param retrackPixels Region around the latest tracker's ROI it will attempt to look for an untracked object (default is 100)
    **/
	void setRetrackRegion(int retrackPixels);
	
    /**
    * @brief Getter for retrack region - When an object is lost in the middle of the frame, the tracker will attempt to find an untracked object that is THIS amount of pixels around the latest ROI
    * @return Region around the latest tracker's ROI it will attempt to look for an untracked object
    **/
	int getRetrackRegion();
	
	/**
    * @brief Setter for retrackFrames
    * @param retrackFrames Once an object is lost, the amount of frames the tracker will look for the object before deleting the tracker from the tracker vector (default is 5)
    **/
	void setRetrackFrames(int retrackFrames);
	
    /**
    * @brief Getter for retrackFrames
    * @return Once an object is lost, the amount of frames the tracker will look for the object before deleting the tracker from the tracker vector (default is 5)
    **/
	int getRetrackFrames();	
	
	/**
    * @brief Setter for rectROIScale
    * @param rectROIScale Once the contours of an image is found, the ROI can be scaled by this amount before being fed into the Tracker
    **/
	void setROIScale(float rectROIScale);	
	
    /**
    * @brief Getter for rectROIScale
    * @return Once the contours of an image is found, the ROI can be scaled by this amount before being fed into the Tracker
    **/
	float getROIScale();
	
	/**
    * @brief Setter for threshhold minimum area
    * @param minArea Minimum area that a thresholded contour has to be in order to be thrown into the tracker algorithm
    **/
	void setMinThresholdArea(int minArea);
	
    /**
    * @brief Getter for threshhold minimum area
    * @return Minimum area that a thresholded contour has to be in order to be thrown into the tracker algorithm
    **/
	int getMinThresholdArea();
	
	/**
    * @brief Setter for minimum combined rect area
    * @param minArea Minimum area two ROIs must be before it's considered "overlapped"
    **/
	void setMinCombinedRectArea(int minArea);
	
    /**
    * @brief Getter for minimum combined rect area
    * @return Minimum area two ROIs must be before it's considered "overlapped"
    **/
	int getMinCombinedRectArea();
	
	/**
    * @brief Setter for frameCenter (overrides the initial value)
    * @param center The center of frame that is used to see if the fish have crossed, increments/decrements fishcounter accordingly
    **/
	void setFrameCenter(int center);
	
    /**
    * @brief Getter for frameCenter
    * @param center The center of frame that is used to see if the fish have crossed, increments/decrements fishcounter accordingly
    **/
	int getFrameCenter();
	
	/**
    * @brief Setter for filepath
    * @param filepath Path that the logger and other files that are saved will get saved to
    **/
	void setFilepath(string filepath);	
	
	/**
    * @brief Setter for filename
    * @param filename Name of prefix parts of the filename
    **/
	void setFilename(string filename);
	
	/**
    * @brief At that moment, saves all the information in the log stringstreams to a file
    * @param fileName If something is put in, will save to this file name
    * @param filePath If something is put in, will save to this file path
    **/
	void saveLogger(string fileName = NULL, string filePath = NULL);
	
	/**
	 * @brief Sets _programMode for either calibration, run, etc.
	 * @param mode Refer to enums for list of modes
	 **/
	void setMode(ftMode mode);
	
	/**
	 *	@brief Describes whether the tracker is object or not
	 *	@return True if tracker is active, false if not.
	 **/
	bool isTracking();
	
	/**
	 *	@brief Returns amount of fishTracker objects being tracked
	 *	@return Size of vector
	 **/
	int trackerAmount();
	
	/**
	 ** @brief Turns testing mode on or off to show important parameters on screen
	 ** @param isTesting true for testing, false to turn testing off
	 ***/
	void setTesting(bool isTesting);

	vector<FishTrackerStruct> getFishTracker() { return _fishTracker; }
	
private:
	////////// PARAMETERS ///////////
	
	//Parameters for if testing
	bool _testing;
	
	//Parameters for thresholding
	Scalar _rangeMin;
	Scalar _rangeMax;
	int _minThresholdArea; // Contour area must be greater than this to be added to tracker
	int _maxThresholdArea;
	int _minCombinedRectArea; // Minimum shared area between overlapping rects
	
	//Parameters for tracking
	vector<FishTrackerStruct> _fishTracker; //Holds all tracked object information
	float _marginProportion; //Essentially, percentage margins should be set to to consider object "on the edge"
	int _retrackPixels; //When occlusion occurs, what size of area around the ROI to look for an untracked object
	int _retrackFrames; //When an object has been lost, how many frames to keep looking for the object before deleting object from vector
	float _rectROIScale; //What percentage to delete from the roi when tracking
	int _minCombinedRectAreaProportion; //Minimum area two ROIs must be before it's considered "overlapped"

	//Parameters for counting
	Size _frameSize; // Takes the size and calculates mid
	int _frameMiddle; // Middle of screen for counting fish
	
	//Parameters for logging
	double _timer; //For putting elapsed times in
	string _startTime;
	string _loggerFilepath;
	string _loggerFilename;
	vector<string> _loggerData; // Keeps track of important events that happen
	vector<double> _loggerCsv; // Writes elapsed tracking computation times 
	
	//Other important data
	ftMode _programMode;
	
	/////////// FUNCTIONS /////////////
	
	//Get date and time in yyyy_mm_dd_hxxmxxsxx
	string _getTime();
	void _logger(vector<string>& logger, string data);
	vector<Rect> _getRects();
	
};