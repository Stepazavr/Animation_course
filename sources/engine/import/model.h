#pragma once
#include "render/mesh.h"
#include <vector>
#include <string>

struct Character;

struct ModelAsset
{
  std::string path;
  std::vector<MeshPtr> meshes;
};

ModelAsset load_model(const char *path, Character& character);
void load_animation(const char *path, Character& character);
