#pragma once

#include <opencv2/opencv.hpp>

#include "pafprocess.h"

struct BodyPart {
	BodyPart(int part_index, float x, float y, float score);

	int part_index;
	float x;
	float y;
	float score;
};

struct Human {
	typedef std::map<int, BodyPart> BodyParts;
	Human() : score(0.0) {};
	Human(const BodyParts & parts, const float score);

	const BodyParts parts;
	const float score;
};

class PoseEstimator {
public:
	PoseEstimator(char* const graph_file, const cv::Size& image);
	~PoseEstimator();

	void loadModel();
	void addPostProcessing(tensorflow::Scope& scope, tensorflow::GraphDef& graph_def);
	void setGaussKernelSize(const size_t size);
	const std::vector<Human> inference(const cv::Mat& frame, const int upsample_size);
	void draw_humans(cv::Mat& image, const std::vector<Human>& humans) const;
	void imshow(const char * caption, tensorflow::Tensor & tensor, int channel);
	void imshow(const char* caption, cv::Mat& mat);
	void imshow(const char* caption, const size_t height, const size_t width, float * d);
	std::vector<Human> estimate_paf(const tensorflow::Tensor& coords, const tensorflow::Tensor& peaks, const tensorflow::Tensor& heat_mat, const tensorflow::Tensor& paf_mat);
private:
	const char * const graph_file;
	const cv::Size inference_mat_default_size;

	tensorflow::GraphDef graph_def;
	tensorflow::Session* session;

	tensorflow::Tensor image_tensor;
	cv::Mat* resized;
	cv::Mat* image_mat;
	PafProcess paf;
};

