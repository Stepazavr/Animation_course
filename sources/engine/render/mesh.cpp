#include "mesh.h"
#include <vector>
#include "glad/glad.h"

static void create_indices(std::span<const uint32_t> indices)
{
  GLuint arrayIndexBuffer;
  glGenBuffers(1, &arrayIndexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, arrayIndexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * indices.size(), indices.data(), GL_STATIC_DRAW);
  glBindVertexArray(0);
}

template <typename T>
static void init_channel(int channel_index, std::span<const T> channel)
{
  if (channel.size() == 0)
    return;
  GLuint arrayBuffer;
  glGenBuffers(1, &arrayBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);
  glBufferData(GL_ARRAY_BUFFER, channel.size() * sizeof(T), channel.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(channel_index);

  const int componentCount = T::length();
  if constexpr (std::is_same<typename T::value_type, float>::value)
    glVertexAttribPointer(channel_index, componentCount, GL_FLOAT, GL_FALSE, 0, 0);
  else
    glVertexAttribIPointer(channel_index, componentCount, GL_UNSIGNED_INT, 0, 0);
}

template <typename... Channel> // Channel is vector<vec3>, vector<vec2> etc
static MeshPtr create_mesh_impl(const char *name, std::span<const uint32_t> indices, Channel &&...channels)
{
  uint32_t vertexArrayBufferObject;
  glGenVertexArrays(1, &vertexArrayBufferObject);
  glBindVertexArray(vertexArrayBufferObject);

  int channelIdx = 0;
  (init_channel(channelIdx++, channels), ...);

  create_indices(indices);
  return std::make_shared<Mesh>(name, vertexArrayBufferObject, indices.size());
}

MeshPtr create_mesh(
    const char *name,
    std::span<const uint32_t> indices,
    std::span<const vec3> vertices,
    std::span<const vec3> normals,
    std::span<const vec2> uv,
    std::span<const vec4> weights,
    std::span<const uvec4> weightsIndex)
{
  return create_mesh_impl(name, indices, vertices, normals, uv, weights, weightsIndex);
}

MeshPtr create_mesh(
    const char *name,
    std::span<const uint32_t> indices,
    std::span<const vec3> vertices,
    std::span<const vec3> normals,
    std::span<const vec2> uv)
{
  return create_mesh_impl(name, indices, vertices, normals, uv);
}


void render(const MeshPtr &mesh)
{
  glBindVertexArray(mesh->vertexArrayBufferObject);
  glDrawElementsBaseVertex(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, 0, 0);
}

MeshPtr make_plane_mesh()
{
  std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 3};
  std::vector<vec3> vertices = {vec3(-1, 0, -1), vec3(1, 0, -1), vec3(1, 0, 1), vec3(-1, 0, 1)};
  std::vector<vec3> normals(4, vec3(0, 1, 0));
  std::vector<vec2> uv = {vec2(0, 0), vec2(1, 0), vec2(1, 1), vec2(0, 1)};
  return create_mesh("plane", indices, vertices, normals, uv);
}
