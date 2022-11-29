#include "vt_window.hpp"

namespace vt
{
	VtWindow::VtWindow(int w, int h, std::string name) : width(w), height(h), windowName(name)
	{
		initWindow();
	}

	VtWindow::~VtWindow()
	{
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void VtWindow::initWindow()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
	}

	
}


