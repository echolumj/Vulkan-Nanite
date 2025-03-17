#pragma once
#include "base.h"


namespace scene {
class Camera
{
public:
	Camera();
	Camera(glm::vec3& cpos, glm::vec3& fpos, glm::vec3& up, float fov);
	~Camera();

	glm::mat4 getViewMatrix(void);
	glm::mat4 getProjMatrix(int width, int height);
	glm::mat4 getModelMatrix(void) { return _modelMat; }
	void setModelMatrix(glm::vec3& rotate, glm::vec3& translate);

private:
	glm::vec3 _cameraPos;
	glm::vec3 _focusPos;
	glm::vec3 _front;
	glm::vec3 _up;
	glm::vec3 _left;

	float _fov;

	glm::mat4 _modelMat;
	glm::mat4 _viewMat;
	glm::mat4 _projMat;

	glm::mat4 _rotate = glm::mat4(1.0f);
	glm::mat4 _translate = glm::mat4(1.0f);
};

}
