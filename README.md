# TensorFlow C++ Examples
This project contains some c++ examples for TensorFlow.

## Preparation
This project references the msbuild targets of the lib project at https://github.com/jenshemprich/lib. Clone the lib repository to the same folder as this project.

Then follow the build instructioons to generate AVX/AVX2/CUDA versions of the Tensorflow library for Windows. 

## HelloTensorflow
A hello-world example copied from https://joe-antognini.github.io/machine-learning/windows-tf-project.

## Pose estimation
An implementation of pose estimation in pure C++. It's a port of https://github.com/ildoonet/tf-pose-estimation/ which in turn is derived from https://github.com/CMU-Perceptual-Computing-Lab/openpose.

An in-depth article about the algorithm can be found here: https://arvrjourney.com/human-pose-estimation-using-openpose-with-tensorflow-part-2-e78ab9104fc8

With that knowledge, you can easily follow the code in PoseEstimator.cpp. It's a three step proces consisting of:
1. Inference to retrieve heat maps and part affinity fields.
2. Post-processing to retrieve coordinate candidates
3. Coco model creation from the coordinate candidates by reducing candidate coords, turn them into body part coordinates and output a coco model for each recognized pose.

The porting went relatively straight forward. Inference is basically a one-liner.

The post processing implementation is slightly different. In python you pass  operators as parameters to session.run(). In the tensorflow C++ interface you explicitely define them in an extra graph, and then merge it with the inference graph. The session can then created from a single graph that contains all operators.

I've got some trouble with using the inference output as input for the post procesisng, because tensorflow crashed on assigning a named node from the inference graph as an input for the slice operator. For the time being, it's workarounded by assinging the inference output to an intermediate add op, or alternatively by splitting up the processing into 2 session runs (one for inference, one for post processing). The issue is isloated in the Google test suite, to look into it later on.

The tensorflow related part of the post processing was relatively straight forward, the most time consuming part was to create the gaussian kernel for the convolution filter with Eigen instead of Numpy.

Because the C++ ops::where() operator outputs coordinates directly instead of producing another NHWC tensor (as the Python version tf.where), the coco model creation becomes slightly less complex than in the original code. Besides some refactoring to turn the original Python extension code into a C++ class, the redundant loops to gather the peak infos from the coordinates have been replaced by a sort statement to get them into the right order - the subsequent stages of the algorithm depend on the proper sequernce of body parts.
