#include "helperfunc.h"

#include <opencv2/opencv.hpp>
#include <filesystem>

using namespace cv;
using namespace std;

namespace fs = std::filesystem;

double FishMLHelper::millis() {
    return 1000*getTickCount()/getTickFrequency();
}

int FishMLHelper::getVideoEntry(string& selectionStr) {
	//Prepare video path
	string testVideoPath = VIDEO_FILE_PATH;
	if (testVideoPath.at(testVideoPath.size() - 1) == '/')
	{
		testVideoPath.pop_back();
	}

	//This is the return variable
	int videoEntered = -1;

	//Get vector of all files
	vector<string> videoFileNames;
	for (const auto & entry : fs::directory_iterator(VIDEO_FILE_PATH))
	{
		//Only add files that are .avi
		if (entry.path().extension() == ".txt")
		{
			continue;
		}

		//Make sure "_out" is not in the file name
		if (entry.path().filename().string().find("_out") != string::npos)
		{
			continue;
		}

		videoFileNames.push_back(entry.path());
	}

	//Need to return and exit to main if there are no files
	if (videoFileNames.size() == 0)
	{
		cout << "No video files in folder found in " + testVideoPath + "/" << endl;
		return -1;
	}

	//Variables for iterating through part of the vidoe file folder.
	int page = 0;
	int pageTotal = (videoFileNames.size() / VIDEOS_PER_PAGE) + 1;
	cout << "Page total is " << pageTotal << endl;

	cout << videoFileNames.size() << " files found: \n";

	//Shows
	FishMLHelper::showVideoList(videoFileNames, page);

	while (videoEntered == -1)
	{
		cout << "Select video. Enter \"q\" to go back. Enter \"n\" or \"p\" for more videos.\n";
		cout << "To select video, enter number associated with file number.\n";
		cout << "I.e., to select \"File 4:\t \'This video.avi\'\", you would simply enter 4.\n";
		cout << "File: ";

		string userInput;
        cin >> userInput;

		//Leave the function and go back to main
		if (userInput == "q" || userInput == "Q")
		{
			return -1;
		}

		//Next page, show video files, re-do while loop
		if (userInput == "n" || userInput == "N")
		{
			if (++page >= pageTotal)
			{
				page = 0;
			}
			FishMLHelper::showVideoList(videoFileNames, page);
			continue;
		}

		//Previous page, show video files, re-do while loop
		if (userInput == "p" || userInput == "P")
		{
			if (--page < 0)
			{
				page = pageTotal - 1;
			}
			FishMLHelper::showVideoList(videoFileNames, page);
			continue;
		}

		//Need to make sure the string can be converted to an integer
		bool inputIsInteger = true;
		for (int charIndex = 0; charIndex < (int) userInput.size(); charIndex++)
		{
			inputIsInteger = isdigit(userInput[charIndex]);

			if (!inputIsInteger)
			{
				break;
			}
		}

		if (!inputIsInteger)
		{
			cout << "Input must be an integer between 0 and " + to_string(videoFileNames.size() - 1) + " (inclusive)" << endl;
			continue;
		}

		int userInputInt = stoi(userInput);

		if (userInputInt < 0 || userInputInt >= (int) videoFileNames.size())
		{
			cout << "Input must be an integer between 0 and " + to_string(videoFileNames.size() - 1) + " (inclusive)" << endl;
			continue;
		}

		videoEntered = userInputInt;
	}

	selectionStr = videoFileNames[videoEntered];
	return 1;
}

void FishMLHelper::showVideoList(vector<string> videoFileNames, int page) {
    //Get indexing numbers
	int vecBegin = page * FishMLHelper::VIDEOS_PER_PAGE;
	int vecEnd = (page + 1) * FishMLHelper::VIDEOS_PER_PAGE;

	//Make sure we don't list files exceeding array size later
	if (vecEnd > (int) videoFileNames.size())
	{
		vecEnd = videoFileNames.size();
	}

	cout << "Showing files " << vecBegin << "-" << vecEnd << " of a total " << videoFileNames.size() << " files.\n";

	//Iterate and list file names
	for (int fileIndex = vecBegin; fileIndex < vecEnd; fileIndex++)
	{
		//This gives us the full path, which is useful later, but we just need the particular file name
		//Therefore, following code delimits with the slashbars
		stringstream buffSS(videoFileNames[fileIndex]);
		string currentFile;

		while (getline(buffSS, currentFile, '/'))
		{
		}

		cout << "\t>> File " << fileIndex << ":\t \"" << currentFile << "\"\n";
	}
}