#pragma once

namespace coco {

	struct BodyPart {
		BodyPart(int part_index, float x, float y, float score);

		int part_index;
		float x;
		float y;
		float score;
	};

	struct Human {
		typedef std::map<int, BodyPart> BodyParts;
		Human() : score(0.0) {};
		Human(const BodyParts& parts, const float score);

		const BodyParts parts;
		const float score;
	};

	class Renderer {
	public:
		virtual void joint(float x, float y, const int* rgb) = 0;
		virtual void segment(float joint1_x, float joint1_y, float joint2_x, float joint2_y, const int* rgb) = 0;

		void draw(const std::vector<Human>& humans);

	};

}
