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
  m_ball_set = false;
  ball_radius = 0.4;
  m_skybox = ALPINE;
}

Viewer::~Viewer()
{
  // Nothing to do here right now.
}

/* Image type - contains height, width, and data */
struct BMPImage {
	unsigned long sizeX;
	unsigned long sizeY;
	char *data;
};
typedef struct BMPImage BMPImage;

int ImageLoad(std::string filename, BMPImage *image)
{
    FILE *file;
    unsigned long size;
    unsigned long i;
    unsigned short int planes;
    unsigned short int bpp;
    char temp;

    if ((file = fopen(filename.c_str(), "rb"))==NULL)
    {
		printf("File Not Found : %s\n",filename.c_str());
		return 0;
    }

    fseek(file, 18, SEEK_CUR);

    if ((i = fread(&image->sizeX, 4, 1, file)) != 1) {
		printf("Error reading width from %s.\n", filename.c_str());
		return 0;
    }

    if ((i = fread(&image->sizeY, 4, 1, file)) != 1) {
		printf("Error reading height from %s.\n", filename.c_str());
		return 0;
    }

    size = image->sizeX * image->sizeY * 3;

    if ((fread(&planes, 2, 1, file)) != 1) {
    	printf("Error reading planes from %s.\n", filename.c_str());
    	return 0;
    }
    if (planes != 1) {
    	printf("Planes from %s is not 1: %u\n", filename.c_str(), planes);
    	return 0;
    }

    if ((i = fread(&bpp, 2, 1, file)) != 1) {
    	printf("Error reading bpp from %s.\n", filename.c_str());
    	return 0;
    }
    if (bpp != 24) {
    	printf("Bpp from %s is not 24: %u\n", filename.c_str(), bpp);
    	return 0;
    }

    fseek(file, 24, SEEK_CUR);

    image->data = (char *) malloc(size);
    if (image->data == NULL) {
    	printf("Error allocating memory for color-corrected image data");
    	return 0;
    }

    if ((i = fread(image->data, size, 1, file)) != 1) {
    	printf("Error reading image data from %s.\n", filename.c_str());
    	return 0;
    }

    for (i=0;i<size;i+=3) { // reverse all of the colors. (bgr -> rgb)
		temp = image->data[i];
		image->data[i] = image->data[i+2];
		image->data[i+2] = temp;
    }

    return 1;
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
		m_camera = Point3D(0.0,40,0.0);//-m_maze->getWidth()/2.0, -40, m_maze->getHeight()/2.0);
		m_rotate_x = 90;
		m_rotate_y = 0;
		m_mouse_x = 0;
		m_mouse_y = 0;
	} else {
		m_camera = Point3D(m_ball.m_location[0],m_ball.m_location[1],m_ball.m_location[2]);
		m_rotate_x = 60;
		m_rotate_y = 0;
		m_mouse_x = 0;
		m_mouse_y = 0;
	}
	invalidate();
}

void Viewer::set_skybox(Skybox s) {
	m_skybox = s;
	configure_skybox();
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
	draw_all();

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
	configure_skybox();
	configure_textures();

	m_quadratic=gluNewQuadric();
	  m_conn.disconnect();
	  m_conn = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Viewer::do_physics), time_refresh);

	gluQuadricNormals(m_quadratic, GLU_SMOOTH);   // Create Smooth Normals
	gluQuadricTexture(m_quadratic, GL_TRUE);      // Create Texture Coords ( NEW )
	return true;


}

void Viewer::configure_skybox() {
	// Load Texture Map
	std::string map;
	if(m_skybox == Viewer::ALPINE) {
		map = "alpine";
	} else if (m_skybox == Viewer::CALM) {
		map = "calm";
	} else if (m_skybox == Viewer::CITY) {
		map = "city";
	} else if (m_skybox == Viewer::DESERT_EVENING) {
		map = "desert_evening";
	} else if (m_skybox == Viewer::HOURGLASS) {
		map = "hourglass";
	} else if (m_skybox == Viewer::ENTROPIC) {
		map = "entropic";
	} else if (m_skybox == Viewer::NIGHT_SKY) {
		map = "nightsky";
	} else if (m_skybox == Viewer::ISLANDS) {
		map = "islands";
	} else if (m_skybox == Viewer::RED) {
		map = "red";
	} else if (m_skybox == Viewer::LOST_VALLEY) {
		map = "lostvalley";
	}
	BMPImage *image[6];

	std::string filenames[6];
	filenames[0] = "../data/" + map + "_south.bmp";
	filenames[1] = "../data/" + map + "_east.bmp";
	filenames[2] = "../data/" + map + "_north.bmp";
	filenames[3] = "../data/" + map + "_up.bmp";
	filenames[4] = "../data/" + map + "_down.bmp";
	filenames[5] = "../data/" + map + "_west.bmp";

	for(int i = 0; i < 6; i++) {
		image[i] = (BMPImage *) malloc(sizeof(BMPImage));
		if (image[i] == NULL) {
			printf("Error allocating space for image");
			exit(0);
		}
		if (!ImageLoad(filenames[i], image[i])) {
			std::cerr << "Error opening file: " << filenames[i] << std::endl;
			exit(1);
		} else {
			// Create Texture
			glGenTextures(1, &m_texture[i]);
			glBindTexture(GL_TEXTURE_2D, m_texture[i]);

			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

			glTexImage2D(GL_TEXTURE_2D, 0, 3, image[i]->sizeX, image[i]->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image[i]->data);
		}
	}
}

void Viewer::configure_textures() {
	char* filename = "../data/tga/brickwall.tga";
	TGAImg * img = new TGAImg();
	img->Load(filename);
	glGenTextures(1, &m_wall_textures[0]);
	glBindTexture(GL_TEXTURE_2D, m_wall_textures[0]);   // 2d texture (x and y size)
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3,img->GetWidth(), img->GetHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, img->GetImg());

	filename = "../data/tga/floortiles.tga";
	img = new TGAImg();
	img->Load(filename);
	glGenTextures(1, &m_wall_textures[1]);
	glBindTexture(GL_TEXTURE_2D, m_wall_textures[1]);   // 2d texture (x and y size)
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3,img->GetWidth(), img->GetHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, img->GetImg());

	filename = "../data/tga/base_plaster.tga";
	img = new TGAImg();
	img->Load(filename);
	glGenTextures(1, &m_wall_textures[2]);
	glBindTexture(GL_TEXTURE_2D, m_wall_textures[2]);   // 2d texture (x and y size)
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3,img->GetWidth(), img->GetHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, img->GetImg());

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
			m_camera[1] -= sin(xrotrad);
			m_camera[2] += cos(yrotrad)*cos(xrotrad);
		} else if(m_mode == GAME) {
			m_tilt_x -= 0.5;
		}
		break;
	case GDK_S:
	case GDK_s:
	case GDK_Down:
		if(m_mode == BIRDS_EYE) {
			yrotrad = (m_rotate_y / 180) * 3.141592654;
			xrotrad = (m_rotate_x / 180) * 3.141592654;
			m_camera[0] += sin(yrotrad);
			m_camera[1] += sin(xrotrad);
			m_camera[2] -= cos(yrotrad)*cos(xrotrad);
		} else if(m_mode == GAME) {
			m_tilt_x += 0.5;
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
			m_tilt_z += 0.5;
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
			m_tilt_z -= 0.5;
		}
		break;
	}
	std::cout << "Camera: (" << m_camera[0] << "," << m_camera[1] << "," << m_camera[2] << ")" << std::endl;
	std::cout << "Rotate (" << m_rotate_x << "," << m_rotate_y << ")" << std::endl;
	if(m_camera[0] < -90) {
		m_camera[0] = -90;
	}
	if(m_camera[0] > 90) {
		m_camera[0] = 90;
	}
	if(m_camera[1] < -90) {
		m_camera[1] = -90;
	}
	if(m_camera[1] > 90) {
		m_camera[1] = 90;
	}
	if(m_camera[2] < -90) {
		m_camera[2] = -90;
	}
	if(m_camera[2] > 90) {
		m_camera[2] = 90;
	}
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

	glTranslated(-width/2,-1,height/2);
    glScaled(width,1,height);
    int tilesW = width / 2;
    int tilesH = height /2 ;
	glBindTexture(GL_TEXTURE_2D, m_wall_textures[1]);
	glBegin(GL_QUADS);
	// top
	glNormal3d(0.0,1.001,0.0);
		glTexCoord2f(tilesW,tilesH);
		glVertex3d(1.001, 1.001,-1.001); // top right
		glTexCoord2f(0, tilesH);
		glVertex3d(0.0, 1.001,-1.001); // top left
		glTexCoord2f(0, 0);
		glVertex3d(0.0, 1.001, 0.0); // bottom left
		glTexCoord2f(tilesW, 0);
		glVertex3d(1.001, 1.001, 0.0); // bottom right
	glEnd();

	// bottom
	glBegin(GL_QUADS);
		glNormal3d(0.0,-1.001,0.0);
		glTexCoord2f(tilesW, tilesH);
		glVertex3d(1.001,0.0,-1.001); // top right
		glTexCoord2f(0, tilesH);
		glVertex3d(0.0,0.0,-1.001); // top left
		glTexCoord2f(0, 0);
		glVertex3d(0.0,0.0,0.0); // bottom left
		glTexCoord2f(tilesW, 0);
		glVertex3d(1.001,0.0,0.0); // bottom right
	glEnd();

	// front
	glBegin(GL_QUADS);
		glNormal3f(0.0,0.0,1.001);
		glTexCoord2f(tilesH,1);
		glVertex3d(1.001,1.001,0.0); // top right
		glTexCoord2f(0,1);
		glVertex3d(0.0,1.001,0.0); // top left
		glTexCoord2f(0, 0);
		glVertex3d(0.0,0.0,0.0); // bottom left
		glTexCoord2f(tilesH, 0);
		glVertex3d(1.001,0.0,0.0); // bottom right
	glEnd();

	// back
	glBegin(GL_QUADS);
		glNormal3f(0.0,0.0,-1.0);
		glTexCoord2f(tilesW, 1);
		glVertex3d(0.0,0.0,-1.001); // bottom left
		glTexCoord2f(0, 1);
		glVertex3d(1.001,0.0,-1.001); // bottom right
		glTexCoord2f(0, 0);
		glVertex3d(1.001,1.001,-1.001); // top right
		glTexCoord2f(tilesW, 0);
		glVertex3d(0.0,1.001,-1.001); // top left
	glEnd();

	// left
	glBegin(GL_QUADS);
		glNormal3f(-1.0,0.0,0.0);
		glTexCoord2f(tilesW, 1);
		glVertex3d(0.0,1.001,-1.001); // top right
		glTexCoord2f(0, 1);
		glVertex3d(0.0,1.001,0.0); // top left
		glTexCoord2f(0, 0);
		glVertex3d(0.0,0.0,0.0); // bottom left
		glTexCoord2f(tilesW, 0);
		glVertex3d(0.0,0.0,-1.001); // bottom right
	glEnd();

	// right
	glBegin(GL_QUADS);
		glNormal3f(1.0,0.0,0.0);
		glTexCoord2f(tilesW,1);
		glVertex3d(1.001,1.001,0.0); // top right
		glTexCoord2f(0,1);
		glVertex3d(1.001,1.001,-1.001); // top left
		glTexCoord2f(0,0);
		glVertex3d(1.001,0.0,-1.001); // bottom left
		glTexCoord2f(tilesW,0);
		glVertex3d(1.001,0.0,0.0); // bottom right
	glEnd();

    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	glPopMatrix();
}

void Viewer::draw_wall(double x, double y, double z, int length, char dir, Colour c)
{
	glPushMatrix();

	glColor3d(1,1,1); //c.R(),c.G(),c.B());
	glTranslated(x,y,z);
	int a = 1; int b = 1;
	if(dir == 'x') {
		glScaled(length,3.0,1.0);
		a = length;
	}

	if(dir == 'z') {
		glScaled(1.0,3.0,length);
		b = length;
	}
	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);						// Enable Smooth Shading
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);					// Black Background
	glClearDepth(1.0f);							// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);						// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);							// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);			// Really Nice Perspective Calculations

	glBindTexture(GL_TEXTURE_2D, m_wall_textures[0]);
	glBegin(GL_QUADS);
	// top
		glNormal3d(0.0,1.001,0.0);
		glTexCoord2f(a, b); glVertex3d(1.001, 1.001,-1.001); // top right
		glTexCoord2f(0, b); glVertex3d(0.0, 1.001,-1.001); // top left
		glTexCoord2f(0, 0); glVertex3d(0.0, 1.001, 0.0); // bottom left
		glTexCoord2f(a, 0); glVertex3d(1.001, 1.001, 0.0); // bottom right
	glEnd();

	// bottom
	glBegin(GL_QUADS);
		glNormal3d(0.0,-1.001,0.0);
		glTexCoord2f(a, b); glVertex3d(1.001,0.0,-1.001); // top right
		glTexCoord2f(0, b); glVertex3d(0.0,0.0,-1.001); // top left
		glTexCoord2f(0, 0); glVertex3d(0.0,0.0,0.0); // bottom left
		glTexCoord2f(a, 0); glVertex3d(1.001,0.0,0.0); // bottom right
	glEnd();

	// front
	glBegin(GL_QUADS);
		glNormal3f(0.0,0.0,1.001);
		glTexCoord2f(a, 3); glVertex3d(1.001,1.001,0.0); // top right
		glTexCoord2f(0, 3); glVertex3d(0.0,1.001,0.0); // top left
		glTexCoord2f(0, 0); glVertex3d(0.0,0.0,0.0); // bottom left
		glTexCoord2f(a, 0); glVertex3d(1.001,0.0,0.0); // bottom right
	glEnd();

	// back
	glBegin(GL_QUADS);
		glNormal3f(0.0,0.0,-1.0);
		glTexCoord2f(a, 3); glVertex3d(0.0,0.0,-1.001); // bottom left
		glTexCoord2f(0, 3); glVertex3d(1.001,0.0,-1.001); // bottom right
		glTexCoord2f(0, 0); glVertex3d(1.001,1.001,-1.001); // top right
		glTexCoord2f(a, 0); glVertex3d(0.0,1.001,-1.001); // top left
	glEnd();

	// left
	glBegin(GL_QUADS);
		glNormal3f(-1.0,0.0,0.0);
		glTexCoord2f(b, 3); glVertex3d(0.0,1.001,-1.001); // top right
		glTexCoord2f(0, 3); glVertex3d(0.0,1.001,0.0); // top left
		glTexCoord2f(0, 0); glVertex3d(0.0,0.0,0.0); // bottom left
		glTexCoord2f(b, 0); glVertex3d(0.0,0.0,-1.001); // bottom right
	glEnd();

	// right
	glBegin(GL_QUADS);
		glNormal3f(1.0,0.0,0.0);
		glTexCoord2f(b, 3); glVertex3d(1.001,1.001,0.0); // top right
		glTexCoord2f(0, 3); glVertex3d(1.001,1.001,-1.001); // top left
		glTexCoord2f(0, 0); glVertex3d(1.001,0.0,-1.001); // bottom left
		glTexCoord2f(b, 0); glVertex3d(1.001,0.0,0.0); // bottom right
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
	glRotated(m_tilt_z,0.0,0.0,1.0);
	
	int width = m_maze->getWidth();
	int height = m_maze->getHeight();

	// floor
	draw_floor(width,height);

	// outside walls
	//draw_wall(-width/2-1, 0,  height/2+1, width+2,'x',Colour(0,0,1));
	//draw_wall(-width/2-1, 0,  height/2+1, height+2,'z',Colour(0,0,1));
	//draw_wall(-width/2-1, 0, -height/2-1, width+2,'x',Colour(0,0,1));
	//wdraw_wall( width/2+1, 0,  height/2+1, height+2,'z',Colour(0,0,1));

	for(int x = 0; x < m_maze->getWidth(); x++) {
		for(int z = 0; z < m_maze->getHeight(); z++) {
			char id = (*m_maze)(x,z);
			if(id == 'w') {
				draw_wall(-width/2 + x,0,-height/2 + z,1,'x', Colour(1,0,0));
			}
			if(id == 's' && !m_ball_set) {
				std::cout << "width = " << width << " height = " << height << std::endl;
				std::cout << "x = " << x << " z = " << z << std::endl;
				m_ball = Ball((int)width/2 - x - 0.5,3,(int)height/2 - z + 0.5,ball_radius);
				std::cout << "ball found" << std::endl;
				m_ball_set = true;
			}
		}
	}
	glPopMatrix();
	//draw_spire();
}

void Viewer::draw_skybox()
{
   // Store the current matrix
	glPushMatrix();

	// Reset and transform the matrix.
	//glLoadIdentity();
	//gluLookAt(
	//	0,0,0,
	//	m_camera[0],m_camera[1],m_camera[2],
	//	0,1,0);
	//glTranslated(-m_camera[0],-m_camera[1],-m_camera[2]);
	glScaled(1000,1000,1000);
	// Enable/Disable features
	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_BLEND);

	// Just in case we set all vertices to white.
	glColor4f(1,1,1,1);

	// Render the front quad

	glBindTexture(GL_TEXTURE_2D, m_texture[2]);
	glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex3f(  0.5f, -0.5f, -0.5f );
		glTexCoord2f(1, 0); glVertex3f( -0.5f, -0.5f, -0.5f );
		glTexCoord2f(1, 1); glVertex3f( -0.5f,  0.5f, -0.5f );
		glTexCoord2f(0, 1); glVertex3f(  0.5f,  0.5f, -0.5f );
	glEnd();

	// Render the left quad
	glBindTexture(GL_TEXTURE_2D, m_texture[5]);
	glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex3f(  0.5f, -0.5f,  0.5f );
		glTexCoord2f(1, 0); glVertex3f(  0.5f, -0.5f, -0.5f );
		glTexCoord2f(1, 1); glVertex3f(  0.5f,  0.5f, -0.5f );
		glTexCoord2f(0, 1); glVertex3f(  0.5f,  0.5f,  0.5f );
	glEnd();

	// Render the back quad
	glBindTexture(GL_TEXTURE_2D, m_texture[0]);
	glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex3f( -0.5f, -0.5f,  0.5f );
		glTexCoord2f(1, 0); glVertex3f(  0.5f, -0.5f,  0.5f );
		glTexCoord2f(1, 1); glVertex3f(  0.5f,  0.5f,  0.5f );
		glTexCoord2f(0, 1); glVertex3f( -0.5f,  0.5f,  0.5f );

	glEnd();

	// Render the right quad
	glBindTexture(GL_TEXTURE_2D, m_texture[1]);
	glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex3f( -0.5f, -0.5f, -0.5f );
		glTexCoord2f(1, 0); glVertex3f( -0.5f, -0.5f,  0.5f );
		glTexCoord2f(1, 1); glVertex3f( -0.5f,  0.5f,  0.5f );
		glTexCoord2f(0, 1); glVertex3f( -0.5f,  0.5f, -0.5f );
	glEnd();

	// Render the top quad
	glBindTexture(GL_TEXTURE_2D, m_texture[3]);
	glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex3f( -0.5f,  0.5f, -0.5f );
		glTexCoord2f(0, 0); glVertex3f( -0.5f,  0.5f,  0.5f );
		glTexCoord2f(1, 0); glVertex3f(  0.5f,  0.5f,  0.5f );
		glTexCoord2f(1, 1); glVertex3f(  0.5f,  0.5f, -0.5f );
	glEnd();

	// Render the bottom quad
	glBindTexture(GL_TEXTURE_2D, m_texture[4]);
	glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex3f( -0.5f, -0.5f, -0.5f );
		glTexCoord2f(0, 1); glVertex3f( -0.5f, -0.5f,  0.5f );
		glTexCoord2f(1, 1); glVertex3f(  0.5f, -0.5f,  0.5f );
		glTexCoord2f(1, 0); glVertex3f(  0.5f, -0.5f, -0.5f );
	glEnd();

	// Restore enable bits and matrixr
	glPopAttrib();
	glPopMatrix();
}

void Viewer::draw_spire()
{
	glPushMatrix();
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glRotatef(90,1.0,0.0,0.0);
	glScalef(m_width/300,m_width/300,500);
    glBindTexture(GL_TEXTURE_2D, m_wall_textures[2]);
    gluCylinder(m_quadratic,1,1,3,32,32);
    glPopMatrix();
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
}

void Viewer::draw_all() {
	Glib::RefPtr<Gdk::GL::Drawable> gldrawable = get_gl_drawable();
	glDrawBuffer(GL_BACK);

	if (!gldrawable->gl_begin(get_gl_context()))
		return;

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
		double balldist = 8;
		double yrotrad = (m_rotate_y / 180) * 3.141592654;
		double xrotrad = (m_rotate_x / 180) * 3.141592654;
		double hball = balldist*sin(xrotrad);
		double xball = balldist*cos(xrotrad)*sin(yrotrad);
		double zball = balldist*cos(xrotrad)*cos(yrotrad);

		m_camera = Point3D(m_ball.m_location[0]+xball,
				m_ball.m_location[1]+hball,m_ball.m_location[2]-zball);
	}
	std::cout << "camera: " << m_camera << std::endl;

	// for flying mode, rotate y based on mouse x
	glRotated(m_rotate_x,1.0,0.0,0.0);
	glRotated(m_rotate_y,0.0,1.0,0.0);

	glTranslated(m_camera[0],-m_camera[1],m_camera[2]);
	draw_skybox();

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
}

bool Viewer::do_physics() {

	if (m_mode == GAME) {
		double delta_t = time_refresh / 1000;
		double xtiltrad = (m_tilt_x / 180) * 3.141592654; //this moves the ball in the z direction (tilt around x)
		double ztiltrad = (m_tilt_z / 180) * 3.141592654; 
		double gforcex = g*sin(ztiltrad); //net force in the direction of the tilt
		double gforcex_h = gforcex * cos(ztiltrad); 
		double gforcez = g*sin(-xtiltrad);
		double gforcez_h = gforcez * cos(xtiltrad);

		//double gforcey = gforcex*sin(ztiltrad) + gforcez*sin(-xtiltrad);
		double netforcey = 0;
		//double floor_normal_v = max(g*cos(ztiltrad),g*cos(xtiltrad))*min(cos(ztiltrad),cos(xtiltrad));

		double ball_z = -m_ball.m_location[2];
		double ball_x = m_ball.m_location[0];
		
		double floor_y = (-m_floor_normal[0]*ball_x+m_floor_normal[2]*ball_z)/m_floor_normal[1];
		double floor_ya = ball_z * tan(-xtiltrad) + ball_x * tan(-ztiltrad);
		std::cout << "Floor height at ball's location = " << floor_y << " or " << floor_ya << std::endl;

		double bx = ball_x / cos(ztiltrad);
		double bz = ball_z / cos(xtiltrad);
		double cornerd = sqrt(bx*bx+bz*bz);
		
		std::cout << "corner distance = " << cornerd << std::endl;
		double nta = asin(floor_y / cornerd);
		std::cout << "x tilt rad = " << xtiltrad << " ; z tilt rad = " << ztiltrad << std::endl;
		if (fabs(xtiltrad) < 0.0000001) nta = ztiltrad;
		else if (fabs(ztiltrad) < 0.0000001) nta = xtiltrad;
		std::cout << "net tilt angle = " << nta << std::endl;
		double baa = ball_radius / cos(nta);
		
		std::cout << "adjustment = " << baa << std::endl;


		double floor_ball_dist = is_ball_below_floor();
		std::cout << "floor-ball dist = " << floor_ball_dist << std::endl;


		//if(floor_ball_dist > 0.0) {
		//	m_ball.m_location[1] -= m_ball.m_velocity[1]*delta_t - 0.5*g*delta_t*delta_t;
		//	m_ball.m_velocity[1] = m_ball.m_velocity[1] + g*delta_t;
		//} else {
			m_ball.m_location[1] = floor_y + baa; 
			if ( floor_ball_dist < 0.05 && fabs(m_ball.m_velocity[1]) < 0.1) {
				m_ball.m_velocity[1] = 0;
			} else {
				m_ball.m_velocity[1] = -m_ball.m_velocity[1] * 0.1;
				m_ball.m_location[1] -= m_ball.m_velocity[1]*delta_t - 0.5*g*delta_t*delta_t;
			} 
			
		//}
		
		
		if (m_ball.m_velocity[0] != 0) {
			m_ball.m_angle[2] = m_ball.m_velocity[0] * 360;
		}
		if (m_ball.m_velocity[2] != 0) {
			m_ball.m_angle[0] = -m_ball.m_velocity[2] * 360;
		}
		
		m_ball.m_velocity = Point3D(m_ball.m_velocity[0] + gforcex_h * delta_t, 
				m_ball.m_velocity[1], 
				m_ball.m_velocity[2] + gforcez_h*delta_t);
		m_ball.m_location = Point3D(m_ball.m_location[0] + m_ball.m_velocity[0]*delta_t,
				m_ball.m_location[1], 
				m_ball.m_location[2] + m_ball.m_velocity[2]*delta_t);
		
		
		m_ball.m_velocity[0] *= 0.9999;
		m_ball.m_velocity[2] *= 0.9999;
		

		std::cout << "ball velocity: " << m_ball.m_velocity << std::endl;
		std::cout << "ball location: " << m_ball.m_location << std::endl;
		std::cout << "tilt x = " << m_tilt_x << " ; tilt z = " << m_tilt_z << std::endl;
		if(is_ball_in_wall() < 0) {
			std::cout << "ball in wall" << std::endl;
		}
		invalidate();
	}
	return true;
}
Matrix4x4 getRotate(char axis, double angle)
{
	Vector4D r1,r2,r3,r4;
	double rad = (angle/180.0) * 3.14159265;
	switch(axis) {
		case 'x':
			r1 = Vector4D(1,0,0,0);
			r2 = Vector4D(0,cos(rad),-sin(rad),0);
			r3 = Vector4D(0,sin(rad),cos(rad),0);
			break;
		case 'y':
			r1 = Vector4D(cos(rad),0,sin(rad),0);
			r2 = Vector4D(0,1,0,0);
			r3 = Vector4D(-sin(rad),0,cos(rad),0);
			break;
		case 'z':
			r1 = Vector4D(cos(rad),-sin(rad),0,0);
			r2 = Vector4D(sin(rad),cos(rad),0,0);
			r3 = Vector4D(0,0,1,0);
			break;
	}
	r4 = Vector4D(0,0,0,1);
    return Matrix4x4(r1,r2,r3,r4);
}

double Viewer::is_ball_in_wall() {
	Point3D p1,p2,p3,p4,p5,p6;
	double A, B, C, dist;
	Vector3D normal;
	Matrix4x4 rotation = getRotate('x', m_tilt_x) * getRotate('z',m_tilt_z);
	Vector3D sphereCentre = Vector3D(m_ball.m_location[0],m_ball.m_location[1],m_ball.m_location[2]);
	double min_dist = 99999;
	for(int x = 0; x < m_maze->getWidth(); x++) {
		for(int z = 0; z < m_maze->getHeight(); z++) {
			char id = (*m_maze)(x,z);
			if(id == 'w') {
				p1 = Point3D(x,0,z);
				p2 = Point3D(x+1,0,z);
				p3 = Point3D(x,1,z);
				p4 = Point3D(x+1,0,z-1);
				p5 = Point3D(x,1,z-1);
				p6 = Point3D(x+1,1,z-1);

				for(int i = 0; i < 3; i++) {
					A = p1[1] * (p2[2] - p3[2]) + p2[1] * (p3[2] - p1[2]) + p3[1] * (p1[2] - p2[2]);
					B = p1[2] * (p2[0] - p3[0]) + p2[2] * (p3[0] - p1[0]) + p3[2] * (p1[0] - p2[0]);
					C = p1[0] * (p2[1] - p3[1]) + p2[0] * (p3[1] - p1[1]) + p3[0] * (p1[1] - p2[1]);

					normal = rotation * Vector3D(A,B,C); normal.normalize();
					dist = m_floor_normal.dot(sphereCentre)-m_ball.m_radius;
					if(dist < 0) {
						return dist;
					}

					if(i==0) p2 = p5;
					if(i==1) { p1 = p6; p3 = p4; }
					if(i==2) p2 =  Point3D(x+1,0,z); // back to p2
					min_dist = std::min(min_dist,dist);
				}
			}
		}
	}
	return min_dist;
}

double Viewer::is_ball_below_floor() {
	// Three points in the floor 'plane'
	int width = m_maze->getWidth();
	int height = m_maze->getHeight();
	Matrix4x4 rotateX = getRotate('x',-m_tilt_x);
	Matrix4x4 rotateZ = getRotate('z',-m_tilt_z);
	Point3D p1 = /*rotateZ * rotateX * */ Point3D(0,0,0);
	Point3D p2 = /* rotateZ * rotateX * */ Point3D(-width/2,0,0);
	Point3D p3 = /* rotateZ * rotateX * */ Point3D(0,0,height/2);


	double A = p1[1] * (p2[2] - p3[2]) + p2[1] * (p3[2] - p1[2]) + p3[1] * (p1[2] - p2[2]);
	double B = p1[2] * (p2[0] - p3[0]) + p2[2] * (p3[0] - p1[0]) + p3[2] * (p1[0] - p2[0]);
	double C = p1[0] * (p2[1] - p3[1]) + p2[0] * (p3[1] - p1[1]) + p3[0] * (p1[1] - p2[1]);
	//double negD = p1[0] * (p2[1] * p3[2] - p3[1] * p2[2])
	//		    + p2[0] * (p3[1] * p1[2] - p1[1] * p3[2])
	//		    + p3[0] * (p1[1] * p2[2] - p2[1] * p1[2]);
	std::cout << "A:" << A << "B:" << B << "C:" << C << std::endl;
	m_floor_normal = Vector3D(A,B,C);
	m_floor_normal = rotateZ * rotateX * m_floor_normal; m_floor_normal.normalize();
	std::cout << "normal vector = " << m_floor_normal << std::endl;
	
	Vector3D sphereCentre = Vector3D(m_ball.m_location[0],m_ball.m_location[1],m_ball.m_location[2]);

	double dist = m_floor_normal.dot(sphereCentre);
	//if(dist-m_ball.m_radius > 0) return true;
	//m_floor_d = -negD;
	//if (fabs(dist-m_ball.m_radius) < 0.000001) return 0;
	return dist-m_ball.m_radius;
	//return false;
}



