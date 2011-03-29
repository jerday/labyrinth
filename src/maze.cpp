#include "maze.hpp"

Maze::Maze() {

}

Maze::Maze(Image & img)
{
	m_width = img.width();
	m_height = img.height();
	m_maze_data = new char[m_width*m_height];

	int r,g,b;
	bool start_found = false;
	bool end_found = false;
	for(int w = 0; w < m_width;w++) {
		for(int h = 0; h < m_height; h++) {
			r = img(w,h,0);
			g = img(w,h,1);
			b = img(w,h,2);

			// white
			if(r && g && b) {
				m_maze_data[w*m_width + h] = 'e'; // wall
			}
			// black
			else if(!r && !g && !b) {
				m_maze_data[w*m_width + h] = 'w'; // empty
			}
			// red
			else if(r && !g && !b) {
				m_maze_data[w*m_width + h] = 'f'; // finish
				if(end_found) {
					m_valid = false;
					m_error_message = "Multiple exits";
					return;
				}
				end_found = true;
			}
			// green
			else if(!r && g && !b) {
				m_maze_data[w*m_width + h] = 's'; // start
				if(start_found) {
					m_valid = false;
					m_error_message = "Multiple starting points";
					return;
				}
				start_found = true;
			}
			// blue
			else if(!r && !g && b) {
				m_maze_data[w*m_width + h] = 'h'; // hole
			} else {
				m_valid = false;
			}
		}
	}
	m_valid = true;
}

bool Maze::isValid() {
	return m_valid;
}
std::string Maze::getErrorMessage() {
	return m_error_message;
}

Maze::~Maze() {

}

char Maze::operator()(int x, int y) {
	return m_maze_data[x*m_width + y];
}
