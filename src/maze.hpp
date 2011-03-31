#ifndef MAZE_HPP
#define MAZE_HPP

#include "image.hpp"

class Maze {
public:
	Maze();
	Maze(Image & img);
	~Maze();
	char operator()(int x, int y);
	bool isValid();
	std::string getErrorMessage();
	int getWidth() { return m_width; }
	int getHeight() { return m_height; }
	char* m_maze_data;
	int m_width;
	int m_height;
	bool m_valid;
	std::string m_error_message;
private:
};

#endif
