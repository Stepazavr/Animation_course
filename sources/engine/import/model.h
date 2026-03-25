#pragma once
#include "render/mesh.h"
#include <vector>

struct Skeleton;

struct ModelAsset
{
  std::string path;
  std::vector<MeshPtr> meshes;
};

ModelAsset load_model(const char *path, Skeleton& skeleton);
