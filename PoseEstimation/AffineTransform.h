#pragma once


class AffineTransform {
public:
	struct PointTriple {
		const cv::Point2f points[3];
	};

	static AffineTransform identity;

	AffineTransform();

	AffineTransform(const PointTriple& from, const PointTriple& to);
	AffineTransform(const cv::Rect2f & from, const cv::Rect2f & to);
	const cv::Point2f operator()(const cv::Point2f& point) const;
	const std::vector<cv::Point2f>& operator()(const std::vector<cv::Point2f>& points) const;
	const std::vector<cv::Point2f> operator()(const std::vector<cv::Point2f>&& points) const;

	cv::Mat transform;
};
