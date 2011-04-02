#ifndef APPWINDOW_HPP
#define APPWINDOW_HPP

#include <gtkmm.h>
#include "viewer.hpp"
#include "maze.hpp"


class AppWindow : public Gtk::Window {
public:
  AppWindow();
  bool parse_maze(std::string file);
protected:
  virtual bool on_key_press_event( GdkEventKey *ev );

private:
  // A "vertical box" which holds everything in our window
  Gtk::VBox m_vbox;

  // The menubar, with all the menus at the top of the window
  Gtk::MenuBar m_menubar;
  // Each menu itself
  Gtk::Menu m_menu_app;
  Gtk::Menu m_menu_mode;
  Gtk::Menu m_menu_textures;

  Viewer m_viewer;

  Maze * m_maze;

  // The main OpenGL area
};

#endif
