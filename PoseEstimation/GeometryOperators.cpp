#include "pch.h"

#include <opencv2/opencv.hpp>

#include "GeometryOperators.h"

using namespace cv;

Point2f operator/(const Point2f& a, const Point2f& b) {
	return Point2f(a.x / b.x, a.y / b.y);
}

Size operator*(const Size& a, const Size& b) {
	return Size(a.width * b.width, a.height * b.height);
}

Size operator/(const Size& a, const Size& b) {
	return Size(a.width / b.width, a.height / b.height);
}
