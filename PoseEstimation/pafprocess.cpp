// Copied from https://github.com/ildoonet/tf-pose-estimation/tree/master/tf_pose/pafprocess/
// In-depth article about the algorithm: https://arvrjourney.com/human-pose-estimation-using-openpose-with-tensorflow-part-2-e78ab9104fc8
// Refactored to reduce complexity
// changed to process peak coordinates from tensorflow::ops::Where instead of peaks map from python tf.Where

#include "pch.h"

#include <basetsd.h>

#include <iostream>
#include <algorithm>
#include <math.h>

#include "pafprocess.h"

const float PafProcess::THRESH_HEAT = 0.05;
const float PafProcess::THRESH_VECTOR_SCORE = 0.05;
const float PafProcess::THRESH_HUMAN_SCORE = 0.4;

const int PafProcess::COCOPAIRS_NET[COCOPAIRS_SIZE][2] = {
	{12, 13}, {20, 21}, {14, 15}, {16, 17}, {22, 23}, {24, 25}, {0, 1}, {2, 3}, {4, 5},
	{6, 7}, {8, 9}, {10, 11}, {28, 29}, {30, 31}, {34, 35}, {32, 33}, {36, 37}, {18, 19}, {26, 27}
};

const int PafProcess::COCOPAIRS[COCOPAIRS_SIZE][2] = {
	{1, 2}, {1, 5}, {2, 3}, {3, 4}, {5, 6}, {6, 7}, {1, 8}, {8, 9}, {9, 10}, {1, 11},
	{11, 12}, {12, 13}, {1, 0}, {0, 14}, {14, 16}, {0, 15}, {15, 17}, {2, 16}, {5, 17}
};

const int PafProcess::CocoColors[COCOPAIRS_SIZE][3] = { {255, 0, 0}, {255, 85, 0}, {255, 170, 0}, {255, 255, 0}, {170, 255, 0}, {85, 255, 0}, {0, 255, 0},
{0, 255, 85}, {0, 255, 170}, {0, 255, 255}, {0, 170, 255}, {0, 85, 255}, {0, 0, 255}, {85, 0, 255},
{170, 0, 255}, {255, 0, 255}, {255, 0, 170}, {255, 0, 85} };


#define PEAKS(i, j, k) peaks[k+p3*(j+p2*i)]
#define HEAT(i, j, k) heatmap[k+h3*(j+h2*i)]
#define PAF(i, j, k) pafmap[k+f3*(j+f2*i)]
#define COORDS(i, j) coords[j+4*i]

using namespace std;

int PafProcess::process(const int c1, const INT64 * coords, const int p1, const int p2, const int p3, const float *peaks, const int h1, const int h2, const int h3, const float *heatmap, const int f1, const int f2, const int f3, const float *pafmap) {
	vector<Peak> peak_infos[NUM_PART];
	gather_peak_infos(c1, coords, p1, p2, p3, peaks, h2, h3, heatmap, peak_infos);
	gather_peak_infos_line(peak_infos);

	// Start to Connect
	vector<Connection> connection_all[COCOPAIRS_SIZE];
	connect_all(peak_infos, h1, f2, f3, pafmap, connection_all);

	// Generate subset
	subset.clear();
	connect_subset(connection_all);

	// delete some rows
	delete_some_rows();

	return 0;
}

void PafProcess::gather_peak_infos(const int c1, const INT64* coords, const int p1, const int p2, const int p3, const float* peaks, const int h2, const int h3, const float* heatmap, std::vector<Peak> peak_infos[18]) {
	int peak_cnt = 0;
	for (int c = 0; c < c1; c++) {
		const int x = COORDS(c, 2);
		const int y = COORDS(c, 1);
		const int part_id = COORDS(c, 3);
		// Ignore background layer (channel 18)
		const bool is_part_layer = part_id < NUM_PART;
		if (is_part_layer && PEAKS(y, x, part_id) > THRESH_HEAT) {
			Peak info;
			info.id = peak_cnt++;
			info.x = x;
			info.y = y;
			info.score = HEAT(y, x, part_id);
			peak_infos[part_id].push_back(info);
		}
	}
}

void PafProcess::gather_peak_infos_line(const std::vector<Peak> peak_infos[18]) {
	peak_infos_line.clear();
	for (int part_id = 0; part_id < NUM_PART; part_id++) {
		for (int i = 0; i < (int)peak_infos[part_id].size(); i++) {
			peak_infos_line.push_back(peak_infos[part_id][i]);
		}
	}
	sort(peak_infos_line.begin(), peak_infos_line.end(), comp_peak_id_ascending);
}

void PafProcess::connect_all(const std::vector<Peak> peak_infos[18], const int h1, const int f2, const int f3, const float* pafmap, std::vector<Connection> connection_all[19]) {
	for (int pair_id = 0; pair_id < COCOPAIRS_SIZE; pair_id++) {
		vector<ConnectionCandidate> candidates;
		const vector<Peak>& peak_a_list = peak_infos[COCOPAIRS[pair_id][0]];
		const vector<Peak>& peak_b_list = peak_infos[COCOPAIRS[pair_id][1]];

		if (peak_a_list.size() == 0 || peak_b_list.size() == 0) {
			continue;
		}

		for (int peak_a_id = 0; peak_a_id < (int)peak_a_list.size(); peak_a_id++) {
			const Peak& peak_a = peak_a_list[peak_a_id];
			for (int peak_b_id = 0; peak_b_id < (int)peak_b_list.size(); peak_b_id++) {
				const Peak& peak_b = peak_b_list[peak_b_id];

				// calculate vector(direction)
				VectorXY vec;
				vec.x = peak_b.x - peak_a.x;
				vec.y = peak_b.y - peak_a.y;
				const float norm = (float)sqrt(vec.x * vec.x + vec.y * vec.y);
				if (norm < 1e-12) continue;
				vec.x = vec.x / norm;
				vec.y = vec.y / norm;

				const vector<VectorXY> paf_vecs = get_paf_vectors(pafmap, COCOPAIRS_NET[pair_id][0], COCOPAIRS_NET[pair_id][1], f2, f3, peak_a, peak_b);
				float scores = 0.0f;

				// criterion 1 : score treshold count
				int criterion1 = 0;
				for (int i = 0; i < STEP_PAF; i++) {
					const float score = vec.x * paf_vecs[i].x + vec.y * paf_vecs[i].y;
					scores += score;

					if (score > THRESH_VECTOR_SCORE) criterion1 += 1;
				}

				const float criterion2 = scores / STEP_PAF + min(0.0, 0.5 * h1 / norm - 1.0);

				if (criterion1 > THRESH_VECTOR_CNT1 && criterion2 > 0) {
					ConnectionCandidate candidate;
					candidate.idx1 = peak_a_id;
					candidate.idx2 = peak_b_id;
					candidate.score = criterion2;
					candidate.etc = criterion2 + peak_a.score + peak_b.score;
					candidates.push_back(candidate);
				}
			}
		}

		vector<Connection>& conns = connection_all[pair_id];
		sort(candidates.begin(), candidates.end(), comp_candidate_score_descending);
		for (int c_id = 0; c_id < (int)candidates.size(); c_id++) {
			const ConnectionCandidate& candidate = candidates[c_id];
			bool assigned = false;
			for (int conn_id = 0; conn_id < (int)conns.size(); conn_id++) {
				if (conns[conn_id].peak_id1 == candidate.idx1) {
					// already assigned
					assigned = true;
					break;
				}
				if (assigned) break;
				if (conns[conn_id].peak_id2 == candidate.idx2) {
					// already assigned
					assigned = true;
					break;
				}
				if (assigned) break;
			}
			if (assigned) continue;

			Connection conn;
			conn.peak_id1 = candidate.idx1;
			conn.peak_id2 = candidate.idx2;
			conn.score = candidate.score;
			conn.cid1 = peak_a_list[candidate.idx1].id;
			conn.cid2 = peak_b_list[candidate.idx2].id;
			conns.push_back(conn);
		}
	}
}

void PafProcess::connect_subset(const std::vector<Connection> connection_all[19]) {
	for (int pair_id = 0; pair_id < COCOPAIRS_SIZE; pair_id++) {
		const vector<Connection>& conns = connection_all[pair_id];
		const int part_id1 = COCOPAIRS[pair_id][0];
		const int part_id2 = COCOPAIRS[pair_id][1];

		for (int conn_id = 0; conn_id < (int)conns.size(); conn_id++) {
			int found = 0;
			int subset_idx1 = 0, subset_idx2 = 0;
			for (int subset_id = 0; subset_id < (int)subset.size(); subset_id++) {
				if (subset[subset_id][part_id1] == conns[conn_id].cid1 || subset[subset_id][part_id2] == conns[conn_id].cid2) {
					if (found == 0) subset_idx1 = subset_id;
					if (found == 1) subset_idx2 = subset_id;
					found += 1;
				}
			}

			if (found == 1) {
				if (subset[subset_idx1][part_id2] != conns[conn_id].cid2) {
					subset[subset_idx1][part_id2] = conns[conn_id].cid2;
					subset[subset_idx1][19] += 1;
					subset[subset_idx1][18] += peak_infos_line[conns[conn_id].cid2].score + conns[conn_id].score;
				}
			} else if (found == 2) {
				int membership = 0;
				for (int subset_id = 0; subset_id < 18; subset_id++) {
					if (subset[subset_idx1][subset_id] > 0 && subset[subset_idx2][subset_id] > 0) {
						membership = 2;
					}
				}

				if (membership == 0) {
					for (int subset_id = 0; subset_id < 18; subset_id++) subset[subset_idx1][subset_id] += (subset[subset_idx2][subset_id] + 1);

					subset[subset_idx1][19] += subset[subset_idx2][19];
					subset[subset_idx1][18] += subset[subset_idx2][18];
					subset[subset_idx1][18] += conns[conn_id].score;
					subset.erase(subset.begin() + subset_idx2);
				} else {
					subset[subset_idx1][part_id2] = conns[conn_id].cid2;
					subset[subset_idx1][19] += 1;
					subset[subset_idx1][18] += peak_infos_line[conns[conn_id].cid2].score + conns[conn_id].score;
				}
			} else if (found == 0 && pair_id < 17) {
				vector<float> row(20);
				for (int i = 0; i < 20; i++) {
					row[i] = -1;
				}
				row[part_id1] = conns[conn_id].cid1;
				row[part_id2] = conns[conn_id].cid2;
				row[19] = 2;
				row[18] = peak_infos_line[conns[conn_id].cid1].score +
					peak_infos_line[conns[conn_id].cid2].score +
					conns[conn_id].score;
				subset.push_back(row);
			}
		}
	}
}

void PafProcess::delete_some_rows() {
	for (int i = subset.size() - 1; i >= 0; i--) {
		if (subset[i][19] < THRESH_PART_CNT || subset[i][18] / subset[i][19] < THRESH_HUMAN_SCORE)
			subset.erase(subset.begin() + i);
	}
}

int PafProcess::get_num_humans() {
    return subset.size();
}

int PafProcess::get_part_cid(int human_id, int part_id) {
    return subset[human_id][part_id];
}

float PafProcess::get_score(int human_id) {
    return subset[human_id][18] / subset[human_id][19];
}

int PafProcess::get_part_x(int cid) {
    return peak_infos_line[cid].x;
}
int PafProcess::get_part_y(int cid) {
    return peak_infos_line[cid].y;
}
float PafProcess::get_part_score(int cid) {
    return peak_infos_line[cid].score;
}

vector<PafProcess::VectorXY> PafProcess::get_paf_vectors(const float *pafmap, const int ch_id1, const int ch_id2, const int f2, const int f3, const Peak& peak1, const Peak& peak2) {
    vector<PafProcess::VectorXY> paf_vectors;

    const float STEP_X = (peak2.x - peak1.x) / float(STEP_PAF);
    const float STEP_Y = (peak2.y - peak1.y) / float(STEP_PAF);

    for (int i = 0; i < STEP_PAF; i ++) {
        int location_x = roundpaf(peak1.x + i * STEP_X);
        int location_y = roundpaf(peak1.y + i * STEP_Y);

		PafProcess::VectorXY v;
        v.x = PAF(location_y, location_x, ch_id1);
        v.y = PAF(location_y, location_x, ch_id2);
        paf_vectors.push_back(v);
    }

    return paf_vectors;
}

int PafProcess::roundpaf(const float v) {
    return (int) (v + 0.5);
}

bool PafProcess::comp_candidate_score_descending(const PafProcess::ConnectionCandidate& a, const PafProcess::ConnectionCandidate& b) {
    return a.score > b.score;
}

bool PafProcess::comp_peak_id_ascending(const PafProcess::Peak& a, const PafProcess::Peak& b) {
	return a.id < b.id;
}
