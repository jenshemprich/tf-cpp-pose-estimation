#pragma once

#include <opencv2/opencv.hpp>

#include "pafprocess.h"

class Human {
};

class PoseEstimator {
public:
	PoseEstimator(char* const graph_file, int target_width, int target_height);
	~PoseEstimator();

	tensorflow::error::Code loadModel();
	tensorflow::Status addPostProcessing(tensorflow::GraphDef& graph_def);
	tensorflow::error::Code inference(const cv::Mat& frame, const int upsample_size, std::vector<Human>& humans);
	void draw_humans(cv::Mat& image, const std::vector<Human> humans) const;
	void imshow(const char * caption, tensorflow::Tensor & tensor, int channel);
	void imshow(const char* caption, cv::Mat& mat);
	void imshow(const char* caption, const size_t height, const size_t width, float * d);
	void estimate_paf(const tensorflow::Tensor& coords, const tensorflow::Tensor& peaks, const tensorflow::Tensor& heat_mat, const tensorflow::Tensor& paf_mat, std::vector<Human>& humans);
private:
	const char * const graph_file;
	tensorflow::GraphDef graph_def;
	tensorflow::Session* session;

	tensorflow::Tensor image_tensor;
	cv::Mat* resized;
	cv::Mat* image_mat;

	PafProcess paf;
};

