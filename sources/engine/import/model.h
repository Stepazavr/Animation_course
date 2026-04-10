#pragma once
#include "render/mesh.h"
#include <vector>
#include <string>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>

struct Character;
struct Skeleton;

struct ModelAsset
{
  std::string path;
  std::vector<MeshPtr> meshes;
};

ModelAsset load_model(const char *path, Character& character);
void load_animation(const char *path, Character& character);

// Load animation and return pointer without initializing character data
ozz::animation::Animation* load_animation_only(const char *path, ozz::animation::Skeleton* skeleton, const Skeleton* engine_skeleton = nullptr);
