#ifndef BALL_HPP
#define BALL_HPP

#include "algebra.hpp"

class Ball {
public:
	Ball();
	Ball(double x, double y, double z, double radius);
	~Ball();
	void draw();
	Point3D m_location;
	double m_radius;
	Vector3D m_velocity;
	Point3D m_angle;
};

#endif
