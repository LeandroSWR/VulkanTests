#include "vt_window.hpp"

namespace vt
{
	// Class constructor that initializes the member variables with the given arguments
	VtWindow::VtWindow(int w, int h, std::string name) : width(w), height(h), windowName(name)
	{
		initWindow();
	}

	// Destroy the window and terminate the GLFW library
	VtWindow::~VtWindow()
	{
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	// Initialize the GLFW library and create a window
	void VtWindow::initWindow()
	{
		// Initialize the GLFW library
		glfwInit();
		
		// Specify that no graphics API should be created
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// Specify that the window shouldn't be resizable
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		// Create a new window with the given parameters
		window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
	}

	
}


