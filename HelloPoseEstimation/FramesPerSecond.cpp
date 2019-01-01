#include "pch.h"

#include <iostream>

#include "FramesPerSecond.h"

using namespace std;
using namespace chrono;

using namespace cv;

void FramesPerSecond::update(cv::Mat& frame) {
	auto end = steady_clock::now();
	auto frame_time = duration<double, centi>(end - start);
	ostringstream fps;
	fps << setprecision(2) << 100.0 / frame_time.count() << "fps";

	putText(frame, fps.str().c_str(), Point(0., 16), cv::FONT_HERSHEY_PLAIN, 1.0, cv::Scalar(0, 128, 0), 1, 8, false);
	start = end;
}

steady_clock::time_point FramesPerSecond::now() const {
	return steady_clock::now();
}
