#include "pch.h"

#include <iostream>

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
using namespace tensorflow;

// TODO target size, up_scale for heat maps etc.
PoseEstimator::PoseEstimator(char * const graph_file, int target_width, int target_height)
	: graph_file(graph_file), session(nullptr)
	, image_tensor(DT_FLOAT, TensorShape({ 1, target_height, target_width, 3 })) {
	// create a "fake" cv::Mat from the tensor memory
	float *p = image_tensor.flat<float>().data();
	image_mat = new cv::Mat(target_height, target_width, CV_32FC3, p);
	resized = new cv::Mat(target_height, target_width, CV_8UC3);

}

PoseEstimator::~PoseEstimator() {
	if (image_mat) delete image_mat;
	if (resized) delete resized;
}

tensorflow::error::Code PoseEstimator::loadModel() {
	cout << "Loading graph...\n";
	Status load_graph_status = ReadBinaryProto(tensorflow::Env::Default(), graph_file, &graph_def);
	if (!load_graph_status.ok()) {
		cerr << load_graph_status;
		return tensorflow::errors::NotFound("Failed to load compute graph from '", graph_file, "'").code();
	}

	GraphDef postProcessing;
	Status addPostProcessingStatus = addPostProcessing(postProcessing);
	if (!addPostProcessingStatus.ok()) {
		cout << addPostProcessingStatus.ToString() << "\n";
		return addPostProcessingStatus.code();
	}

	graph_def.MergeFrom(postProcessing);

	cout << "Creating session...\n";
	Status status = NewSession(tensorflow::SessionOptions(), &session);
	if (!status.ok()) {
		cout << status.ToString() << "\n";
		return status.code();
	}

	// Build static app example code (non-working with dll):
	// https://github.com/tensorflow/tensorflow/blob/master/tensorflow/examples/label_image/main.cc
	// session->Reset(tensorflow::NewSession(tensorflow::SessionOptions()));
	// auto s = new std::unique_ptr<tensorflow::Session>(session);
	// s->reset(tensorflow::NewSession(tensorflow::SessionOptions()));

	Status session_create_status = session->Create(graph_def);
	if (!session_create_status.ok()) {
		cerr << session_create_status;
		return session_create_status.code();
	}

	return error::Code::OK;
}

// #define SPLIT_RUNS

tensorflow::Status PoseEstimator::addPostProcessing(GraphDef& graph_def) {
	Scope scope = Scope::NewRootScope();
	auto upsample_size_placeholder = ops::Placeholder(scope.WithOpName("upsample_size"), DT_INT32, ops::Placeholder::Shape({  }));
#ifdef SPLIT_RUNS
	auto inference_tensor_placeholder = ops::Placeholder(scope.WithOpName("inference_tensor"), DT_FLOAT, ops::Placeholder::Shape({ -1, -1, -1, 57 }));
#else
	auto Z = ops::Const(scope.WithOpName("z"), { { 0.f } });
	auto node_from_other_graph = Input("Openpose/concat_stage7", 0, DT_FLOAT);
	auto inference_tensor_placeholder = ops::Add(scope.WithOpName("c"), Z, node_from_other_graph);
#endif
	// TODO Needs two runs in the same session

	// Crashes badly,and usage islimited according to docs
	//  auto inference_tensor_placeholder = ops::Placeholder(scope.WithOpName("inference_tensor"), DT_FLOAT, ops::Placeholder::Shape({ -1, -1, -1, 57 }));

	// TODO inference and post-process in same session run by telling heat_mat and paf_mat to directly use Output tensor Openpose/concat_stage7:0
	// TODO strings aren't accepted as input
	// auto inference_tensor_placeholder = Input::Initializer("Openpose/concat_stage7:0");
	// TODO need to add :0 but this causes INVALID_CHARACTER, and without the node reference is not unique
	// -> because the placeholder tries to create a node with the same name as in the model -> how to reference it?
	// auto inference_tensor_placeholder = ops::Placeholder(scope.WithOpName("Openpose/concat_stage7:0"), DT_FLOAT, ops::Placeholder::Shape({ -1, -1, -1, 57 }));
	// TODO Looks like the way to go but aborts with code 3 when trying to add as input to heat_map
	// auto inference_tensor_placeholder = Input("Openpose/concat_stage7",0, DT_FLOAT);
	// TODO Aborts for existing name, so it's not specific to unknown node
	// auto inference_tensor_placeholder = Input("Openpose/concat_stage7",0, DT_FLOAT);

	// Not a node reference but hardcoded tensor values
	// auto node = ops::AsNodeOut(scope, "Openpose/concat_stage7:0").node;
	// Output test = ops::ResizeArea(scope.WithOpName("test"), node, upsample_size_placeholder);
	//	auto inference_tensor_placeholder = Node({ "Openpose/concat_stage7:0" });

	Output heat_mat = ops::Slice(scope.WithOpName("heat_mat"), inference_tensor_placeholder, Input::Initializer({ 0, 0, 0, 0 }), Input::Initializer({ -1, -1, -1, 19 }));
	Output paf_mat = ops::Slice(scope.WithOpName("paf_mat"), inference_tensor_placeholder, Input::Initializer({ 0, 0, 0, 19 }), Input::Initializer({ -1, -1, -1, 38 }));

	Output heat_mat_up = ops::ResizeArea(scope.WithOpName("heat_mat_up"), heat_mat, upsample_size_placeholder, ops::ResizeArea::AlignCorners(false));
	Output paf_mat_up = ops::ResizeArea(scope.WithOpName("paf_mat_up"), paf_mat, upsample_size_placeholder, ops::ResizeArea::AlignCorners(false));


	// smoother = Smoother({ 'data': self.tensor_heatMat_up }, 25, 3.0)
	// TODO gauss_kernel is NCHW, but declared as NHWC -> filtering wrong
	GaussKernel gauss_kernel(25, 3.0, 19);
	//	gaussian_heatMat = smoother.get_output()
	Output gaussian_heat_mat = ops::DepthwiseConv2dNative(scope.WithOpName("gaussian_heat_mat"), heat_mat_up, Input(gauss_kernel), gtl::ArraySlice<int>({ 1,1,1,1 }), "SAME");

	// max_pooled_in_tensor = tf.nn.pool(gaussian_heatMat, window_shape = (3, 3), pooling_type = 'MAX', padding = 'SAME')
	// self.tensor_peaks = tf.where(tf.equal(gaussian_heatMat, max_pooled_in_tensor), gaussian_heatMat, tf.zeros_like(gaussian_heatMat))
	// TODO array strides are probably wrong
	Output max_pooled_in_tensor = ops::MaxPool(scope.WithOpName("max_pooled_in_tensor"), gaussian_heat_mat, gtl::ArraySlice<int>({ 1,3,3,1 }), gtl::ArraySlice<int>({1,1,1,1}), "SAME");
	Output equal_values = ops::Equal(scope.WithOpName("equal_values"), gaussian_heat_mat, max_pooled_in_tensor);
	// Output zeros_like_gaussian_heat_mat = ops::ZerosLike(scope.WithOpName("zeros_like_gaussian_heat_mat"), gaussian_heat_mat);
	// TODO tf.where uses equals as a mask for copying values from gaussian_heat_mat, whereas ops::Where returns coordinates instead
	// Output tensor_peaks = tf.where(scope.WithOpName("tensor_peaks"), equal_values, gaussian_heat_mat, zeros_like_gaussian_heat_mat);
	Output tensor_peaks_coords = ops::Where(scope.WithOpName("tensor_peaks_coords"), equal_values);


	// TODO Name op (other than Slice:0)
//	cout << heat_mat.name() << endl;
//	cout << paf_mat.name() << endl;

	return scope.ToGraphDef(&graph_def);
}

#include "opencv2/opencv.hpp"

// TODO Specify target size here to achieve flexible runtime cpu usage -> separate targetSize & upsapleSize from estimator logic
tensorflow::error::Code PoseEstimator::inference(const cv::Mat& frame, const int upsample_size, vector<Human>& humans) {
	cv::resize(frame, *resized, resized->size());

	resized->convertTo(*image_mat, CV_32FC3);

	// TODO image tensor is checked to be qint8 (qunatizized image) -> see about that later

	// In python, the non-learning part can be defined via place holders and global functions
	// -> tensor_peaks is defined via a lot of tf.* python functions
	// when the session is run in python, the outputs re fed into the placeholders
	// -> upsample_size becomes a feed input of tensor_peaks via the feed_dict, but is not an input of the neural network
	// To build the final graph, in python it is necessary to call tf.variables_initializer
	// -> tf.variables_initializer build the complete graph

	// Why the additionalwarm-up calls for self.tensor_peaks, self.tensor_heatMat_up, self.tensor_pafMat_up with different upsample_sizes?
	// - none, they can be commented out, they're noops

	// THese are derived from output tensor, and the data type is set at runtime (tf.session.run)

	// This is all to run the session
	// Python has a global session object, but all ondes are imported under "TfPoseEstimator/*"
	// Here the session is local,with no prefix, therefore "TfPoseEstimator/" is ommited

	const int upsample_height = image_tensor.dim_size(1) / 8 * upsample_size;
	const int upsample_width = image_tensor.dim_size(2) / 8 * upsample_size;
	const tensorflow::Tensor upsample_size_tensor = tensorflow::Input::Initializer( {upsample_height, upsample_width} ).tensor;

	Status fetch_tensor;
	std::vector<Tensor> post_processing;
#ifdef SPLIT_RUNS
		std::vector<Tensor> outputs_raw;
		fetch_tensor = session->Run({
				{"image:0", image_tensor}
			}, {
				"Openpose/concat_stage7:0"
			}, {
			},
			&outputs_raw);

		if (fetch_tensor.code() != error::Code::OK) {
			cerr << fetch_tensor;
			return fetch_tensor.code();
		}

		// TODO Need two session runs because post processing is built in different scope than model
		// They're joined later on in the build process, but I've no clue yet
		// how to access the placeholder reference "Openpose/concat_stage7:0" directly and use it as input for ops::Slice
		Tensor& output = outputs_raw.at(0);
		fetch_tensor = session->Run({
			{"inference_tensor", output}, {"upsample_size", upsample_size_tensor}
			}, {
				"tensor_peaks_coords", "gaussian_heat_mat", "heat_mat_up", "paf_mat_up"
			}, {
			},
			&post_processing);
#else
		fetch_tensor = session->Run({
				{"image:0", image_tensor}, {"upsample_size", upsample_size_tensor}
			}, {
				"tensor_peaks_coords", "gaussian_heat_mat", "heat_mat_up", "paf_mat_up"
			}, {
			},
			&post_processing);
#endif

	if (fetch_tensor.code() != error::Code::OK) {
		cerr << fetch_tensor;
		return fetch_tensor.code();
	}

	Tensor& coords = post_processing.at(0);
	Tensor& peaks = post_processing.at(1);
	Tensor& heat_mat = post_processing.at(2);
	Tensor& paf_mat = post_processing.at(3);

	cout << "   coords \t dims=" << coords.dims() << " size=" << coords.dim_size(0) << "," << coords.dim_size(1) << endl;
	const INT64* data = coords.flat<INT64>().data();
	cout << "\t\t values[0]=" << data[0] << "," << data[1] << "," << data[2] << "," << data[3] << endl;
	cout << "    peaks \t dims=" << peaks.dims() << " size=" << peaks.dim_size(0) << "," << peaks.dim_size(1) << "," << peaks.dim_size(2) << "," << peaks.dim_size(3) << endl;
	cout << " heat_mat \t dims=" << heat_mat.dims()<< " size=" << heat_mat.dim_size(0) << "," << heat_mat.dim_size(1) <<","<< heat_mat.dim_size(2) << "," << heat_mat.dim_size(3) << endl;
	cout << "  paf_mat \t dims=" << paf_mat.dims() << " size=" << paf_mat.dim_size(0) << "," << paf_mat.dim_size(1)<< "," << paf_mat.dim_size(2) << "," << paf_mat.dim_size(3) << endl;

	estimate_paf(coords, peaks, heat_mat, paf_mat, humans);

	// Without max pooling there are far too many peaks
	//estimate_paf(peaks, heat_mat, paf_mat, humans);

	return tensorflow::error::Code::OK;
}

void PoseEstimator::draw_humans(cv::Mat& image, const vector<Human> humans) const {
	// TODO Create humans

	//def draw_humans(npimg, humans, imgcopy = False) :
	//	if imgcopy :
	//		npimg = np.copy(npimg)
	//		image_h, image_w = npimg.shape[:2]
	//		centers = {}
	//		for human in humans :
	//# draw point
	//	for i in range(common.CocoPart.Background.value) :
	//		if i not in human.body_parts.keys() :
	//			continue

	//			body_part = human.body_parts[i]
	//			center = (int(body_part.x * image_w + 0.5), int(body_part.y * image_h + 0.5))
	//			centers[i] = center
	//			cv2.circle(npimg, center, 3, common.CocoColors[i], thickness = 3, lineType = 8, shift = 0)

	//			# draw line
	//			for pair_order, pair in enumerate(common.CocoPairsRender) :
	//				if pair[0] not in human.body_parts.keys() or pair[1] not in human.body_parts.keys() :
	//					continue

	//					# npimg = cv2.line(npimg, centers[pair[0]], centers[pair[1]], common.CocoColors[pair_order], 3)
	//					cv2.line(npimg, centers[pair[0]], centers[pair[1]], common.CocoColors[pair_order], 3)

	//					return npimg
	std::for_each(humans.begin(), humans.end(), [&image](const Human& human) {
		//for(human.body_parts
	});
}

void PoseEstimator::imshow(const char * caption, tensorflow::Tensor & tensor, int channel) {
	float* data = tensor.flat<float>().data();
	imshow(caption, tensor.dim_size(2), tensor.dim_size(1), &data[0, 0, 0, channel]);
}

void PoseEstimator::imshow(const char* caption, cv::Mat& mat) {
	cv::imshow(caption, mat);
}

void PoseEstimator::imshow(const char* caption, const size_t width, const size_t height, float * data) {
	cv::Mat tensor_plane(height, width, CV_32FC1, data);
	cv::Mat debug_output(height, width, CV_8UC1);
	// tensor_plane.convertTo(debug_output, CV_8UC1, 127, 128);
	cv::resize(tensor_plane, debug_output, cv::Size(width * 4, height * 4));
	cv::imshow(caption, debug_output);
}

void PoseEstimator::estimate_paf(const Tensor& coords, const Tensor& peaks, const Tensor& heat_mat, const Tensor& paf_mat, vector<Human>& humans) {
	// pafprocess.cpp runs over all peak planes and collets heatmap values into a Peak info vector
	// -> there might be a chance to replace this with ops::Where but the risk of falure is hight at my current point of knowledge
	// On the other hand, ops::Where collects the coordinates of the peaks, so we could just look up those in the heatmap to collect the vector<Peak>

	process_paf(
		coords.dim_size(0), coords.flat<INT64>().data(),
		peaks.dim_size(1), peaks.dim_size(2), peaks.dim_size(3), peaks.flat<float>().data(),
		heat_mat.dim_size(1), heat_mat.dim_size(2), heat_mat.dim_size(3), heat_mat.flat<float>().data(),
		paf_mat.dim_size(1), paf_mat.dim_size(2), paf_mat.dim_size(3), paf_mat.flat<float>().data());

	cout << "Number of humans = " << get_num_humans() << endl;
}
