#include  "interact.h"
//
//Interact::Interact()
//{
//}
//
//Interact::~Interact()
//{
//}
//
//
//static void Interact::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
//	if (button == GLFW_MOUSE_BUTTON_LEFT) {
//		if (action == GLFW_PRESS) {
//			isLeftMousePressed = true;
//			glfwGetCursorPos(window, &lastMouseX, &lastMouseY); // 记录按下时的鼠标位置
//		}
//		else if (action == GLFW_RELEASE) {
//			isLeftMousePressed = false;
//
//		}
//	}
//	else if (button == GLFW_MOUSE_BUTTON_RIGHT)
//	{
//		if (action == GLFW_PRESS) {
//			isRightMousePressed = true;
//			glfwGetCursorPos(window, &lastMouseX, &lastMouseY); // 记录按下时的鼠标位置
//		}
//		else if (action == GLFW_RELEASE) {
//			isRightMousePressed = false;
//
//		}
//	}
//}
//
//
//static void Interact::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
//{
//	transzOffset = yoffset;
//	isScroll = true;
//}