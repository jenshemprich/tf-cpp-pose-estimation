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
	void addPostProcessing(tensorflow::GraphDef& graph_def);
	const std::vector<Human> inference(const TensorMat& input, const int upsample_size);
	const std::vector<Human> inference(const tensorflow::Tensor& input, const int upsample_size);
	void draw_humans(cv::Mat& image, const std::vector<Human>& humans) const;
	void imshow(const char * caption, tensorflow::Tensor & tensor, int channel);
	void imshow(const char* caption, cv::Mat& mat);
	void imshow(const char* caption, const size_t height, const size_t width, float * d);
	std::vector<Human> estimate_paf(const tensorflow::Tensor& coords, const tensorflow::Tensor& peaks, const tensorflow::Tensor& heat_mat, const tensorflow::Tensor& paf_mat);
private:
	const char * const graph_file;
	tensorflow::Session* session;
	PafProcess paf;
};

