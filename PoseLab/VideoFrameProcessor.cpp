#include "pch.h"

#include "VideoFrameProcessor.h"

VideoFrameProcessor::VideoFrameProcessor(const Function&& function)
	: function(function)
{}

void VideoFrameProcessor::process(QVideoFrame& frame) const {
	function(frame);
}
