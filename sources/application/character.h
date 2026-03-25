#pragma once
#include "engine/3dmath.h"
#include "engine/render/material.h"
#include "engine/render/mesh.h"
#include <vector>
#include <string>

struct Mesh;
struct Material;

struct Bone
{
	std::string name = "";
	int parent_idx = -1;
	glm::mat4 offset_matrix = glm::mat4(1.f);
	glm::mat4 local_transform = glm::mat4(1.f);
	std::vector<int> children_indices;
};

struct Skeleton
{
	std::vector<Bone> bones;
	std::vector<glm::mat4> bone_matrices;
};

struct Character
{
	std::string name;
	glm::mat4 transform;
	std::vector<MeshPtr> meshes;
	MaterialPtr material;
	Skeleton skeleton;
};
