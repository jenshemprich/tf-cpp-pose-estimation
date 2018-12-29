#include "pch.h"

#include <iostream>
#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/public/session.h"
#include "Eigen/Dense"
#include "tensorflow/core/public/session.h"
#include "tensorflow/cc/ops/standard_ops.h"

using namespace std;
using namespace tensorflow;

int main() {
	Session* session;
	Status status = NewSession(SessionOptions(), &session);
	if (!status.ok()) {
		cout << status.ToString() << "\n";
		return 1;
	}
	cout << "Session successfully created.\n";
	cin.get();
	return 0;
}
