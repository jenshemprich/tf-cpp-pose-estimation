#include "pch.h"

#include <opencv2/opencv.hpp>

#include "tensorflow/core/platform/env.h"
#include "Eigen/Dense"
#include "tensorflow/core/public/session.h"
#include "tensorflow/cc/ops/standard_ops.h"

#include "tensorflow/cc/client/client_session.h"
#include "tensorflow/cc/framework/ops.h"

#include "GaussKernel.h"
#include "PoseEstimator.h"

#include "pafprocess.h"

using namespace std;
using namespace cv;
using namespace tensorflow;

PoseEstimator::PoseEstimator(char * const graph_file)
	: graph_file(graph_file), session(nullptr) {}

PoseEstimator::~PoseEstimator() {
}

void PoseEstimator::loadModel() {
	tensorflow::GraphDef graph_def;
	Status load_graph_status = ReadBinaryProto(tensorflow::Env::Default(), graph_file, &graph_def);
	if (!load_graph_status.ok()) {
		throw tensorflow::errors::NotFound("Failed to load compute graph from '", graph_file, "'");
	}

	Scope scope = Scope::NewRootScope();
	GraphDef postProcessing;
	addPostProcessing(scope, postProcessing);
	graph_def.MergeFrom(postProcessing);

	Status status = NewSession(tensorflow::SessionOptions(), &session);
	if (!status.ok()) {
		throw status;
	}


	// Build static app example code (non-working with dll):
	// https://github.com/tensorflow/tensorflow/blob/master/tensorflow/examples/label_image/main.cc
	// session->Reset(tensorflow::NewSession(tensorflow::SessionOptions()));
	// auto s = new std::unique_ptr<tensorflow::Session>(session);
	// s->reset(tensorflow::NewSession(tensorflow::SessionOptions()));

	Status session_create_status = session->Create(graph_def);
	if (!session_create_status.ok()) {
		throw session_create_status;
	}
}

// Whether to run inference and post-processing in the same or in separate sessions
// #define SPLIT_RUNS

void PoseEstimator::addPostProcessing(Scope& scope, GraphDef& graph_def) {
	 auto upsample_size_placeholder = ops::Placeholder(scope.WithOpName("upsample_size"), DT_INT32, ops::Placeholder::Shape({  }));
#ifdef SPLIT_RUNS
	auto inference_tensor_placeholder = ops::Placeholder(scope.WithOpName("inference_tensor"), DT_FLOAT, ops::Placeholder::Shape({ -1, -1, -1, 57 }));
#else
	auto noop = ops::Const(scope.WithOpName("noop"), { { 0.f } });
	auto node_from_other_graph = Input("Openpose/concat_stage7", 0, DT_FLOAT);
	// TODO Resolve crash when slicing directly and remove the ops::Add(...) workaround (see unit tests)
	auto inference_tensor_placeholder = ops::Add(scope.WithOpName("workaround"), noop, node_from_other_graph);
#endif

	/// size = 40x22
	Output heat_mat = ops::Slice(scope.WithOpName("heat_mat"), inference_tensor_placeholder, Input::Initializer({ 0, 0, 0, 0 }), Input::Initializer({ -1, -1, -1, 19 }));
	Output paf_mat = ops::Slice(scope.WithOpName("paf_mat"), inference_tensor_placeholder, Input::Initializer({ 0, 0, 0, 19 }), Input::Initializer({ -1, -1, -1, 38 }));

	/// use upscale == 4 or higher to achieve reasonable detection and accuracy of body parts
	Output heat_mat_up = ops::ResizeArea(scope.WithOpName("heat_mat_up"), heat_mat, upsample_size_placeholder, ops::ResizeArea::AlignCorners(false));
	Output paf_mat_up = ops::ResizeArea(scope.WithOpName("paf_mat_up"), paf_mat, upsample_size_placeholder, ops::ResizeArea::AlignCorners(false));
	   	 
	/// default == 25, but provides reasonable values down to 5 - may speed up fps by ~40% 
	auto gauss_kernel_variable = ops::Variable(scope.WithOpName("gauss_kernel_variable"), PartialTensorShape(), DT_FLOAT);
	auto gauss_kernel = ops::Placeholder(scope.WithOpName("gauss_kernel"), DT_FLOAT, ops::Placeholder::Shape(PartialTensorShape({ -1, -1, 19, 1 })));
	auto assign_gauss_kernel_variable = ops::Assign(scope.WithOpName("assign_gauss_kernel_variable"), gauss_kernel_variable, Input(gauss_kernel), ops::Assign::ValidateShape(false));

	Output gaussian_heat_mat = ops::DepthwiseConv2dNative(scope.WithOpName("gaussian_heat_mat"), heat_mat_up, gauss_kernel_variable, gtl::ArraySlice<int>({ 1,1,1,1 }), "SAME");
	Output max_pooled_in_tensor = ops::MaxPool(scope.WithOpName("max_pooled_in_tensor"), gaussian_heat_mat, gtl::ArraySlice<int>({ 1,3,3,1 }), gtl::ArraySlice<int>({1,1,1,1}), "SAME");
	Output equal_values = ops::Equal(scope.WithOpName("equal_values"), gaussian_heat_mat, max_pooled_in_tensor);
	Output tensor_peaks_coords = ops::Where(scope.WithOpName("tensor_peaks_coords"), equal_values);

	tensorflow::Status status = scope.ToGraphDef(&graph_def);
	if (!status.ok()) {
		throw status;
	}
}

void PoseEstimator::setGaussKernelSize(const size_t size) {
	GaussKernel gauss_kernel(size, 3.0, 19);

	vector<Tensor> ignore_result;
	const Status session_run = session->Run({
		{ "gauss_kernel", gauss_kernel }
		}, {
			"assign_gauss_kernel_variable"
		}, {
		},
		&ignore_result);

	if (session_run.code() != error::Code::OK) {
		throw session_run;
	}

}

const vector<Human> PoseEstimator::inference(const TensorMat & input, const int upsample_size) {
	const int upsample_height = input.tensor.dim_size(1) / 8 * upsample_size;
	const int upsample_width = input.tensor.dim_size(2) / 8 * upsample_size;
	const Tensor upsample_size_tensor = tensorflow::Input::Initializer( {upsample_height, upsample_width} ).tensor;

	vector<Tensor> post_processing;
#ifdef SPLIT_RUNS
		vector<Tensor> outputs_raw;
		Status session_run;
		session_run = session->Run({
				{"image:0", input.tensor}
			}, {
				"Openpose/concat_stage7:0"
			}, {
			},
			&outputs_raw);

		if (fetch_tensor.code() != error::Code::OK) {
			throw fetch_tensor;
		}

		Tensor& output = outputs_raw.at(0);
		session_run = session->Run({
			{"inference_tensor", output}, {"upsample_size", upsample_size_tensor}
			}, {
				"tensor_peaks_coords", "gaussian_heat_mat", "heat_mat_up", "paf_mat_up"
			}, {
			},
			&post_processing);
#else
		const Tensor gaussian_kernel_index = tensorflow::Input::Initializer( 0 ).tensor;
		const Status session_run = session->Run({
				{"image:0", input.tensor}, {"upsample_size", upsample_size_tensor}
			}, {
				"tensor_peaks_coords", "gaussian_heat_mat", "heat_mat_up", "paf_mat_up"
			}, {
			},
			&post_processing);
#endif

	if (session_run.code() != error::Code::OK) {
		throw session_run;
	}

	Tensor& coords = post_processing.at(0);
	Tensor& peaks = post_processing.at(1);
	Tensor& heat_mat = post_processing.at(2);
	Tensor& paf_mat = post_processing.at(3);

	return estimate_paf(coords, peaks, heat_mat, paf_mat, input.transform);
}

vector<Human> PoseEstimator::estimate_paf(const Tensor& coords, const Tensor& peaks, const Tensor& heat_mat, const Tensor& paf_mat, const AffineTransform& transform) {
	paf.process(
		coords.dim_size(0), coords.flat<INT64>().data(),
		peaks.dim_size(1), peaks.dim_size(2), peaks.dim_size(3), peaks.flat<float>().data(),
		heat_mat.dim_size(1), heat_mat.dim_size(2), heat_mat.dim_size(3), heat_mat.flat<float>().data(),
		paf_mat.dim_size(1), paf_mat.dim_size(2), paf_mat.dim_size(3), paf_mat.flat<float>().data());

	vector<Human> humans;
	const size_t n = paf.get_num_humans();
	for(int human_id = 0; human_id < n; ++human_id) {
		Human::BodyParts parts;
		for (int part_index = 0; part_index < 18; ++part_index) {
			const int c_idx = paf.get_part_cid(human_id, part_index);
			if (c_idx < 0) {
				continue; // body part not set
			} else {
				const Point2f point = transform({
					static_cast<float>(paf.get_part_x(c_idx)) / heat_mat.dim_size(2), 
					static_cast<float>(paf.get_part_y(c_idx)) / heat_mat.dim_size(1)
				});
				parts.insert(Human::BodyParts::value_type(
					part_index,
					BodyPart(part_index, point.x, point.y, paf.get_part_score(c_idx))));
			}
		}

		if (!parts.empty()) {
			humans.push_back(Human(parts, paf.get_score(human_id)));
		}
	}
	return humans;
}

BodyPart::BodyPart(int part_index, float x, float y, float score)
	: part_index(part_index), x(x), y(y), score(score) {}

Human::Human(const BodyParts & parts, const float score)
	: parts(parts), score(score) {}

void PoseEstimator::draw_humans(Mat& image, const AffineTransform& view, const vector<Human>& humans) const {
	for_each(humans.begin(), humans.end(), [&image, &view](const Human& human) {
		vector<Point> centers(PafProcess::COCOPAIRS_SIZE);

		for (int i = 0; i < PafProcess::COCOPAIRS_SIZE; ++i) {
			auto part = human.parts.find(i);
			if (part != human.parts.end()) {
				const BodyPart& body_part = part->second;
				const Point center = view({ body_part.x, body_part.y });
				centers[i] = center;
				const int* coco_color = PafProcess::CocoColors[i];
				const Scalar_<int> color(coco_color[0], coco_color[1], coco_color[2]);
				circle(image, center, 3, color, 3, 8, 0);
			}
		}

		// CocoPairsRender = CocoPairs[:-2] -> coco pairs but the last two elements
		for (int pair_order = 0; pair_order < PafProcess::COCOPAIRS_SIZE - 2; ++pair_order) {
			const int* pair = PafProcess::COCOPAIRS[pair_order];
		
			if (human.parts.find(pair[0]) != human.parts.end() && human.parts.find(pair[1]) != human.parts.end()) {
				const int* coco_color = PafProcess::CocoColors[pair_order];
				const Scalar_<int> color(coco_color[0], coco_color[1], coco_color[2]);
				line(image, centers[pair[0]], centers[pair[1]], color, 3);
			}
		}
	});
}
