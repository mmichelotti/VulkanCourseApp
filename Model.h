#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "Mesh.h"
class Model
{
public:
	Model();
	Model(std::vector<Mesh> meshes);
	~Model();

	size_t getMeshCount() { return meshes.size(); }
	Mesh* getMesh(size_t index);
	glm::mat4 getModel() { return model; }

	void setModel(glm::mat4 model) { this->model = model; }

private:
	std::vector<Mesh> meshes;
	glm::mat4 model;
};

