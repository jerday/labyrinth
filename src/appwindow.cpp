#include "appwindow.hpp"
#include "image.hpp"
#include <iostream>
#include "SoundManager.h"

AppWindow::AppWindow()
{
  set_title("Labyrinth");

  using Gtk::Menu_Helpers::MenuElem;
  using Gtk::Menu_Helpers::RadioMenuElem;
  using Gtk::Menu_Helpers::CheckMenuElem;

  // Set up the application menu
  m_menu_app.items().push_back(MenuElem("_Rickroll", Gtk::AccelKey("r"), sigc::mem_fun(*this, &AppWindow::rickroll)));
  m_menu_app.items().push_back(MenuElem("_Quit", Gtk::AccelKey("q"), sigc::mem_fun(*this, &AppWindow::hide)));

  // Set up the mode menu
  sigc::slot1<void, Viewer::Mode> mode_slot = sigc::mem_fun(m_viewer, &Viewer::set_mode);
  Gtk::RadioButtonGroup group = Gtk::RadioButtonGroup();
  m_menu_mode.items().push_back(RadioMenuElem(group, "_Bird's eye view", Gtk::AccelKey("b"), sigc::bind( mode_slot, Viewer::BIRDS_EYE ) ) );
  m_menu_mode.items().push_back(RadioMenuElem(group, "_Game", Gtk::AccelKey("g"), sigc::bind( mode_slot, Viewer::GAME ) ) );

 // Texture menu
  sigc::slot1<void, Viewer::Skybox> skybox_slot = sigc::mem_fun(m_viewer, &Viewer::set_skybox);
  Gtk::RadioButtonGroup sky = Gtk::RadioButtonGroup();
  m_menu_textures.items().push_back(RadioMenuElem(sky, "_Alpine", sigc::bind( skybox_slot, Viewer::ALPINE)));
  m_menu_textures.items().push_back(RadioMenuElem(sky, "_Calm", sigc::bind( skybox_slot, Viewer::CALM)));
  m_menu_textures.items().push_back(RadioMenuElem(sky, "C_ity", sigc::bind( skybox_slot, Viewer::CITY)));
  m_menu_textures.items().push_back(RadioMenuElem(sky, "_Desert Evening", sigc::bind( skybox_slot, Viewer::DESERT_EVENING)));
  m_menu_textures.items().push_back(RadioMenuElem(sky, "_Entropic", sigc::bind( skybox_slot, Viewer::ENTROPIC)));
  m_menu_textures.items().push_back(RadioMenuElem(sky, "_Hourglass", sigc::bind( skybox_slot, Viewer::HOURGLASS)));
  m_menu_textures.items().push_back(RadioMenuElem(sky, "_Islands", sigc::bind( skybox_slot, Viewer::ISLANDS)));
  m_menu_textures.items().push_back(RadioMenuElem(sky, "_Lost Valley", sigc::bind( skybox_slot, Viewer::LOST_VALLEY)));
  m_menu_textures.items().push_back(RadioMenuElem(sky, "_Night Sky", sigc::bind( skybox_slot, Viewer::NIGHT_SKY)));
  m_menu_textures.items().push_back(RadioMenuElem(sky, "_Red", sigc::bind( skybox_slot, Viewer::RED)));

  // Set up the menu bar
  m_menubar.items().push_back(MenuElem("_File", m_menu_app));

  // Modes
  m_menubar.items().push_back(MenuElem("_Mode", m_menu_mode));

  // Textures
  m_menubar.items().push_back(MenuElem("_Textures", m_menu_textures));

  // First add the vertical box as our single "top" widget
  add(m_vbox);

  // Put the menubar on the top, and make it as small as possible
  m_vbox.pack_start(m_menubar, Gtk::PACK_SHRINK);

  // Put the viewer below the menubar. pack_start "grows" the widget
  // by default, so it'll take up the rest of the window.
  m_viewer.set_size_request(600, 600);
  m_vbox.pack_start(m_viewer);

  show_all();
}

void AppWindow::rickroll() {
	int id = SM.LoadMusic("../data/rickroll.wav");
	SM.PlayMusic(id);
}
bool AppWindow::on_key_press_event( GdkEventKey *ev )
{
        // This is a convenient place to handle non-shortcut
        // keys.  You'll want to look at ev->keyval.

	// An example key; delete and replace with the
	// keys you want to process
        if( ev->keyval == 't' ) {
                std::cerr << "Hello!" << std::endl;
                return true;
        } else {
                return Gtk::Window::on_key_press_event( ev );
        }
}

bool AppWindow::parse_maze(std::string filename)
{
	Image img;
	bool ret = img.loadPng(filename);
	if(!ret) {
		std::cerr << "Error opening file: " << filename << ". Exiting." << std::endl;
		return false;
	}

	m_maze = new Maze(img);

	if(!m_maze->isValid()) {
		std::cerr << "Error: " << m_maze->getErrorMessage() << std::endl;
		return false;
	}

	m_viewer.set_maze(m_maze);
	m_viewer.set_mode(Viewer::BIRDS_EYE);
	return true;
}
