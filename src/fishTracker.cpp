#include "fishTracker.h"

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>

using namespace FishMLHelper;

//Nothing is needed in constructor, using init() instead
FishTracker::FishTracker()
{
}

FishTracker::~FishTracker()
{
	cout << "\tNOTE: FishTracker destroyed\n";
}


int FishTracker::run(Mat& im, mutex& lock, int& fishIncrement, int& fishDecrement, vector<FishMLData>& detectedObjects, vector<FishTrackerStruct>& trackedObjects)
{		
	//Make new Mat so it can be used without needing mutex
	Mat frameRaw;
	
	//First, get edges for comparison at multiple times through loop
	vector<Rect> edges;
	edges.push_back(Rect(0, 0, _frameSize.width * _marginProportion, _frameSize.height)); // left edge
	edges.push_back(Rect(_frameSize.width * (1 - _marginProportion), 0, _frameSize.width, _frameSize.height)); // right edge
	edges.push_back(Rect(0, _frameSize.height - 2, _frameSize.width, 2)); // bottom edge
	edges.push_back(Rect(0, 0, _frameSize.width, 2)); // top edge
	
	{
		//Make automatic mutex control
		scoped_lock lockGuard (lock);

		//Make sure there's actually a frame to do something with
		if (im.empty())
		{
			cout << "ERROR: No frame to process in FishTracker::run()\n";
			return -1;
		}
		
		frameRaw = im.clone();
	}
		
	//Update tracker, and ensure it's running
	//THIS FOLLOWING CODE MAY BE HARD TO READ AND MIGHT NEED TO BE CLEANED UP SOMEHOW
	for (int i = _fishTracker.size() - 1; i >= 0; i--) 
	{
		//Update ROI from tracking			********** CHANGE FOLLOWING LINE FOR TRACKER IF NEED BE **************
		_fishTracker[i].isTracked = _fishTracker[i].tracker->update(frameRaw, _fishTracker[i].roi);
		
		//Make sure we aren't detecting the same object twice
		for (int j = _fishTracker.size() - 1; j > i; j--)
		{
			bool isOverlapped = false;

			Rect rect1 = _fishTracker[i].roi;
			Rect rect2 = _fishTracker[j].roi;
			
			int area1 = rect1.area();
			int area2 = rect2.area();

			// Check if one is inside the other
			if(_fishTracker[i].isCounted || _fishTracker[j].isCounted)
			{
				isOverlapped |= (rect1 & rect2).area() >= area1 * 0.9;
				isOverlapped |= (rect1 & rect2).area() >= area2 * 0.9;
			}
			else
			{
				isOverlapped |= (rect1 & rect2).area() >= area1 * 0.6;
				isOverlapped |= (rect1 & rect2).area() >= area2 * 0.6;
			}
			//If roi overlap, same object
			if (isOverlapped)
			{
				_fishTracker.erase(_fishTracker.begin() + j);
			}
		}
		
		//Move onto next iteration if cannot be detected anymore
		if (!_fishTracker[i].isTracked)
		{
			continue;	
		}									
	}

	//Go through these contours and find the parameters of each
	for (auto &obj : detectedObjects)
	{
		int newWidth, newHeight;
		Point centerPoint;

		{
			scoped_lock lockGuard(lock);
			//Make roi smaller that will actually be tracked
			//First need center
			centerPoint = Point(obj.ROI.x + obj.ROI.width / 2, obj.ROI.y + obj.ROI.height / 2);

			//Make scaled width
			newWidth = (obj.ROI.width / 2) * _rectROIScale;
			newWidth = (newWidth == 0) ? 1 : newWidth;
					
			//Make scaled width
			newHeight = (obj.ROI.height / 2) * _rectROIScale;
			newHeight = (newHeight == 0) ? 1 : newHeight;
		}		
		
		//Make rect that will be used for tracking
		Rect contourRectROI = Rect(centerPoint.x - newWidth, centerPoint.y - newHeight, newWidth * 2, newHeight * 2);
					
		//See if this contour rectangle is an object that's already been detected
		bool fishOverlappedROIs = false; // Turns true if the contour rect overlaps with one of the fishTracker rois
		for (auto& fish : _fishTracker)
		{	
			scoped_lock lockGuard(lock);
			//Checks for overlap and stores result
			//fishOverlappedROIs |= ((obj.ROI & fish.roi).area() > _minCombinedRectArea);

			//Check to see if the roi is within the other, as well
			//fishOverlappedROIs |= (obj.ROI & fish.roi).area() >= min(obj.ROI.area(), fish.roi.area()) * _minCombinedRectAreaProportion;

			//Now check whether the contour rect is within the roi
			if(fish.isCounted)
			{
				fishOverlappedROIs |= (obj.ROI & fish.roi).area() >= obj.ROI.area() * 0.6;
				fishOverlappedROIs |= (obj.ROI & fish.roi).area() >= fish.roi.area() * 0.6;
			}
			else
			{
				fishOverlappedROIs |= (obj.ROI & fish.roi).area() >= obj.ROI.area() * 0.3;
				fishOverlappedROIs |= (obj.ROI & fish.roi).area() >= fish.roi.area() * 0.3;
			}

			//Can exit loop if there's been an overlap
			if (fishOverlappedROIs)
			{
				break;
			}					
		}
			
		//Now see if a tracker has been lost, and if a widened area (by a reasonable amount) overlaps with the contour
		//If so, then have that specific tracker "re-track" with the contour
		bool fishRetracked = false;
		for (auto& fish : _fishTracker)
		{
			
			//Skip iteration if tracker hasn't been lost
			if (fish.isTracked)
			{ 
				continue;
			}
				
			//Make wider ROI to account for moving ball
			Rect newFishROI = Rect(fish.roi.x - _retrackPixels, fish.roi.y - _retrackPixels, fish.roi.width + _retrackPixels * 2, fish.roi.height + _retrackPixels * 2);
			
			scoped_lock lockGuard (lock);

			if ((obj.ROI & newFishROI).area() > _minCombinedRectArea)
			{
				fishRetracked = true;	
				fish.isTracked = true;
					
				//Re-initialize the tracker with the new coordinates
				TrackerKCF::Params params;
				fish.tracker = TrackerKCF::create(params);
				fish.tracker->init(frameRaw, contourRectROI);
				
				break;
			}
		}
				

		//One last thing that needs to be checked is if this is a reflection
		//The method I can think of best doing this is by creating an artificial rect for both the input and output, and comparing the two
		bool isReflection = false;
		
		///////////////////UNCOMMENT FOLLOWING IF WE NEED REFLECTION CHECKING
		// Rect currentContourRect;
		// int currentContourY;
		
		// {	
		// 	scoped_lock lockGuard(lock);
		// 	currentContourRect = Rect(obj.ROI.x, 0, obj.ROI.x + obj.ROI.width, 2);
		// 	currentContourY = obj.ROI.y;
		// }
		
		// for (auto& fish : _fishTracker)
		// {
		// 	Rect newFishROIRect = Rect(fish.roi.x - EXTRA_REFLECT_WIDTH, 0, fish.roi.x + fish.roi.width + EXTRA_REFLECT_WIDTH, 3);
			
		// 	//Two things we need to check:
		// 	// 1.) Is this contour ABOVE the ROI of a tracked object?
		// 	// 2.) Is this contour's x horizontally between the start and end of the tracked object?
		// 	if (currentContourY < fish.roi.y && (newFishROIRect & currentContourRect).area() > 0)
		// 	{
		// 		isReflection = true;
		// 		break;				
		// 	}			
		// }			


		//Now need to iterate through the other contours
		// if (!isReflection)
		// {				
		// 	for (int j = 0; j < (int) contours.size(); j++) 
		// 	{
		// 		//Disregard this iteration of loop if it's the same contour
		// 		if (i == j)
		// 		{
		// 			continue;						
		// 		}
				
		// 		Rect comparedContour = boundingRect(contours[j]);
		// 		Rect comparedContourNewRect = Rect(comparedContour.x - EXTRA_REFLECT_WIDTH, 0, comparedContour.x + comparedContour.width + EXTRA_REFLECT_WIDTH, 2);
			
		// 		if (contourRect.y < comparedContour.y && (comparedContourNewRect & currentContourRect).area() > 0)
		// 		{
		// 			isReflection = true;
		// 			break;				
		// 		}		
		// 	}
		// }
		///////////////////END OF REFLECTION CHECKING
	
		bool isOnEdge = false;
		for (auto edge : edges)
		{
			isOnEdge |= (obj.ROI & edge).area() > obj.ROI.area() * 0.3;
		}

		if(isOnEdge)
		{
			cout << "Fish on edge" << endl;
			continue;
		}

		//Make rect mostly in center of screen, from top of screen to bottom
		Rect centerRect = Rect(_frameSize.width * 0.4, 0, _frameSize.width * 0.2, _frameSize.height);
		bool isCenter = (obj.ROI & centerRect).area() > 0;

		//If no overlap, then make new struct and track
		if (!fishOverlappedROIs && !fishRetracked && !isReflection && isCenter)
		{									
			//Initialize struct that keeps track of the tracking info
			FishTrackerStruct tempTracker;					
			tempTracker.isTracked = true;
			tempTracker.tracker = TrackerKCF::create();					
			tempTracker.tracker->init(frameRaw, contourRectROI);
			tempTracker.roi = contourRectROI;
			tempTracker.posX.push_back(contourRectROI.x + contourRectROI.width / 2);
			tempTracker.lostFrameCount = 0;
			tempTracker.startTime = millis();
			tempTracker.currentTime = millis() - tempTracker.startTime;
			tempTracker.isCounted = false;
				
			//Now add that to the overall fish tracker
			_fishTracker.push_back(tempTracker);	
		}

		/********** DELME LATER *************/
		// if (_testing)
		// {
		// 	if (isReflection)
		// 	{
		// 		Scalar thisColour = Scalar(0, 0, 255);
		// 		rectangle(frameRaw, contourRect, thisColour, 3);
		// 		putText(frameRaw, "Reflection", Point(contourRect.x, contourRect.y - 6), FONT_HERSHEY_PLAIN, 0.7, thisColour, 2);
		// 	}
		// 	else 
		// 	{
		// 		Scalar thisColour = Scalar(0, 255, 0);
		// 		rectangle(frameRaw, contourRect, thisColour, 3);
		// 		putText(frameRaw, "No reflection", Point(contourRect.x, contourRect.y - 6), FONT_HERSHEY_PLAIN, 0.7, thisColour, 2);	
		// 	}
		// }
			
	}
			
		
	//Parse through all fish trackers, from end to beginning, and do housekeeping
	//Meaning: 1. Time gets updated, and delete if timeout (ONLY if ROI isn't in center though)
	//2. delete tracked object if tracking has been lost
	//3. need to check if the contour is a reflection. Need to delete if it is a reflection
	
	//Now parse through fishTrackers
	//Must go backwards else this algorithm will actually skip elements	
	for (int i = _fishTracker.size() - 1; i >= 0; i--) 
	{	
		
		_fishTracker[i].currentTime = millis() - _fishTracker[i].startTime;
		bool trackedObjectOutdated = _fishTracker[i].currentTime > TRACKER_TIMEOUT_MILLIS;

		//Delete if timeout and not in center
		if (trackedObjectOutdated && !(_frameMiddle > _fishTracker[i].roi.x && _frameMiddle < _fishTracker[i].roi.x + _fishTracker[i].roi.width))
		{
			_fishTracker.erase(_fishTracker.begin() + i);
			continue; // Move onto next iteration
		}

		bool trackObjectOudatedCenter = _fishTracker[i].currentTime > TRACKER_TIMEOUT_MILLIS_CENTER;
		
		if (!_fishTracker[i].isTracked)
		{	
			//Count amount of frames the fish has been lost for
			_fishTracker[i].lostFrameCount++;
				
			//See if the item is just on the edge
			bool isOnEdge = false;
			
			for (auto edge : edges) 
			{
				isOnEdge |= (edge & _fishTracker[i].roi).area() > 0;
			}		
									
			if (_fishTracker[i].lostFrameCount >= _retrackFrames || (isOnEdge && _fishTracker[i].currentTime > 2000))
			{
				//After trying to retrack a while, element cannot be found so it should be removed from overall vector list
				_fishTracker.erase(_fishTracker.begin() + i);
				continue;
			}
		}
		else
		{
			//Reset if fish has been found again
			_fishTracker[i].lostFrameCount = 0;
		}
		
		//Check to see if there's another object below it, and if so delete it from tracked vector list

		////////////////////////////// UNCOMMENT THIS TO ENABLE REFLECTION CHECKING
		// Rect modifiedCurrentRect = Rect(_fishTracker[i].roi.x, 0, _fishTracker[i].roi.x + _fishTracker[i].roi.width, 3);
			
		// for (int j = 0; j < (int) _fishTracker.size(); j++) 
		// {
				
		// 	//Don't bother if it is the same number
		// 	if (i == j)
		// 	{
		// 		continue;
		// 	}
				
		// 	Rect modifiedCompareRect = Rect(_fishTracker[j].roi.x, 0, _fishTracker[j].roi.x + _fishTracker[j].roi.width, 3);
				
		// 	//IF this is true, that means tracked object is directly above another tracked object
		// 	//This could be improved, but for now we will consider that a reflection
		// 	//(How could it be improved? Two objects could be real above and beneath each other)
		// 	if (_fishTracker[i].roi.y < _fishTracker[j].roi.y && (modifiedCompareRect & modifiedCurrentRect).area() > 0)
		// 	{
		// 		_fishTracker.erase(_fishTracker.begin() + i);
		// 		_logger(_loggerData, "REFLECTION DETECTED, DELETING OBJECT: " + to_string(i + 1));		
		// 		break;					
		// 	}
		// }
		////////////////////////////// UNCOMMENT THIS TO ENABLE REFLECTION CHECKING
	}
	
	//Track through all fish trackers
	for(int i = _fishTracker.size()-1; i>=0; i--)
	{
		//Following script sees if the fish has passed through the midpoint, and from which side
		//If it passes from left to right, we increase the ball counter. If the other way, we decrease
		//Additionally, some code has been put in to make sure we aren't counting vector additions from multiple runs
		_fishTracker[i].posX.push_back(_fishTracker[i].roi.x + _fishTracker[i].roi.width / 2);				
					
		if (_fishTracker[i].posX.size() >= 2)
		{
			int posCurr = _fishTracker[i].posX[1];
			int posLast = _fishTracker[i].posX[0];

			//If the fish is in the middle, we don't want to count it
			if(_fishTracker[i].isCounted)
			{
				continue;
			}

			if (posCurr > _frameMiddle && posLast <= _frameMiddle)
			{
				fishIncrement++;	
				_fishTracker[i].isCounted=true;
				continue;				
			}
			else if (posCurr < _frameMiddle && posLast >= _frameMiddle)
			{
				fishDecrement++;	
				_fishTracker[i].isCounted=true;
				continue;
			}
			
			//Delete unnecessary position values
			_fishTracker[i].posX.erase(_fishTracker[i].posX.begin());
		}			
	}

	//Copy rects from fishTracker vector to parent class
	trackedObjects.clear();
	trackedObjects = _fishTracker;
	
	return 0;
}


int FishTracker::init(Size frameSize)
{
	//Clear fish tracking vector
	_fishTracker.clear();
	
	//Emptying data vectors	
	_loggerData.clear();
	_loggerCsv.clear();
	
	//Test mode for seeing important parameters
	_testing = false;

	//Area of the ROIs that overlap before it is considered a "tracked object" already
	_minCombinedRectAreaProportion = DEFAULT_COMBINED_RECT_AREA_PROPORTION;
	
	//Store temporary frame to get Size and middle
	_frameSize = frameSize;
	_frameMiddle = _frameSize.width / 2;

	//Thresholding minimum area of contour to throw into tracker
	_minThresholdArea = DEFAULT_MIN_AREA;
	_maxThresholdArea = DEFAULT_MAX_AREA;
	
	//Minimum area that combined ROIs can be before they are considered a "tracked object" already
	_minCombinedRectArea = DEFAULT_COMBINED_RECT_AREA;
	
	//A percentage specified of the frame to be considered the edge
	//If the frame were 1000 pixels, and 0.08 is specified, then 80 pixels from the left
	//And 80 pixels from the right will be considered the edge
	_marginProportion = DEFAULT_MARGIN;
	
	//When an object is lost, there is occlusion detection. This specifies the area around
	//the latest ROI entry as to where to look for an untracked object
	_retrackPixels = DEFAULT_RETRACK_PIXELS;
	
	//When object is not on the edge, amount of frames to try to "re-track" a lost object
	_retrackFrames = DEFAULT_RETRACK_FRAMES;
	
	//Proportion of ROI to scale down by to increase computational speed of tracker
	_rectROIScale = DEFAULT_RECT_SCALE;
		
	//Defaults for logger path and filename
	_startTime = _getTime();
	_loggerFilename = "data_" + _startTime;
	_loggerFilepath = DEFAULT_FILE_PATH;
	
	//Test mode to display possible helpful debugging parameters
	_programMode = ftMode::TRACKING;
	
	return 0;
}

//Set proportion of frame that will be considered the "edge"
void FishTracker::setMargin(float marginProportion)
{
	_marginProportion = marginProportion;
}

//Returns value of frame margin proportion
float FishTracker::getMargin()
{
	return _marginProportion;
}

//Set region that occlusion detector will look around lost object
void FishTracker::setRetrackRegion(int retrackPixels)
{
	_retrackPixels = retrackPixels;
}

//Returns value for occlusion detector region
int FishTracker::getRetrackRegion()
{
	return _retrackPixels;
}

//Sets amount of frames occlusion detector will run before deleting tracker vector
void FishTracker::setRetrackFrames(int retrackFrames)
{
	_retrackFrames = retrackFrames;
}

//Returns amount of frames occlusion detector will run before deleting tracker vector
int FishTracker::getRetrackFrames()
{
	return _retrackFrames;
}

//Sets a scalar value that multiplies tracker ROI to save computation time
void FishTracker::setROIScale(float rectROIScale)
{
	_rectROIScale = rectROIScale;
}

//Returns scalar value for multiplying tracker ROI 
float FishTracker::getROIScale()
{
	return _rectROIScale;
}

//Sets minimum threshold that a contour must be over to be added into a tracker
void FishTracker::setMinThresholdArea(int minArea)
{
	_minThresholdArea = minArea;
}

//Returns minimum threshold that a contour must be over to be added into a tracker
int FishTracker::getMinThresholdArea()
{
	return _minThresholdArea;
}

//Sets area two ROIs overlapping is before it's considered the ROI of a tracked object
void FishTracker::setMinCombinedRectArea(int minArea)
{
	_minCombinedRectArea = minArea;
}

//Gets area two ROIs overlapping is before it's considered the ROI of a tracked object
int FishTracker::getMinCombinedRectArea()
{
	return _minCombinedRectArea;
}

//Sets the x_position the vertical line will be which fish crossing over will be counted
void FishTracker::setFrameCenter(int center)
{
	_frameMiddle = center;
}

//Gets the x_position the vertical line will be which fish crossing over will be counted
int FishTracker::getFrameCenter()
{
	return _frameMiddle;
}


//Sets the program mode, i.e. calibration, normal, testing, etc.
void FishTracker::setMode(ftMode mode)
{
	_programMode = mode;
}

//Describes whether the tracker is object or not
bool FishTracker::isTracking()
{
	return _fishTracker.size() > 0;
}

//Returns amount of fishTracker objects being tracked
int FishTracker::trackerAmount()
{
	return _fishTracker.size();
}

//Setter for filepath
void FishTracker::setFilepath(string filepath)
{
	_loggerFilepath = filepath;
}

//Setter for filename
void FishTracker::setFilename(string filename)
{
	_loggerFilename = filename;
}

//Helper that gets the current time in ymd hms and returns a string
std::string FishTracker::_getTime()
{
	//Create unique timestamp for folder
	stringstream timestamp;
	
	//First, create struct which contains time values
	time_t now = time(0);
	tm *ltm = localtime(&now);
	
	//Store stringstream with numbers	
	timestamp << 1900 + ltm->tm_year << "_";
	
	//Zero-pad month
	if ((1 + ltm->tm_mon) < 10) 
	{
		timestamp << "0";
	}
	
	timestamp << 1 + ltm->tm_mon << "_";
	
	//Zero-pad day
	if ((1 + ltm->tm_mday) < 10) 
	{
		timestamp << "0";
	}
	
	timestamp << ltm->tm_mday << "_";
	
	//Zero-pad hours
	if (ltm->tm_hour < 10) 
	{
		timestamp << "0";
	}
	
	timestamp << ltm->tm_hour << "h";
	
	//Zero-pad minutes
	if (ltm->tm_min < 10) 
	{
		timestamp << "0";
	}
	
	timestamp << ltm->tm_min << "m";
	
	//Zero-pad seconds
	if (ltm->tm_sec < 10) 
	{
		timestamp << "0";
	}
	
	timestamp << ltm->tm_sec << "s";
	
	//Return string version of ss
	return timestamp.str();
}

//Helper function for 
void FishTracker::_logger(vector<string>& logger, string data)
{
	//Clear first entry in the vector if buffer size is full
	if (logger.size() > MAX_DATA_SIZE)
	{
		logger.erase(logger.begin());
	}
	
	string logString = _getTime() + ": " + data;
	//cout << logString << "\n";
	logger.push_back(logString);
}

//Saves both data and elapsed tracking frame times to files
void FishTracker::saveLogger(string fileName /* = NULL */, string filePath /* = NULL */)
{
	//Use file-name stored in object if filename is blank
	if (fileName.empty())
	{
		fileName = _loggerFilename;		
	}
	
	//Use folder-path stored in object if filePath is blank
	if (filePath.empty())
	{
		filePath = _loggerFilepath;
	}
	
	//Add slash if folder-path does not end in slash so filename doesn't merge with the folder it's in
	if (filePath[filePath.size() - 1] != '/')
	{
		filePath += '/';
	}
	
	//Make sure file doesn't end in ".txt" if we end up being knobs and accidentally adding it
	stringstream tempSString(fileName);
	string tempString;
	fileName = "";
	
	while (getline(tempSString, tempString, '.'))
	{
		if (tempString != "csv" && tempString != "txt")
		{
			fileName += tempString;
		}
	}
	
	//Create txt file for data and then store the data
	ofstream outFileData(filePath + fileName + ".txt");
	for (auto line : _loggerData)
	{
		outFileData << line;
	}
	outFileData.close();
	
	//Create CSV file and store data
	ofstream outFileCSV(filePath + fileName + ".csv");
	for (auto val : _loggerCsv) 
	{
		outFileCSV << to_string(val) << "\n";
	}
	outFileCSV.close();
}

//Helper function to pull ROIs from fishTracker vector and return them as its own vector
vector<Rect> FishTracker::_getRects()
{
	//Since the rects are embedded in structs, need to extract them into separate vector
	vector<Rect> tempRects;
	
	for (auto fish : _fishTracker)
	{
		tempRects.push_back(fish.roi);
	}
	
	return tempRects;
}


void FishTracker::setTesting(bool isTesting)
{
	_testing = isTesting;
}
