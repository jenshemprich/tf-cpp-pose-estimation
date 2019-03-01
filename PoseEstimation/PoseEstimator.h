#pragma once

#include <opencv2/opencv.hpp>

#include "TensorMat.h"
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
	PoseEstimator(char* const graph_file);
	~PoseEstimator();

	void loadModel();
	void addPostProcessing(tensorflow::Scope& scope, tensorflow::GraphDef& graph_def);
	void setGaussKernelSize(const size_t size);
	const std::vector<Human> inference(const TensorMat& input, const int upsample_size);
	void draw_humans(cv::Mat& image, const cv::Rect& view, const std::vector<Human>& humans) const;
	std::vector<Human> estimate_paf(const tensorflow::Tensor& coords, const tensorflow::Tensor& peaks, const tensorflow::Tensor& heat_mat, const tensorflow::Tensor& paf_mat, const cv::Mat& transform);
private:
	const char * const graph_file;
	tensorflow::Session* session;
	PafProcess paf;
};

