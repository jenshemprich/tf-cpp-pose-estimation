#include <vector>

#ifndef PAFPROCESS
#define PAFPROCESS

const float THRESH_HEAT = 0.05;
const float THRESH_VECTOR_SCORE = 0.05;
const int THRESH_VECTOR_CNT1 = 8;
const int THRESH_PART_CNT = 4;
const float THRESH_HUMAN_SCORE = 0.4;

const int NUM_PART = 18;
//const int NUM_PART = 19;  // TODO Why 18?

const int STEP_PAF = 10;

const int COCOPAIRS_SIZE = 19;
const int COCOPAIRS_NET[COCOPAIRS_SIZE][2] = {
    {12, 13}, {20, 21}, {14, 15}, {16, 17}, {22, 23}, {24, 25}, {0, 1}, {2, 3}, {4, 5},
    {6, 7}, {8, 9}, {10, 11}, {28, 29}, {30, 31}, {34, 35}, {32, 33}, {36, 37}, {18, 19}, {26, 27}
};

const int COCOPAIRS[COCOPAIRS_SIZE][2] = {
    {1, 2}, {1, 5}, {2, 3}, {3, 4}, {5, 6}, {6, 7}, {1, 8}, {8, 9}, {9, 10}, {1, 11},
    {11, 12}, {12, 13}, {1, 0}, {0, 14}, {14, 16}, {0, 15}, {15, 17}, {2, 16}, {5, 17}
};

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

// TODO refactor to C++ class to resolve globals instances

int process_paf(const int c1, const INT64 * coords, const int p1, const int p2, const int p3, const float *peaks, const int h1, const int h2, const int h3, const float *heatmap, const int f1, const int f2, const int f3, const float *pafmap);
void gather_peak_infos(const int c, const INT64 * coords, const int p1, const int p2, const int p3, const float * peaks, const  int h2, const int h3, const float * heatmap, std::vector<Peak> peak_infos[NUM_PART]);
void gather_peak_infos_line(const std::vector<Peak> * peak_infos);
void connect_all(const std::vector<Peak> peak_infos[NUM_PART], const int h1, const int f2, const int f3, const float * pafmap, std::vector<Connection> connection_all[19]);
void connect_subset(const std::vector<Connection> connection_all[19]);
void delete_some_rows();

int get_num_humans();
int get_part_cid(int human_id, int part_id);
float get_score(int human_id);
int get_part_x(int cid);
int get_part_y(int cid);
float get_part_score(int cid);


#endif
