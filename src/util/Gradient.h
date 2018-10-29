#pragma once

#include <vector>
#include <glm/glm.hpp>

struct Gradient_ControlPoint {
	float position;
	glm::vec4 color;
	
	Gradient_ControlPoint() : position(0.5f), color(glm::vec4(1, 1, 1, 1)) {

	}

	Gradient_ControlPoint(float pos, glm::vec4 color) : position(pos), color(color) {

	}
};


class Gradient
{
public:
	Gradient();
	~Gradient();

	bool isBlended;

	void AddControlPoint(float pos, glm::vec4 color);

	glm::vec4 SampleGradient(float pos);

	std::vector<Gradient_ControlPoint> controlPoints;

	void SetFrontColor(glm::vec4 color);
	void SetBackColor(glm::vec4 color);
private:
	glm::vec4 LinearInterpolate(float val, Gradient_ControlPoint start, Gradient_ControlPoint end);
};

