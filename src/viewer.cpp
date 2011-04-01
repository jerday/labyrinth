#include "viewer.hpp"
#include <iostream>
#include <GL/gl.h>
#include <GL/glu.h>

Viewer::Viewer()
{
  Glib::RefPtr<Gdk::GL::Config> glconfig;

  glconfig = Gdk::GL::Config::create(Gdk::GL::MODE_RGB |
                                     Gdk::GL::MODE_DEPTH |
                                     Gdk::GL::MODE_DOUBLE);
  if (glconfig == 0) {
    // If we can't get this configuration, die
    std::cerr << "Unable to setup OpenGL Configuration!" << std::endl;
    abort();
  }

  // Accept the configuration
  set_gl_capability(glconfig);
  set_flags(Gtk::CAN_FOCUS);
  // Register the fact that we want to receive these events
  add_events(//Gdk::POINTER_MOTION_MASK |
		    Gdk::BUTTON1_MOTION_MASK    |
             Gdk::BUTTON2_MOTION_MASK    |
            Gdk::BUTTON3_MOTION_MASK    |
             Gdk::BUTTON_PRESS_MASK      |
             Gdk::BUTTON_RELEASE_MASK    |
             Gdk::KEY_PRESS_MASK		 |
             Gdk::KEY_RELEASE_MASK		 |
             Gdk::VISIBILITY_NOTIFY_MASK);



  m_left_click_pressed = false;
  m_middle_click_pressed = false;
  m_right_click_pressed = false;
  m_camera = Point3D(0.0, 0.0, 0.0);
  m_rotate_x = 0;
  m_rotate_y = 0;
  m_tilt_x = 0;
  m_tilt_z = 0;
}

Viewer::~Viewer()
{
  // Nothing to do here right now.
}

void set_cursor(int x, int y)
{
	gdk_display_warp_pointer(gdk_display_get_default(), gdk_screen_get_default(), x, y);
}

bool Viewer::invalidate()
{
  //Force a rerender
  Gtk::Allocation allocation = get_allocation();
  get_window()->invalidate_rect( allocation, false);
	return true;
}
void Viewer::set_mode(Mode m) {
	m_mode = m;

	if(m_mode == BIRDS_EYE) {
		m_camera = Point3D(0.0,-40,0.0);//-m_maze->getWidth()/2.0, -40, m_maze->getHeight()/2.0);
		m_rotate_x = 90;
		m_rotate_y = 0;
		m_mouse_x = 0;
		m_mouse_y = 0;
	} else {
		m_camera = Point3D(m_ball.m_location[0],m_ball.m_location[1],m_ball.m_location[2]);
		m_rotate_x = 90;
		m_rotate_y = 0;
		m_mouse_x = 0;
		m_mouse_y = 0;
	}
	invalidate();
}

void Viewer::on_realize()
{
  // Do some OpenGL setup.
  // First, let the base class do whatever it needs to
  Gtk::GL::DrawingArea::on_realize();

  Glib::RefPtr<Gdk::GL::Drawable> gldrawable = get_gl_drawable();

  if (!gldrawable)
    return;

  if (!gldrawable->gl_begin(get_gl_context()))
    return;

  // Just enable depth testing and set the background colour.
  glEnable(GL_DEPTH_TEST);
  glClearColor(0.7, 0.7, 1.0, 0.0);

  gldrawable->gl_end();
}

bool Viewer::on_expose_event(GdkEventExpose* event)
{
	(void) event;
	Glib::RefPtr<Gdk::GL::Drawable> gldrawable = get_gl_drawable();
	glDrawBuffer(GL_BACK);
	//set_cursor(m_width/2,m_height/2);

	if (!gldrawable->gl_begin(get_gl_context()))
		return false;

	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Modify the current projection matrix so that we move the
	// camera away from the origin.  We'll draw the game at the
	// origin, and we need to back up to see it.
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glEnable(GL_LINE_SMOOTH);
	// Set up lighting
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_LIGHT2);
	glEnable(GL_LIGHT3);

	////float global_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	//glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
	//float lightColor0[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	//glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor0);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	glShadeModel ( GL_SMOOTH );

	if(m_mode == GAME) {
		m_camera = Point3D(m_ball.m_location[0],m_ball.m_location[1],m_ball.m_location[2]);
	}

	// for flying mode, rotate y based on mouse x
	glRotated(m_rotate_x,1.0,0.0,0.0);
	glRotated(m_rotate_y,0.0,1.0,0.0);

	glTranslated(m_camera[0],m_camera[1],m_camera[2]);

	// draw origin
	draw_wall(0.0,5,0.0,1,'w',Colour(0,0,0));
	draw_maze();

	if(m_mode == GAME) {
		m_ball.draw();
	}
	// We pushed a matrix on the PROJECTION stack earlier, we
	// need to pop it.
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	// Swap the contents of the front and back buffers so we see what we
	// just drew. This should only be done if double buffering is enabled.
	gldrawable->swap_buffers();
	gldrawable->gl_end();

	return true;
}

bool Viewer::on_configure_event(GdkEventConfigure* event)
{
	Glib::RefPtr<Gdk::GL::Drawable> gldrawable = get_gl_drawable();
	if (!gldrawable) return false;

	if (!gldrawable->gl_begin(get_gl_context()))
		return false;

	// Set up perspective projection, using current size and aspect
	// ratio of display
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0.0, 0, event->width, event->height);
	gluPerspective(40.0, (GLfloat)event->width/(GLfloat)event->height, 0.1, 1000.0);
	m_width = event->width;
	m_height = event->height;
	// Reset to modelview matrix mode
	glMatrixMode(GL_MODELVIEW);
	gldrawable->gl_end();
	return true;
}

bool Viewer::on_key_press_event(GdkEventKey* event)
{
	double xrotrad, yrotrad;
	switch(event->keyval) {
	case GDK_W:
	case GDK_w:
	case GDK_Up:
		if(m_mode == BIRDS_EYE) {
			yrotrad = (m_rotate_y / 180) * 3.141592654;
			xrotrad = (m_rotate_x / 180) * 3.141592654;
			m_camera[0] -= sin(yrotrad);
			m_camera[1] += sin(xrotrad);
			m_camera[2] += cos(yrotrad)*cos(xrotrad);
		} else if(m_mode == GAME) {
			m_tilt_x += 1;
		}
		break;
	case GDK_S:
	case GDK_s:
	case GDK_Down:
		if(m_mode == BIRDS_EYE) {
			yrotrad = (m_rotate_y / 180) * 3.141592654;
			xrotrad = (m_rotate_x / 180) * 3.141592654;
			m_camera[0] += sin(yrotrad);
			m_camera[1] -= sin(xrotrad);
			m_camera[2] -= cos(yrotrad)*cos(xrotrad);
		} else if(m_mode == GAME) {
			m_tilt_x -= 1;
		}
		break;
	case GDK_A:
	case GDK_a:
	case GDK_Left:
		if(m_mode == BIRDS_EYE) {
			yrotrad = (m_rotate_y / 180) * 3.141592654;
			m_camera[0] += cos(yrotrad);
			m_camera[2] += sin(yrotrad);
		} else if(m_mode == GAME) {

		}
		break;
	case GDK_D:
	case GDK_d:
	case GDK_Right:
		if(m_mode == BIRDS_EYE) {
			yrotrad = (m_rotate_y / 180) * 3.141592654;
			m_camera[0] -= cos(yrotrad);
			m_camera[2] -= sin(yrotrad);
		} else if(m_mode == GAME) {

		}
		break;
	}
	std::cout << "Camera: (" << m_camera[0] << "," << m_camera[1] << "," << m_camera[2] << ")" << std::endl;
	std::cout << "Rotate (" << m_rotate_x << "," << m_rotate_y << ")" << std::endl;

	invalidate();
	return true;
}

bool Viewer::on_button_press_event(GdkEventButton* event)
{
	switch(event->button) {
	case 1:
		m_left_click_pressed = true;
		break;
	case 2:
		m_middle_click_pressed = true;
		break;
	case 3:
		m_right_click_pressed = true;
		break;
	}
	m_mouse_x = event->x;
	m_mouse_y = event->y;
	return true;
}

bool Viewer::on_button_release_event(GdkEventButton* event)
{
	switch(event->button) {
	case 1:
		m_left_click_pressed = false;
		break;
	case 2:
		m_middle_click_pressed = false;
		break;
	case 3:
		m_right_click_pressed = false;
		break;
	}
	m_mouse_x = event->x;
	m_mouse_y = event->y;
	return true;
}

bool Viewer::on_motion_notify_event(GdkEventMotion* event)
{
	// Constraints:
	// m_rotate_x between -90 and 90
	// m_rotate_y between 0 and 360
	m_rotate_y += (event->x-m_mouse_x)/10.0;
	m_rotate_x += (event->y-m_mouse_y)/10.0;

	// Cleanse
	if (m_rotate_x < -90) m_rotate_x = -90;
	else if (m_rotate_x > 90) m_rotate_x = 90;

	if (m_rotate_y < -180) {
		m_rotate_y += 360;
	} else if(m_rotate_y > 180) {
		m_rotate_y -= 360;
	}

	m_mouse_x = event->x;
	m_mouse_y = event->y;
	invalidate();
	return true;
}

void Viewer::draw_floor(const int width, const int height)
{
	glPushMatrix();

	glColor3d(1, 1, 1);
	int tmpW = width/2;
	int tmpH = height/2;
	std::cout << "(" << tmpW << "," << tmpH << ")" << std::endl;

	glTranslated(-width/2,-1,height/2);
    glScaled(width,1,height);

	glBegin(GL_QUADS);

	// top
	glNormal3d(0.0,1.001,0.0);
	glVertex3d(1.001, 1.001,-1.001); // top right
	glVertex3d(0.0, 1.001,-1.001); // top left
	glVertex3d(0.0, 1.001, 0.0); // bottom left
	glVertex3d(1.001, 1.001, 0.0); // bottom right

	// bottom
	glNormal3d(0.0,-1.001,0.0);
	glVertex3d(1.001,0.0,-1.001); // top right
	glVertex3d(0.0,0.0,-1.001); // top left
	glVertex3d(0.0,0.0,0.0); // bottom left
	glVertex3d(1.001,0.0,0.0); // bottom right

	// front
	glNormal3f(0.0,0.0,1.001);
	glVertex3d(1.001,1.001,0.0); // top right
	glVertex3d(0.0,1.001,0.0); // top left
	glVertex3d(0.0,0.0,0.0); // bottom left
	glVertex3d(1.001,0.0,0.0); // bottom right

	// back
	glNormal3f(0.0,0.0,-1.0);
	glVertex3d(0.0,0.0,-1.001); // bottom left
	glVertex3d(1.001,0.0,-1.001); // bottom right
	glVertex3d(1.001,1.001,-1.001); // top right
	glVertex3d(0.0,1.001,-1.001); // top left

	// left
	glNormal3f(-1.0,0.0,0.0);
	glVertex3d(0.0,1.001,-1.001); // top right
	glVertex3d(0.0,1.001,0.0); // top left
	glVertex3d(0.0,0.0,0.0); // bottom left
	glVertex3d(0.0,0.0,-1.001); // bottom right

	// right
	glNormal3f(1.0,0.0,0.0);
	glVertex3d(1.001,1.001,0.0); // top right
	glVertex3d(1.001,1.001,-1.001); // top left
	glVertex3d(1.001,0.0,-1.001); // bottom left
	glVertex3d(1.001,0.0,0.0); // bottom right

    glEnd();

    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	glPopMatrix();
}

void Viewer::draw_wall(double x, double y, double z, int length, char dir, Colour c)
{
	glPushMatrix();

	glColor3d(c.R(),c.G(),c.B());
	glTranslated(x,y,z);
	if(dir == 'x') {
		glScaled(length,5.0,1.0);
	}

	if(dir == 'z') {
		glScaled(1.0,5.0,length);
	}

	glBegin(GL_QUADS);

	// top
	glNormal3d(0.0,1.001,0.0);
	glVertex3d(1.001, 1.001,-1.001); // top right
	glVertex3d(0.0, 1.001,-1.001); // top left
	glVertex3d(0.0, 1.001, 0.0); // bottom left
	glVertex3d(1.001, 1.001, 0.0); // bottom right

	// bottom
	glNormal3d(0.0,-1.001,0.0);
	glVertex3d(1.001,0.0,-1.001); // top right
	glVertex3d(0.0,0.0,-1.001); // top left
	glVertex3d(0.0,0.0,0.0); // bottom left
	glVertex3d(1.001,0.0,0.0); // bottom right

	// front
	glNormal3f(0.0,0.0,1.001);
	glVertex3d(1.001,1.001,0.0); // top right
	glVertex3d(0.0,1.001,0.0); // top left
	glVertex3d(0.0,0.0,0.0); // bottom left
	glVertex3d(1.001,0.0,0.0); // bottom right

	// back
	glNormal3f(0.0,0.0,-1.0);
	glVertex3d(0.0,0.0,-1.001); // bottom left
	glVertex3d(1.001,0.0,-1.001); // bottom right
	glVertex3d(1.001,1.001,-1.001); // top right
	glVertex3d(0.0,1.001,-1.001); // top left

	// left
	glNormal3f(-1.0,0.0,0.0);
	glVertex3d(0.0,1.001,-1.001); // top right
	glVertex3d(0.0,1.001,0.0); // top left
	glVertex3d(0.0,0.0,0.0); // bottom left
	glVertex3d(0.0,0.0,-1.001); // bottom right

	// right
	glNormal3f(1.0,0.0,0.0);
	glVertex3d(1.001,1.001,0.0); // top right
	glVertex3d(1.001,1.001,-1.001); // top left
	glVertex3d(1.001,0.0,-1.001); // bottom left
	glVertex3d(1.001,0.0,0.0); // bottom right

    glEnd();

    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	glPopMatrix();
}

void Viewer::set_maze(Maze * maze) {
	m_maze = maze;
}

void Viewer::draw_maze()
{
	glPushMatrix();
	glRotated(m_tilt_x,1.0,0.0,0.0);
	int width = m_maze->getWidth();
	int height = m_maze->getHeight();
	// floor
	draw_floor(width+2,height+2);

	// outside walls
	draw_wall(-width/2-1, 0,  height/2+1, width+2,'x',Colour(0,0,1));
	draw_wall(-width/2-1, 0,  height/2+1, height+2,'z',Colour(0,0,1));
	draw_wall(-width/2-1, 0, -height/2-1, width+2,'x',Colour(0,0,1));
	draw_wall( width/2+1, 0,  height/2+1, height+2,'z',Colour(0,0,1));

	for(int x = 0; x < m_maze->getWidth(); x++) {
		for(int y = 0; y < m_maze->getHeight(); y++) {
			char id = (*m_maze)(x,y);
			if(id == 'w') {
				draw_wall(-width/2 + x,0,-height/2 + y,1,'x', Colour(1,0,0));
			}
			if(id == 's') {
				m_ball = Ball((int)-width/2 + x,(int)-height/2 + y,1.0,0.5);
			}
		}
	}
	glPopMatrix();
}
