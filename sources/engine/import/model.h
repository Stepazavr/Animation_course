#pragma once
#include "render/mesh.h"
#include <vector>

struct ModelAsset
{
  std::string path;
  std::vector<MeshPtr> meshes;
};

ModelAsset load_model(const char *path);
