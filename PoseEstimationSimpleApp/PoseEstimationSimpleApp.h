#pragma once

int main(int argc, char * const argv[]);
void renderInference(const std::vector<coco::Human> &humans, const TensorMat & input, cv::Mat &frame, FramesPerSecond &fps, const std::string &window_name);
