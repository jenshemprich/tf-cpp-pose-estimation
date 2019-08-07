#include <iostream>

#include "opencv2/opencv.hpp"

// Avoid tensorflow compile error hell
#define COMPILER_MSVC
#define NOMINMAX

#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/public/session.h"
#include "Eigen/Dense"
#include "tensorflow/core/public/session.h"
#include "tensorflow/cc/ops/standard_ops.h"

#include "PoseEstimation/AffineTransform.h"
#include "PoseEstimation/CocoDataModel.h"
#include "PoseEstimation/CocoOpenCvRenderer.h"
#include "PoseEstimation/FramesPerSecond.h"
#include "PoseEstimation/GeometryOperators.h"
#include "PoseEstimation/PoseEstimator.h"
#include "PoseEstimation/TensorMat.h"

#include "PoseEstimationSimpleApp.h"


using namespace coco;
using namespace cv;
using namespace std;
using namespace tensorflow;


int main(int argc,  char* const argv[]) {
	if (argc < 2) {
		cout << "Graph path not specified" << "\n";
		return 1;
	}

	VideoCapture cap;
	try {
		boolean haveFile = argc > 2;
		if (argc > 2) {
			if (!cap.open(argv[2])) {
				cout << "File not found: " << argv[2] << "\n";
				return -1;
			}
		} else {
			if (!cap.open(0, CAP_DSHOW)) { // TODO Open issue @ OpenCV since for CAP_MSMF=default aspect ratio is 16:9 on 4:3 chip size cams
				cout << "Webcam not found: " << argv[2] << "\n";
				return 2;
			}
			cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
			cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
		}


		// TODO crop input mat so that resized image size is multiple of 16
		// TODO Better: adjust image size to multiple of heatmat * upsample_size ??
		// TODO crop input mat or add insets so that resized image size is multiple of 16 as in python code - why?
		// TODO Resize to multiple of heat/paf mat?
		// TODO Do not resize images -> do not resize unless specified on command line -> add dlib command line parser
		
		const int heat_mat_upsample_size = 4;
		const int frame_max_size = 320;
		const int gauss_kernel_size = 25;
		//const cv::Size inset = cv::Size(0, 0);
		const cv::Size inset = cv::Size(16, 16);
		//const cv::Size inset = cv::Size(32, 32);

		PoseEstimator pose_estimator(argv[1]);
		pose_estimator.loadModel();
		pose_estimator.setGaussKernelSize(gauss_kernel_size);

		auto aspect_corrected_size = [](const cv::Size& size, const int desired)->cv::Size {
			const auto aspect_ratio = size.aspectRatio();
			return aspect_ratio > 1.0 ? cv::Size(desired, desired / aspect_ratio) : cv::Size(desired / aspect_ratio, desired);
		};
		const bool displaying_images = cap.getBackendName().compare("CV_IMAGES") == 0;
		const Size frame_size = cv::Size(cap.get(cv::CAP_PROP_FRAME_WIDTH), cap.get(cv::CAP_PROP_FRAME_HEIGHT));
		const Size render_size = displaying_images ? TensorMat::AutoResize : aspect_corrected_size(frame_size, frame_max_size);

		TensorMat input = TensorMat(render_size, inset);

		FramesPerSecond fps;
		const string window_name = haveFile ? argv[2] : "this is you, smile! :)";

		for (;;) {
			Mat frame;
			cap >> frame;
			if (frame.empty()) break; // end of video stream

			const vector<Human> humans = pose_estimator.inference(input.copyFrom(frame).tensor, heat_mat_upsample_size);
			renderInference(humans, input, frame, fps,  window_name);

			if (waitKey(10) == 27) break; // ESC -> exit
			else if (displaying_images) {
				waitKey();
			}
		}
	}
	catch (const tensorflow::Status& e) {
		cout << e << endl;
	}
	catch (std::exception& e) {
		cout << e.what() << endl;
	}
	
	cap.release();
	return 0;
}

void renderInference(const std::vector<Human> &humans, const TensorMat & input, cv::Mat &frame, FramesPerSecond &fps, const std::string &window_name) {
	const cv::Size display_inset = input.inset * frame.size() / input.view.size();
	const bool render_insets = input.inset != cv::Size(0, 0);

	// TODO resolve "render_insets" condition and init all beforehand -> improved performance
	Mat display = render_insets ? Mat(display_inset + frame.size() + display_inset, CV_8UC3) : frame;

	const Rect roi = Rect(Point(display_inset), frame.size());
	if (display_inset != Size(0, 0)) {
		display = 0;
		const Mat view = display(roi);
		frame.copyTo(view);
	}

	AffineTransform view_transform(Rect2f(0.0, 0.0, 1.0, 1.0), roi);

	coco::OpenCvRenderer renderer = { display, input.transform, view_transform };

	renderer.draw(humans);
	fps.update(display);

	namedWindow(window_name, WINDOW_AUTOSIZE | WINDOW_KEEPRATIO);
	imshow(window_name, display);
}
