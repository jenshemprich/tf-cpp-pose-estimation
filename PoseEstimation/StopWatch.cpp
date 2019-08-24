#include "pch.h"

#include <limits>
#include <iomanip>
#include <sstream>

#include "StopWatch.h"

using namespace std;
using namespace chrono;

StopWatch::StopWatch() : start(now())
{}

std::chrono::duration<double, std::milli> StopWatch::get() const {
	return std::chrono::duration<double, std::milli>(now() - start);
}

StopWatch::operator std::string() const {
	ostringstream duration;
	duration << setprecision(3) << get().count();
	return duration.str();
}

void StopWatch::reset() {
	start = now();
}

steady_clock::time_point StopWatch::now() {
	return steady_clock::now();
}
