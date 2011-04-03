#include <iostream>
#include <gtkmm.h>
#include <gtkglmm.h>
#include "appwindow.hpp"

int main(int argc, char** argv)
{
	std::string filename = "maze.png";
	if (argc >= 2) {
		filename = argv[1];
	}

	// Construct our main loop
	Gtk::Main kit(argc, argv);

	// Initialize OpenGL
	Gtk::GL::init(argc, argv);

	// Construct our (only) window
	AppWindow window;

	if(window.parse_maze(filename)) {
		// And run the application
		window.fullscreen();
		Gtk::Main::run(window);
	}
}

