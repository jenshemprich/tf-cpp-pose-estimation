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
	TensorMat(const cv::Size& size);
	TensorMat(const cv::Size& size, const cv::Size& inset);
	virtual ~TensorMat();

	TensorMat& copyFrom(const cv::Mat& mat);
	TensorMat& resize(const cv::Size& size, const cv::Size& inset);

	tensorflow::ReAllocator allocator;
	tensorflow::Tensor tensorBuffer;
	const tensorflow::Tensor& tensor;
private:
	cv::Size size;
	cv::Size inset;
	cv::Mat buffer;
	cv::Mat view;
public:
	AffineTransform transform;
};
