#include <GL/gl.h>
#include <GL/glu.h>
#include "ball.hpp"

Ball::Ball() { }
Ball::~Ball() { }
Ball::Ball(double x, double y, double z, double radius) {
	m_location = Point3D(x,y,z);
	m_radius = radius;
	m_velocity = Point3D(0.0,0.0,0.0);
	m_angle = Point3D(0.0,0.0,0.0);
}

void Ball::draw() {
	glPushMatrix();
    glTranslated(-m_location[0],m_location[1],-m_location[2]);
    glRotated(m_angle[0],1.0,0.0,0.0);
    glRotated(m_angle[2],0.0,0.0,1.0);

	glColor3d(0.5, 0.5, 0.5);
	GLUquadric* quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);
    gluQuadricDrawStyle(quad, GLU_FILL);
	gluQuadricTexture(quad, GL_TRUE);
    gluSphere(quad, m_radius, 32, 32);
    glPopMatrix();
}

