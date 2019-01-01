#include "pch.h"

#include <corecrt_math_defines.h>

#include "tensorflow/core/platform/env.h"
#include "Eigen/Dense"
#include "tensorflow/core/public/session.h"
#include "tensorflow/cc/ops/standard_ops.h"

#include "tensorflow/cc/client/client_session.h"
#include "tensorflow/cc/framework/ops.h"

#include "GaussKernel.h"

using namespace tensorflow;

GaussKernel::GaussKernel(const size_t size, const float sigma, size_t channels) : tensor(gauss_kernel(size,  sigma, channels)) {
}

GaussKernel::~GaussKernel() {
}

GaussKernel::operator const tensorflow::Tensor&() const {
	return tensor;
}

// Cumulative Normal Distribution Function in C/C++
// https://stackoverflow.com/questions/2328258/cumulative-normal-distribution-function-in-c-c
double normalCFD(double value) {
	return 0.5 * erfc(-value * M_SQRT1_2);
}

// scypi Cumulative distribution function
// https://en.wikipedia.org/wiki/Cumulative_distribution_function
// https://docs.scipy.org/doc/scipy/reference/generated/scipy.stats.norm.html
Eigen::VectorXd cdf(const Eigen::VectorXd& x) {
	return x.unaryExpr([](const double x) ->double {
		return normalCFD(x);
	});
}

// numpy diff
// https://docs.scipy.org/doc/numpy-1.9.0/reference/generated/numpy.diff.html
Eigen::VectorXd diff(const Eigen::VectorXd& x) {
	Eigen::VectorXd diff(x.size() - 1);
	for (int i = 1; i < x.size(); ++i) {
		diff[i-1] = x.data()[i] - x.data()[i - 1];
	}
	return diff;
}

tensorflow::Tensor GaussKernel::gauss_kernel(const size_t kernlen, const float nsig, tensorflow::int64 channels) {
	// https://eigen.tuxfamily.org/dox/AsciiQuickReference.txt
	// interval = (2 * nsig + 1.) / (kernlen)
	const float interval = (2 * nsig + 1.) / (kernlen);

	// x = np.linspace(-nsig - interval / 2., nsig + interval / 2., kernlen + 1)
	const Eigen::VectorXd x = Eigen::VectorXd::LinSpaced(kernlen + 1 ,-nsig - interval / 2., nsig + interval / 2.);

	// kern1d = np.diff(st.norm.cdf(x))
	const Eigen::VectorXd kernel_1d = diff(cdf(x));
	// -> same values as with numpy/scypi for first 11 digits

	//	kernel_raw = np.sqrt(np.outer(kern1d, kern1d))
	const Eigen::MatrixXd kernel_raw = (kernel_1d * kernel_1d.transpose()).cwiseSqrt();
	// -> same values as with numpy/scypi

	//	kernel = kernel_raw / kernel_raw.sum()
	const Eigen::MatrixXd kernel = kernel_raw / kernel_raw.sum();
	// -> same values as with numpy/scypi

	//	out_filter = np.array(kernel, dtype = np.float32)
	const Eigen::MatrixXf kernel_f = kernel.cast<float>();

	//	out_filter = out_filter.reshape((kernlen, kernlen, 1, 1))
	const Eigen::Index kernel_1d_size = kernel_1d.size();
	Eigen::TensorMap<Eigen::Tensor<const float, 3>> gauss_kernel_2d(kernel_f.data(), { kernel_1d_size, kernel_1d_size, 1});
	
	// out_filter = np.repeat(out_filter, channels, axis = 2)
	tensorflow::Tensor gauss_kernel = Tensor(DT_FLOAT, tensorflow::TensorShape({kernel_1d_size, kernel_1d_size, channels, 1 }));
	repeat_channels_NHWC(gauss_kernel_2d, channels, gauss_kernel);

	return gauss_kernel;
}

void GaussKernel::repeat_channels_NHWC(const Eigen::TensorMap<Eigen::Tensor<const float, 3>> &kernel_2d, const tensorflow::int64 channels, tensorflow::Tensor &tensor) {
	const float* kernel_data = kernel_2d.data();
	float* tensor_data = tensor.tensor<float, 4>().data();
	for (int j = 0; j < kernel_2d.size(); ++j) {
		for (int i = 0; i < channels; ++i) {
			*(tensor_data++) = kernel_data[j];
		}
	}
}

void GaussKernel::repeat_channels_NCHW(const Eigen::TensorMap<Eigen::Tensor<const float, 3>> &kernel_2d, const tensorflow::int64 channels, tensorflow::Tensor &tensor) {
	const float* kernel_data = kernel_2d.data();
	auto tensor_data = tensor.tensor<float, 4>().data();
	for (int i = 0; i < channels; ++i) {
		memcpy(&tensor_data[kernel_2d.size() * i], kernel_data, kernel_2d.size() * sizeof(float));
	}
}
