#pragma once

#include <chrono>

#include <opencv2/opencv.hpp>

class FramesPerSecond {
public:
	void update(cv::Mat& frame);
private:
	std::chrono::steady_clock::time_point now() const;
	std::chrono::steady_clock::time_point start;
};

