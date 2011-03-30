#ifndef VIEWER_HPP
#define VIEWER_HPP

#include <gtkmm.h>
#include <gtkglmm.h>
#include "algebra.hpp"
#include "maze.hpp"

// The "main" OpenGL widget
class Viewer : public Gtk::GL::DrawingArea {
public:
  Viewer();
  virtual ~Viewer();

  // A useful function that forces this widget to rerender. If you
  // want to render a new frame, do not call on_expose_event
  // directly. Instead call this, which will cause an on_expose_event
  // call when the time is right.
  bool invalidate();
  void set_maze(Maze * maze);
  enum Mode {
	  BIRDS_EYE,
	  GAME
  };
  void set_mode(Mode m) { m_mode = m; }

protected:

  // Called when GL is first initialized
  virtual void on_realize();
  // Called when our window needs to be redrawn
  virtual bool on_expose_event(GdkEventExpose* event);
  // Called when the window is resized
  virtual bool on_configure_event(GdkEventConfigure* event);
  // Called when a mouse button is pressed
  virtual bool on_button_press_event(GdkEventButton* event);
  // Called when a mouse button is released
  virtual bool on_button_release_event(GdkEventButton* event);
  // Called when the mouse moves
  virtual bool on_motion_notify_event(GdkEventMotion* event);
  // Called when the keyboard is pressed
  virtual bool on_key_press_event(GdkEventKey* event);
  void draw_cube(double x, double y, double z);
  void draw_floor(const int width, const int height);
  void draw_wall(double x, double y, double z, double length, char dir);
  void draw_maze();

private:
  bool m_left_click_pressed;
  bool m_middle_click_pressed;
  bool m_right_click_pressed;
  int m_width;
  int m_height;
  double m_mouse_x;
  double m_mouse_y;
  double m_rotate_y;
  double m_rotate_x;

  Point3D m_camera;

  Maze * m_maze;

  Mode m_mode;
};

#endif
