#include "pch.h"

#include "algorithm"
#include "map"

#include "pafprocess.h"

#include "CocoDataModel.h"

using namespace std;

namespace coco {
	void Renderer::draw(const vector<Human>& humans) {
		for_each(humans.begin(), humans.end(), [this](const Human& human) {
			struct Point { float x; float y; };
			vector<Point> centers(PafProcess::COCOPAIRS_SIZE);

			for (int i = 0; i < PafProcess::COCOPAIRS_SIZE; ++i) {
				auto part = human.parts.find(i);
				if (part != human.parts.end()) {
					const BodyPart& body_part = part->second;
					centers[i] = { body_part.x, body_part.y };
					joint(body_part.x, body_part.y, PafProcess::CocoColors[i]);
				}
			}

			// CocoPairsRender = CocoPairs[:-2] -> coco pairs but the last two elements
			for (int pair_order = 0; pair_order < PafProcess::COCOPAIRS_SIZE - 2; ++pair_order) {
				const int* pair = PafProcess::COCOPAIRS[pair_order];

				if (human.parts.find(pair[0]) != human.parts.end() && human.parts.find(pair[1]) != human.parts.end()) {
					const int* coco_color = PafProcess::CocoColors[pair_order];
					segment(
						centers[pair[0]].x, centers[pair[0]].y,
						centers[pair[1]].x, centers[pair[1]].y,
						coco_color);
				}
			}
			});
	}
}