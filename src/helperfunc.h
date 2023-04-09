#pragma once

#include <iostream>
#include <vector>
#include <string>

namespace FishMLHelper
{
   const int VIDEOS_PER_PAGE = 10;
   const std::string VIDEO_FILE_PATH = "./vid/";

   double millis();

	//Helps list files for video playback initializer
	int getVideoEntry(std::string& selectionStr);
	void showVideoList(std::vector<std::string> videoFileNames, int page);
}