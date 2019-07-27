#include "Gradient.h"
#include "cml/cml.h"


Gradient::Gradient ()
{
	controlPoints.push_back (Gradient_ControlPoint (0.0f, cml::vec4f (0, 0, 0, 1)));
	controlPoints.push_back (Gradient_ControlPoint (1.0f, cml::vec4f (1, 1, 1, 1)));
}


Gradient::~Gradient () {}

void Gradient::SetFrontColor (cml::vec4f color)
{
	if (controlPoints.at (0).position == 0)
		controlPoints.at (0).color = color;
	else
		controlPoints.insert (controlPoints.begin (), Gradient_ControlPoint (0, color));
}

void Gradient::SetBackColor (cml::vec4f color)
{
	if ((controlPoints.end () - 1)->position == 1)
		(controlPoints.end () - 1)->color = color;
	else
		controlPoints.push_back (Gradient_ControlPoint (1, color));
}

void Gradient::AddControlPoint (float pos, cml::vec4f color)
{
	Gradient_ControlPoint gcp = Gradient_ControlPoint (pos, color);

	auto it = controlPoints.begin ();

	while (it != controlPoints.end ())
	{
		if (it->position > gcp.position) break;
		it++;
	}

	controlPoints.insert (it, gcp);
}

cml::vec4f Gradient::SampleGradient (float pos)
{
	auto it = controlPoints.begin ();
	it++;

	while (it->position < pos && it + 1 != controlPoints.end ())
	{
		it++;
	}

	return LinearInterpolate (pos, *(it - 1), *it);
}

cml::vec4f Gradient::LinearInterpolate (float pos, Gradient_ControlPoint start, Gradient_ControlPoint end)
{
	float range = end.position - start.position;
	float endPos = end.position - pos;
	float posStart = pos - start.position;

	return cml::vec4f ((start.color.x * endPos + end.color.x * posStart) / range,
	    (start.color.y * endPos + end.color.y * posStart) / range,
	    (start.color.z * endPos + end.color.z * posStart) / range,
	    (start.color.w * endPos + end.color.w * posStart) / range);
}