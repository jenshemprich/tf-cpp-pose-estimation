#pragma once
class GaussKernel
{
public:
	GaussKernel(const size_t size, const float sigma, size_t channels);
	~GaussKernel();

	operator const tensorflow::Tensor&() const;

private:
	tensorflow::Tensor tensor;

	static tensorflow::Tensor gauss_kernel(const size_t size, const float sigma, tensorflow::int64 channels);
	static void repeat_channels_NHWC(const Eigen::TensorMap<Eigen::Tensor<const float, 3>> &gauss_kernel_2d, const tensorflow::int64 channels, tensorflow::Tensor &gauss_kernel);
	static void repeat_channels_NCHW(const Eigen::TensorMap<Eigen::Tensor<const float, 3>> &gauss_kernel_2d, const tensorflow::int64 channels, tensorflow::Tensor &gauss_kernel);
};

