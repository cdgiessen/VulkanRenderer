#pragma once

#include <cml/cml.h>
#include <vector>

struct Gradient_ControlPoint
{
	float position;
	cml::vec4f color;

	Gradient_ControlPoint () : position (0.5f), color (cml::vec4f (1, 1, 1, 1)) {}

	Gradient_ControlPoint (float pos, cml::vec4f color) : position (pos), color (color) {}
};


class Gradient
{
	public:
	Gradient ();
	~Gradient ();

	bool isBlended;

	void AddControlPoint (float pos, cml::vec4f color);

	cml::vec4f SampleGradient (float pos);

	std::vector<Gradient_ControlPoint> controlPoints;

	void SetFrontColor (cml::vec4f color);
	void SetBackColor (cml::vec4f color);

	private:
	cml::vec4f LinearInterpolate (float val, Gradient_ControlPoint start, Gradient_ControlPoint end);
};
