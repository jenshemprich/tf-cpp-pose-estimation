#pragma once

#include <chrono>
#include <string>

class StopWatch {
public:
	StopWatch();

	std::chrono::duration<double, std::milli> get() const;
	operator std::string() const;

	void reset();
private:
	static std::chrono::steady_clock::time_point now();
	std::chrono::steady_clock::time_point start;
};

