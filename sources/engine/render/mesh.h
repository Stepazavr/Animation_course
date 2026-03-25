#pragma once
#include <map>
#include <memory>
#include <string>
#include <span>
#include "3dmath.h"


struct Mesh
{
  std::string name;
  const uint32_t vertexArrayBufferObject;
  const int numIndices;

  Mesh(const char *name, uint32_t vertexArrayBufferObject, int numIndices) :
    name(name),
    vertexArrayBufferObject(vertexArrayBufferObject),
    numIndices(numIndices)
    {}
};

using MeshPtr = std::shared_ptr<Mesh>;

MeshPtr create_mesh(
    const char *name,
    std::span<const uint32_t> indices,
    std::span<const vec3> vertices,
    std::span<const vec3> normals,
    std::span<const vec2> uv,
    std::span<const vec4> weights,
    std::span<const uvec4> weightsIndex);

MeshPtr create_mesh(
    const char *name,
    std::span<const uint32_t> indices,
    std::span<const vec3> vertices,
    std::span<const vec3> normals,
    std::span<const vec2> uv);

MeshPtr make_plane_mesh();

void render(const MeshPtr &mesh);