#include "pch.h"


// TODO testhost ignores $(ExecutablePath) - copied debug tensorflow.dll manually to output folder
// -> tests not recognized
// TODO test if $(ExecutablePath) (set in Google Test Adapter options) works
// - likely not being evaluated


#include <iostream>
#include <vector>

#include <eigen/Dense>

#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/public/session.h"
#include "tensorflow/cc/ops/standard_ops.h"
#include "tensorflow/cc/framework/ops.h"
#include "test.h"

using namespace tensorflow;

void CreateExampleGraphDef(GraphDef& def) {
	Scope root = Scope::NewRootScope();

	auto X = ops::Placeholder(root.WithOpName("x"), DT_FLOAT, ops::Placeholder::Shape({ -1, 2 }));
	auto Z = ops::Const(root.WithOpName("z"), { { 3.f, 2.f },{ -1.f, 0.f } });
	auto Y = ops::MatMul(root.WithOpName("y"), Z, X, ops::MatMul::TransposeB(true));

	TF_CHECK_OK(root.ToGraphDef(&def));
}

void CreateAnotherExampleGraphDef(GraphDef& def) {
	Scope root = Scope::NewRootScope();

	auto A = ops::Placeholder(root.WithOpName("a"), DT_FLOAT, ops::Placeholder::Shape({ 2, 2 }));
	auto B = ops::Const(root.WithOpName("b"), { { 5.f, 6.f },{ 7.f, 8.f } });
	auto C = ops::Add(root.WithOpName("c"), A, B);

	TF_CHECK_OK(root.ToGraphDef(&def));
}

TEST(GraphBuilding, TestExampleTrainerOriginal) {
	GraphDef graph_def;
	CreateExampleGraphDef(graph_def);

	std::unique_ptr<Session> session(NewSession(SessionOptions()));
	TF_CHECK_OK(session->Create(graph_def));

	// Define some data.  This needs to be converted to an Eigen Tensor to be
	// fed into the placeholder.  Note that this will be broken up into two
	// separate vectors of length 2: [1, 2] and [3, 4], which will separately
	// be multiplied by the matrix.
	std::vector<float> data = { 1, 2, 3, 4 };
	auto mapped_X_ = Eigen::TensorMap<Eigen::Tensor<float, 2, Eigen::RowMajor>>
		(&data[0], 2, 2);
	auto eigen_X_ = Eigen::Tensor<float, 2, Eigen::RowMajor>(mapped_X_);

	Tensor X_(DT_FLOAT, TensorShape({ 2, 2 }));
	X_.tensor<float, 2>() = eigen_X_;

	std::vector<Tensor> outputs;
	TF_CHECK_OK(session->Run({ { "x", X_ } }, { "y" }, {}, &outputs));

	// Get the result and print it out
	Tensor Y_ = outputs[0];
	EXPECT_EQ(Y_.dims(), 2);
	std::cout << Y_.tensor<float, 2>() << std::endl;

	// result:
	//  7 17
	// -1 - 3

	EXPECT_EQ(Y_.flat<float>().data()[0], 7.f);
	EXPECT_EQ(Y_.flat<float>().data()[1], 17.f);
	EXPECT_EQ(Y_.flat<float>().data()[2], -1.f);
	EXPECT_EQ(Y_.flat<float>().data()[3], -3.f);

	session->Close();
}


TEST(GraphBuilding, TestExampleTrainerSimplified) {
	GraphDef graph_def;
	CreateExampleGraphDef(graph_def);

	std::unique_ptr<Session> session(NewSession(SessionOptions()));
	TF_CHECK_OK(session->Create(graph_def));

	Tensor X_ = Input::Initializer({ { 1.f, 2.f },{ 3.f, 4.f } }).tensor;
	std::vector<Tensor> outputs;
	TF_CHECK_OK(session->Run({ { "x", X_ } }, { "y" }, {}, &outputs));

	Tensor Y_ = outputs[0];
	EXPECT_EQ(Y_.dims(), 2);
	EXPECT_EQ(Y_.flat<float>().data()[0], 7.f);
	EXPECT_EQ(Y_.flat<float>().data()[1], 17.f);
	EXPECT_EQ(Y_.flat<float>().data()[2], -1.f);
	EXPECT_EQ(Y_.flat<float>().data()[3], -3.f);

	session->Close();
}

TEST(GraphBuilding, TestMergeConstructedGraphIntoConstructed) {
	GraphDef example_graph_def;
	CreateExampleGraphDef(example_graph_def);
	GraphDef another_graph_def;
	CreateAnotherExampleGraphDef(another_graph_def);

	another_graph_def.MergeFrom(example_graph_def);

	std::unique_ptr<Session> session(NewSession(SessionOptions()));
	TF_CHECK_OK(session->Create(another_graph_def));

	Tensor X_ = Input::Initializer({ { 1.f, 2.f },{ 3.f, 4.f } }).tensor;
	std::vector<Tensor> outputs;

	TF_CHECK_OK(session->Run({ { "x", X_ } }, { "y" }, {}, &outputs));
	Tensor Y_ = outputs[0];
	EXPECT_EQ(Y_.dims(), 2);
	EXPECT_EQ(Y_.flat<float>().data()[0], 7.f);
	EXPECT_EQ(Y_.flat<float>().data()[1], 17.f);
	EXPECT_EQ(Y_.flat<float>().data()[2], -1.f);
	EXPECT_EQ(Y_.flat<float>().data()[3], -3.f);

	TF_CHECK_OK(session->Run({ { "a", X_ } }, { "c" }, {}, &outputs));

	Tensor C_ = outputs[0];
	EXPECT_EQ(C_.dims(), 2);
	EXPECT_EQ(C_.flat<float>().data()[0], 6.f);
	EXPECT_EQ(C_.flat<float>().data()[1], 8.f);
	EXPECT_EQ(C_.flat<float>().data()[2], 10.f);
	EXPECT_EQ(C_.flat<float>().data()[3], 12.f);

	session->Close();
}

TEST(GraphBuilding, TestReferenceBackEdge) {
	Scope scope(Scope::NewRootScope());
	auto A = ops::Placeholder(scope.WithOpName("a"), DT_FLOAT, ops::Placeholder::Shape({ 2, 2 }));
	auto node_from_other_graph = Input("z", 0, DT_FLOAT);
	auto C = ops::Add(scope.WithOpName("c"), A, node_from_other_graph);
	GraphDef another_graph_def;
	TF_CHECK_OK(scope.ToGraphDef(&another_graph_def));
	
	GraphDef example_graph_def;
	CreateExampleGraphDef(example_graph_def);

	example_graph_def.MergeFrom(another_graph_def);

	std::unique_ptr<Session> session(NewSession(SessionOptions()));
	TF_CHECK_OK(session->Create(example_graph_def));

	Tensor X_ = Input::Initializer({ { 1.f, 2.f },{ 3.f, 4.f } }).tensor;
	std::vector<Tensor> outputs;

	TF_CHECK_OK(session->Run({ { "x", X_ } }, { "y" }, {}, &outputs));
	Tensor Y_ = outputs[0];
	EXPECT_EQ(Y_.dims(), 2);
	EXPECT_EQ(Y_.flat<float>().data()[0], 7.f);
	EXPECT_EQ(Y_.flat<float>().data()[1], 17.f);
	EXPECT_EQ(Y_.flat<float>().data()[2], -1.f);
	EXPECT_EQ(Y_.flat<float>().data()[3], -3.f);

	TF_CHECK_OK(session->Run({ { "a", X_ } }, { "c" }, {}, &outputs));

	Tensor C_ = outputs[0];
	EXPECT_EQ(C_.dims(), 2);
	EXPECT_EQ(C_.flat<float>().data()[0], 4.f);
	EXPECT_EQ(C_.flat<float>().data()[1], 4.f);
	EXPECT_EQ(C_.flat<float>().data()[2], 2.f);
	EXPECT_EQ(C_.flat<float>().data()[3], 4.f);

	session->Close();
}


TEST(GraphBuilding, TestIdentityOp) {
	Scope scope(Scope::NewRootScope());

	auto definedBackEdge = ops::Const(scope.WithOpName("backEdge"), Input::Initializer({ 0.f,0.f,0.f,0.f }));

	auto identity = ops::Identity(scope.WithOpName("test"), definedBackEdge);
	GraphDef another_graph_def;
	TF_CHECK_OK(scope.ToGraphDef(&another_graph_def));
}

TEST(GraphBuilding, TestIdentityOp_DefinedBackEdge) {
	Scope scope(Scope::NewRootScope());

	auto definedBackEdge = ops::Const(scope.WithOpName("backEdge"), Input::Initializer({ 0.f,0.f,0.f,0.f }));

	auto backEdge = Input("backEdge", 0, DT_FLOAT);
	auto identity = ops::Identity(scope.WithOpName("test"), backEdge);
	GraphDef another_graph_def;
	TF_CHECK_OK(scope.ToGraphDef(&another_graph_def));
}

TEST(GraphBuilding, TestIdentityOp_UndefinedBackEdge) {
	Scope scope(Scope::NewRootScope());

	auto backEdge = Input("backEdge", 0, DT_FLOAT);
	auto identity = ops::Identity(scope.WithOpName("test"), backEdge);
	GraphDef another_graph_def;
	TF_CHECK_OK(scope.ToGraphDef(&another_graph_def));
}


TEST(GraphBuilding, TesthSliceOp) {
	Scope scope(Scope::NewRootScope());

	auto definedBackEdge = ops::Const(scope.WithOpName("backEdge"), Input::Initializer({ 0.f,0.f,0.f,0.f }));
	auto slice = ops::Slice(scope.WithOpName("test"), definedBackEdge, Input::Initializer({ 0 }), Input::Initializer({ 1 }));

	GraphDef another_graph_def;
	TF_CHECK_OK(scope.ToGraphDef(&another_graph_def));
}


TEST(GraphBuilding, TestSliceOp_DefinedBackEdge) {
	Scope scope(Scope::NewRootScope());

	auto definedBackEdge = ops::Const(scope.WithOpName("backEdge"), Input::Initializer({ 0.f,0.f,0.f,0.f }));

	auto backEdge = Input("backEdge", 0, DT_FLOAT);
	auto slice = ops::Slice(scope.WithOpName("test"), backEdge, Input::Initializer({ 0 }), Input::Initializer({ 1 }));

	GraphDef another_graph_def;
	TF_CHECK_OK(scope.ToGraphDef(&another_graph_def));
}
TEST(GraphBuilding, TestSliceOp_UndefinedBackEdge) {
	Scope scope(Scope::NewRootScope());

	auto backEdge = Input("backEdge", 0, DT_FLOAT);
	auto slice = ops::Slice(scope.WithOpName("test"), backEdge, Input::Initializer({ 0 }), Input::Initializer({ 1 }));

	GraphDef another_graph_def;
	TF_CHECK_OK(scope.ToGraphDef(&another_graph_def));
}



TEST(GraphBuilding, TestAddOp_DefinedBackEdge) {
	Scope scope(Scope::NewRootScope());

	auto definedBackEdge = ops::Const(scope.WithOpName("backEdge"), Input::Initializer({ 0.f,0.f,0.f,0.f }));

	auto backEdge = Input("backEdge", 0, DT_FLOAT);
	auto add = ops::Add(scope.WithOpName("test"), backEdge, Input::Initializer({ 0.f,0.f,0.f,0.f }));

	GraphDef another_graph_def;
	TF_CHECK_OK(scope.ToGraphDef(&another_graph_def));
}

TEST(GraphBuilding, TestAppOp_UndefinedBackEdge) {
	Scope scope(Scope::NewRootScope());

	auto backEdge = Input("backEdge", 0, DT_FLOAT);
	auto slice = ops::Add(scope.WithOpName("test"), backEdge, Input::Initializer({ 0.f,0.f,0.f,0.f }));

	GraphDef another_graph_def;
	TF_CHECK_OK(scope.ToGraphDef(&another_graph_def));
}


// Test graph merging with
// https://www.tensorflow.org/tutorials/images/image_recognition
// https://github.com/tensorflow/tensorflow/blob/master/tensorflow/examples/label_image/main.cc
// - Place the inception model in the source dir
// it uses a different interface than what I use here, but 
// casts ops to node objects and adds them to a graphdefbuilder

TEST(GraphBuilding, TestLoadGraph) {
	EXPECT_EQ(1, 1);
}

TEST(GraphBuilding, TestMergeConstructedGraphIntoLoaded) {
	EXPECT_EQ(1, 1);
}

TEST(GraphBuilding, TestMergeLoadedGraphIntoConstructed) {
	EXPECT_EQ(1, 1);
}
