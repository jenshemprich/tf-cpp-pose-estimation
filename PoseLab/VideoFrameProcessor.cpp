#include "pch.h"

#include "VideoFrameProcessor.h"

VideoFrameProcessor::VideoFrameProcessor(const std::function<void(QVideoFrame&)>&& function)
	: function(function)
{}

void VideoFrameProcessor::process(QVideoFrame& frame) const {
	function(frame);
	}
