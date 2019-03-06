#pragma once


class AffineTransform {
public:
	struct PointTriple {
		const cv::Point2f points[3];
	};

	AffineTransform(const PointTriple& from, const PointTriple& to);
	AffineTransform(const cv::Rect2f & from, const cv::Rect2f & to);
	const cv::Point2f operator()(const cv::Point2f& point) const;
	const std::vector<cv::Point2f>& operator()(const std::vector<cv::Point2f>& points) const;
	const std::vector<cv::Point2f> operator()(const std::vector<cv::Point2f>&& points) const;
private:
	const cv::Mat transform;
};

cv::Point2f operator/(const cv::Point2f& a, const cv::Point2f& b);
cv::Size operator*(const cv::Size& a, const cv::Size& b);
cv::Size operator/(const cv::Size& a, const cv::Size& b);
