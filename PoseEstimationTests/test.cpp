#include "pch.h"


// TODO testhost ignores project VC++ directory $(ExecutablePath) -> tests are not recognized by the Test Explorer
// Workaround:  copy debug versions of tensorflow.dll manually out output folder $(SolutionDir)\x64\Debug\
// TODO test if $(ExecutablePath) (set in Tools->Options->Google Test Adapter->General) works -> Nope

#include <iostream>
#include <vector>
#include <eigen/Dense>

#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/public/session.h"
#include "tensorflow/cc/ops/standard_ops.h"
#include "tensorflow/cc/framework/ops.h"


using namespace tensorflow;

TEST(TestSessionRun, Rank) {
	Scope scope = Scope::NewRootScope();

	auto input_placeholder = ops::Placeholder(scope.WithOpName("test_input"), DT_FLOAT, ops::Placeholder::Shape({ 3,2 }));
	auto test = ops::Rank(scope.WithOpName("test_output"), input_placeholder);

	GraphDef graph_def;
	EXPECT_TRUE(scope.ToGraphDef(&graph_def).ok());

	tensorflow::Session* session;
	EXPECT_TRUE(NewSession(tensorflow::SessionOptions(), &session).ok());
	EXPECT_TRUE(session->Create(graph_def).ok());


	std::vector<float> data = { 1.f, 2.f, 3.f, 4.f, 5.f, 6.f };
	auto mapped_X_ = Eigen::TensorMap<Eigen::Tensor<float, 2, Eigen::RowMajor>>(&data[0], 3, 2);
	auto eigen_X_ = Eigen::Tensor<float, 2, Eigen::RowMajor>(mapped_X_);

	Tensor input(DT_FLOAT, TensorShape({ 3, 2 }));
	input.tensor<float, 2>() = eigen_X_;

	std::vector<Tensor> output(1);
	EXPECT_TRUE(session->Run({
			{"test_input", input}
		}, {
			"test_output"
		}, {
		},
		&output).ok());
	EXPECT_EQ(output.size(), 1);

	Tensor& rank = output.at(0);
	EXPECT_EQ(rank.dims(), 1);
	EXPECT_EQ(rank.NumElements(), 1);
	EXPECT_EQ(rank.flat<int>().data()[0], 2);

	// std::cout << rank.tensor<int, 1>() << std::endl;

	session->Close();
}

TEST(TestSessionRun, RankEasy) {
	Scope scope = Scope::NewRootScope();

	auto input_placeholder = ops::Placeholder(scope.WithOpName("test_input"), DT_FLOAT, ops::Placeholder::Shape({ 3,2 }));
	auto test = ops::Rank(scope.WithOpName("test_output"), input_placeholder);

	GraphDef graph_def;
	EXPECT_TRUE(scope.ToGraphDef(&graph_def).ok());

	tensorflow::Session* session;
	EXPECT_TRUE(NewSession(tensorflow::SessionOptions(), &session).ok());
	EXPECT_TRUE(session->Create(graph_def).ok());

	const tensorflow::Tensor input = tensorflow::Input::Initializer({{ 1.f, 2.f, 3.f }, { 4.f, 5.f, 6.f }}).tensor;
	EXPECT_EQ(input.dims(), 2);
	EXPECT_EQ(input.NumElements(), 6);
	EXPECT_EQ(input.matrix<float>().data()[1], 2.0);

	std::vector<Tensor> output(1);
	EXPECT_TRUE(session->Run({
			{"test_input", input}
		}, {
			"test_output"
		}, {
		},
		&output).ok());
	EXPECT_EQ(output.size(), 1);

	Tensor& rank = output.at(0);
	EXPECT_EQ(rank.dims(), 1);
	EXPECT_EQ(rank.NumElements(), 1);
	EXPECT_EQ(rank.flat<int>().data()[0], 2);

	// std::cout << rank.tensor<int, 1>() << std::endl;

	session->Close();
}

TEST(TestSessionRun, AssignPartialShapeToVariable) {
	Scope scope = Scope::NewRootScope();

	PartialTensorShape partialTensorShape = PartialTensorShape({ -1, -1, 19, 1 });
	auto variable = ops::Variable(scope.WithOpName("variable"), PartialTensorShape(), DT_FLOAT);
	auto placeholder = ops::Placeholder(scope.WithOpName("input"), DT_FLOAT, ops::Placeholder::Shape(partialTensorShape));
	auto assign = ops::Assign(scope.WithOpName("assign"), variable, Input(placeholder), ops::Assign::ValidateShape(false));

	GraphDef graph_def;
	EXPECT_TRUE(scope.ToGraphDef(&graph_def).ok());

	tensorflow::Session* session;
	EXPECT_TRUE(NewSession(tensorflow::SessionOptions(), &session).ok());
	EXPECT_TRUE(session->Create(graph_def).ok());

	Tensor tensor = Tensor(DT_FLOAT, TensorShape({ 25, 25, 19, 1 }));
	Status init_variables;
	std::vector<Tensor> output;
	EXPECT_TRUE(session->Run({
		{ "input", tensor }
		}, {
			"assign"
		}, {
		},
		&output).ok());

	session->Close();
}

TEST(TestSessionRun, SelectVariableByIndex) {
	Scope scope = Scope::NewRootScope();

	auto variable1 = ops::Variable(scope.WithOpName("variable1"), PartialTensorShape(), DT_FLOAT);
	auto variable2 = ops::Variable(scope.WithOpName("variable2"), PartialTensorShape(), DT_FLOAT);
	auto variables = InputList(std::initializer_list<Output>({ variable1, variable2 }));

	auto index = ops::Placeholder(scope.WithOpName("index"), DT_INT32, ops::Placeholder::Shape({  }));
	Output selector = ops::RefSelect(scope.WithOpName("selector"), index, variables);
}