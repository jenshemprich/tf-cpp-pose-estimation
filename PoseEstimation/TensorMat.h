#pragma once

#include "AffineTransform.h"

namespace tensorflow {
	class ReAllocator : public Allocator {
	public:
		ReAllocator();
		virtual ~ReAllocator() override;
		virtual string Name() override;
		virtual void* AllocateRaw(size_t alignment, size_t num_bytes) override;
		virtual void DeallocateRaw(void* ptr) override;
	private:
		void* buffer;
		size_t size;
	};
}

class TensorMat {
public:
	static const cv::Size AutoResize;

	TensorMat(const cv::Size& size);
	TensorMat(const cv::Size& size, const cv::Size& inset);
	virtual ~TensorMat();

	TensorMat& copyFrom(const cv::Mat& mat);
	TensorMat& resize(const cv::Size& size);
	TensorMat& resize(const cv::Size& size, const cv::Size& inset);

	tensorflow::ReAllocator allocator;
	tensorflow::Tensor tensorBuffer;
	const tensorflow::Tensor& tensor;
private:
	cv::Mat buffer;
	cv::Size size;
public:
	cv::Size inset;
	cv::Mat view;
	AffineTransform transform;
};
