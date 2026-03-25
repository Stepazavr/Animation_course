#pragma once
#include "engine/3dmath.h"
#include "engine/render/material.h"
#include "engine/render/mesh.h"

struct Character
{
  std::string name;
  glm::mat4 transform;
  std::vector<MeshPtr> meshes;
  MaterialPtr material;
};
