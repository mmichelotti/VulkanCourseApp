#include "Model.h"

Model::Model()
{
}

Model::Model(std::vector<Mesh> meshes) : meshes(meshes), model(glm::mat4(1.0f)) {}

Model::~Model()
{
	for (Mesh& mesh : meshes)
	{
		mesh.cleanUp();
	}
}

Mesh* Model::getMesh(size_t index)
{
	if (index >= meshes.size())
	{
		throw std::runtime_error("Attemppted to acces invalid mesh index");
	}
	return &meshes[index];
}
