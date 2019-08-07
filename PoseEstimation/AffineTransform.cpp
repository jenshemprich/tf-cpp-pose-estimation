#include "pch.h"

#include <opencv2/opencv.hpp>

#include "AffineTransform.h"

using namespace cv;
using namespace std;

AffineTransform AffineTransform::identity;

AffineTransform::AffineTransform()
	: transform(Mat::eye(2, 2, CV_32F))
{}

AffineTransform::AffineTransform(const PointTriple& from, const PointTriple& to) : transform(getAffineTransform(from.points, to.points))
{}

AffineTransform::AffineTransform(const Rect2f & from, const Rect2f & to) : AffineTransform({
		from.tl(), Point2f(from.x + from.width, from.y), Point2f(from.x, from.y + from.height)
	}, {
		to.tl(), Point2f(to.x + to.width, to.y), Point2f(to.x, to.y + to.height)
	})
{}

const Point2f AffineTransform::operator()(const cv::Point2f& point) const {
	vector<Point2f> points = { point };
	return operator()(points).at(0);
}

const vector<Point2f>& AffineTransform::operator()(const vector<Point2f>& points) const {
	cv::transform(points, points, transform);
	return points;
}

const vector<Point2f> AffineTransform::operator()(const vector<Point2f>&& points) const {
	return operator()(points);
}
