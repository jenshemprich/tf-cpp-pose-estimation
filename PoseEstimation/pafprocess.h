#pragma once

#include <vector>
#include <basetsd.h>


class PafProcess {
	static const float THRESH_HEAT;
	static const float THRESH_VECTOR_SCORE;
	static const float THRESH_HUMAN_SCORE;

	static const int THRESH_VECTOR_CNT1 = 8;
	static const int THRESH_PART_CNT = 4;

	static const int NUM_PART = 18;
	static const int STEP_PAF = 10;

public:
	static const int COCOPAIRS_SIZE = 19;
	static const int COCOPAIRS_NET[COCOPAIRS_SIZE][2];
	static const int COCOPAIRS[COCOPAIRS_SIZE][2];
	static const int CocoColors[COCOPAIRS_SIZE][3];


	struct Peak {
		int x;
		int y;
		float score;
		int id;
	};

	struct VectorXY {
		float x;
		float y;
	};

	struct ConnectionCandidate {
		int idx1;
		int idx2;
		float score;
		float etc;
	};

	struct Connection {
		int cid1;
		int cid2;
		float score;
		int peak_id1;
		int peak_id2;
	};

	int process(const int c1, const INT64* coords, const int p1, const int p2, const int p3, const float* peaks, const int h1, const int h2, const int h3, const float* heatmap, const int f1, const int f2, const int f3, const float* pafmap);

private:
	void gather_peak_infos(const int c, const INT64* coords, const int p1, const int p2, const int p3, const float* peaks, const  int h2, const int h3, const float* heatmap, std::vector<Peak> peak_infos[18]);
	void gather_peak_infos_line(const std::vector<Peak> peak_infos[18]);
	void connect_all(const std::vector<Peak> peak_infos[18], const int h1, const int f2, const int f3, const float* pafmap, std::vector<Connection> connection_all[19]);
	void connect_subset(const std::vector<Connection> connection_all[19]);
	void delete_some_rows();

	std::vector<PafProcess::VectorXY> get_paf_vectors(const float* pafmap, const int ch_id1, const int ch_id2, const int f2, const int f3, const Peak& peak1, const Peak& peak2);
	static int roundpaf(float v);
	static bool comp_candidate_score_descending(const PafProcess::ConnectionCandidate& a, const PafProcess::ConnectionCandidate& b);
	static bool comp_peak_id_ascending(const PafProcess::Peak& a, const PafProcess::Peak& b);


	std::vector<std::vector<float>> subset;
	std::vector<PafProcess::Peak> peak_infos_line;

public:
	int get_num_humans();
	int get_part_cid(int human_id, int part_id);
	float get_score(int human_id);
	int get_part_x(int cid);
	int get_part_y(int cid);
	float get_part_score(int cid);
};
