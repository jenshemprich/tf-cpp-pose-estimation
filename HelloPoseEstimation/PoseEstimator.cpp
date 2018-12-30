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

PoseEstimator::PoseEstimator(char * const graph_file, int target_width, int target_height)
	: graph_file(graph_file), session(nullptr)
	, image_tensor(DT_FLOAT, TensorShape({ 1, target_height, target_width, 3 })) {
	float *data = image_tensor.flat<float>().data();
	image_mat = new cv::Mat(target_height, target_width, CV_32FC3, data);
	resized = new cv::Mat(target_height, target_width, CV_8UC3);

}

PoseEstimator::~PoseEstimator() {
	if (image_mat) delete image_mat;
	if (resized) delete resized;
}

void PoseEstimator::loadModel() {
	cout << "Loading graph...\n";
	Status load_graph_status = ReadBinaryProto(tensorflow::Env::Default(), graph_file, &graph_def);
	if (!load_graph_status.ok()) {
		throw tensorflow::errors::NotFound("Failed to load compute graph from '", graph_file, "'");
	}

	GraphDef postProcessing;
	addPostProcessing(postProcessing);
	graph_def.MergeFrom(postProcessing);

	cout << "Creating session...\n";
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

void PoseEstimator::addPostProcessing(GraphDef& graph_def) {
	Scope scope = Scope::NewRootScope();
	auto upsample_size_placeholder = ops::Placeholder(scope.WithOpName("upsample_size"), DT_INT32, ops::Placeholder::Shape({  }));
#ifdef SPLIT_RUNS
	auto inference_tensor_placeholder = ops::Placeholder(scope.WithOpName("inference_tensor"), DT_FLOAT, ops::Placeholder::Shape({ -1, -1, -1, 57 }));
#else
	auto Z = ops::Const(scope.WithOpName("noop"), { { 0.f } });
	auto node_from_other_graph = Input("Openpose/concat_stage7", 0, DT_FLOAT);
	// TODO Resolve crash when slicing directly and remove the ops::Add(...) workaround (see unit tests)
	auto inference_tensor_placeholder = ops::Add(scope.WithOpName("workaround"), Z, node_from_other_graph);
#endif
	Output heat_mat = ops::Slice(scope.WithOpName("heat_mat"), inference_tensor_placeholder, Input::Initializer({ 0, 0, 0, 0 }), Input::Initializer({ -1, -1, -1, 19 }));
	Output paf_mat = ops::Slice(scope.WithOpName("paf_mat"), inference_tensor_placeholder, Input::Initializer({ 0, 0, 0, 19 }), Input::Initializer({ -1, -1, -1, 38 }));

	Output heat_mat_up = ops::ResizeArea(scope.WithOpName("heat_mat_up"), heat_mat, upsample_size_placeholder, ops::ResizeArea::AlignCorners(false));
	Output paf_mat_up = ops::ResizeArea(scope.WithOpName("paf_mat_up"), paf_mat, upsample_size_placeholder, ops::ResizeArea::AlignCorners(false));

	// smoother = Smoother({ 'data': self.tensor_heatMat_up }, 25, 3.0)
	GaussKernel gauss_kernel(25, 3.0, 19);
	//	gaussian_heatMat = smoother.get_output()
	Output gaussian_heat_mat = ops::DepthwiseConv2dNative(scope.WithOpName("gaussian_heat_mat"), heat_mat_up, Input(gauss_kernel), gtl::ArraySlice<int>({ 1,1,1,1 }), "SAME");

	// max_pooled_in_tensor = tf.nn.pool(gaussian_heatMat, window_shape = (3, 3), pooling_type = 'MAX', padding = 'SAME')
	Output max_pooled_in_tensor = ops::MaxPool(scope.WithOpName("max_pooled_in_tensor"), gaussian_heat_mat, gtl::ArraySlice<int>({ 1,3,3,1 }), gtl::ArraySlice<int>({1,1,1,1}), "SAME");

	// self.tensor_peaks = tf.where(tf.equal(gaussian_heatMat, max_pooled_in_tensor), gaussian_heatMat, tf.zeros_like(gaussian_heatMat))
	Output equal_values = ops::Equal(scope.WithOpName("equal_values"), gaussian_heat_mat, max_pooled_in_tensor);
	// tf_cc ops::Where output is a coordinate tensor
	Output tensor_peaks_coords = ops::Where(scope.WithOpName("tensor_peaks_coords"), equal_values);

	tensorflow::Status status = scope.ToGraphDef(&graph_def);
	if (!status.ok()) {
		throw status;
	}
}

// TODO Specify target_size here to achieve flexible runtime cpu usage -> separate target_size & upsample_size from estimator logic
std::vector<Human> PoseEstimator::inference(const cv::Mat& frame, const int upsample_size) {
	cv::resize(frame, *resized, resized->size());

	resized->convertTo(*image_mat, CV_32FC3);

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
			throw fetch_tensor;
		}

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
		throw fetch_tensor;
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

	return estimate_paf(coords, peaks, heat_mat, paf_mat);
}

void PoseEstimator::draw_humans(cv::Mat& image, const vector<Human>& humans) const {

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

// TODO Extract single or multiple layers from NHWC (channels are interleaved)
void PoseEstimator::imshow(const char* caption, const size_t width, const size_t height, float * data) {
	cv::Mat tensor_plane(height, width, CV_32FC1, data);
	cv::Mat debug_output(height, width, CV_8UC1);
	// tensor_plane.convertTo(debug_output, CV_8UC1, 127, 128);
	cv::resize(tensor_plane, debug_output, cv::Size(width * 4, height * 4));
	cv::imshow(caption, debug_output);
}

vector<Human> PoseEstimator::estimate_paf(const Tensor& coords, const Tensor& peaks, const Tensor& heat_mat, const Tensor& paf_mat) {
	paf.process(
		coords.dim_size(0), coords.flat<INT64>().data(),
		peaks.dim_size(1), peaks.dim_size(2), peaks.dim_size(3), peaks.flat<float>().data(),
		heat_mat.dim_size(1), heat_mat.dim_size(2), heat_mat.dim_size(3), heat_mat.flat<float>().data(),
		paf_mat.dim_size(1), paf_mat.dim_size(2), paf_mat.dim_size(3), paf_mat.flat<float>().data());
	cout << "Number of humans = " << paf.get_num_humans() << endl;

	// TODO Easy as pie...
	vector<Human>&& humans = vector<Human>();

	//def estimate_paf(peaks, heat_mat, paf_mat) :
	//	pafprocess.process_paf(peaks, heat_mat, paf_mat)

	//	humans = []
	//	for human_id in range(pafprocess.get_num_humans()) :
	//		human = Human([])
	//		is_added = False

	//		for part_idx in range(18) :
	//			c_idx = int(pafprocess.get_part_cid(human_id, part_idx))
	//			if c_idx < 0 :
	//				continue

	//				is_added = True
	//				human.body_parts[part_idx] = BodyPart(
	//					'%d-%d' % (human_id, part_idx), part_idx,
	//					float(pafprocess.get_part_x(c_idx)) / heat_mat.shape[1],
	//					float(pafprocess.get_part_y(c_idx)) / heat_mat.shape[0],
	//					pafprocess.get_part_score(c_idx)
	//				)

	//				if is_added:
	//					score = pafprocess.get_score(human_id)
	//					human.score = score
	//					humans.append(human)

	//	return humans
	return humans;
}
