#include "camera.h"

using namespace scene;


Camera::Camera()
{
	_cameraPos = glm::vec3(0.0f, 0.0f, 10.0f);
	_focusPos = glm::vec3(0.0f, 0.0f, 0.0f);
	_front = _focusPos - _cameraPos;
	_up = glm::vec3(0.0f, 1.0f, 0.0f);
	_left = glm::cross(_up, _front);
	_fov = 45.0f;

	_modelMat = glm::mat4(1.0f);
}

Camera::Camera(glm::vec3& cpos = glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3& fpos = glm::vec3(0.0f, 0.0f, 0.0f),
	 glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f), float fov = 4.5f)
{
	_cameraPos = cpos;
	_focusPos = fpos;
	_front = fpos - cpos;
	_up = up;
	_left = glm::cross(up, _front);
	_fov = fov;

	_modelMat = glm::mat4(1.0f);
}

Camera::~Camera()
{
	
}

glm::mat4 Camera::getViewMatrix()
{
	return glm::lookAt(_cameraPos, _focusPos, glm::normalize(_up));
}

glm::mat4 Camera::getProjMatrix(int width, int height)
{
	return glm::perspective(float(glm::radians(_fov)), float(width) / height, 0.1f, 1000.0f);
}

void Camera::setModelMatrix(glm::vec3& rotate, glm::vec3& translate)
{
	glm::mat4 rotateMat = glm::mat4(1.0f);
	glm::mat4 transMat = glm::mat4(1.0f);
	rotateMat = glm::rotate(rotateMat, rotate.x, glm::vec3(1.0f, 0.0f, 0.0f)); // ÈÆXÖáÐý×ª
	rotateMat = glm::rotate(rotateMat, rotate.y, glm::vec3(0.0f, 1.0f, 0.0f)); // ÈÆYÖáÐý×ª
	rotateMat = glm::rotate(rotateMat, rotate.z, glm::vec3(0.0f, 0.0f, 1.0f)); // ÈÆZÖáÐý×ª

	transMat = glm::translate(transMat, translate);

	_translate = transMat * _translate;
	_rotate = rotateMat * _rotate;

	_modelMat = _translate * _rotate;
}