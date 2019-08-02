#pragma once

#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/public/session.h"
#include "tensorflow/cc/ops/standard_ops.h"
#include "tensorflow/cc/client/client_session.h"
#include "tensorflow/cc/framework/ops.h"

#include "CocoDataModel.h"
#include "pafprocess.h"

class PoseEstimator {
public:
	PoseEstimator(char* const graph_file);
	~PoseEstimator();

	void loadModel();
	void addPostProcessing(tensorflow::Scope& scope, tensorflow::GraphDef& graph_def);
	void setGaussKernelSize(const size_t size);
	const std::vector<coco::Human> inference(const tensorflow::Tensor & input, const int upsample_size);
	const std::vector<coco::Human> estimate_paf(const tensorflow::Tensor& coords, const tensorflow::Tensor& peaks, const tensorflow::Tensor& heat_mat, const tensorflow::Tensor& paf_mat);
private:
	const char * const graph_file;
	tensorflow::Session* session;
	PafProcess paf;
};

