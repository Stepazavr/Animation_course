#include "render/mesh.h"
#include <vector>
#include <3dmath.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include "engine/api.h"
#include "glad/glad.h"

#include "import/model.h"

MeshPtr create_mesh(const aiMesh *mesh)
{
  std::vector<uint32_t> indices;
  std::vector<vec3> vertices;
  std::vector<vec3> normals;
  std::vector<vec2> uv;
  std::vector<vec4> weights;
  std::vector<uvec4> weightsIndex;

  int numVert = mesh->mNumVertices;
  int numFaces = mesh->mNumFaces;

  if (mesh->HasFaces())
  {
    indices.resize(numFaces * 3);
    for (int i = 0; i < numFaces; i++)
    {
      assert(mesh->mFaces[i].mNumIndices == 3);
      for (int j = 0; j < 3; j++)
        indices[i * 3 + j] = mesh->mFaces[i].mIndices[j];
    }
  }

  if (mesh->HasPositions())
  {
    vertices.resize(numVert);
    for (int i = 0; i < numVert; i++)
      vertices[i] = to_vec3(mesh->mVertices[i]);
  }

  if (mesh->HasNormals())
  {
    normals.resize(numVert);
    for (int i = 0; i < numVert; i++)
      normals[i] = to_vec3(mesh->mNormals[i]);
  }

  if (mesh->HasTextureCoords(0))
  {
    uv.resize(numVert);
    for (int i = 0; i < numVert; i++)
      uv[i] = to_vec2(mesh->mTextureCoords[0][i]);
  }

  if (mesh->HasBones())
  {
    weights.resize(numVert, vec4(0.f));
    weightsIndex.resize(numVert);
    int numBones = mesh->mNumBones;
    std::vector<int> weightsOffset(numVert, 0);
    for (int i = 0; i < numBones; i++)
    {
      const aiBone *bone = mesh->mBones[i];
      // bonesMap[std::string(bone->mName.C_Str())] = i;

      for (unsigned j = 0; j < bone->mNumWeights; j++)
      {
        int vertex = bone->mWeights[j].mVertexId;
        int offset = weightsOffset[vertex]++;
        weights[vertex][offset] = bone->mWeights[j].mWeight;
        weightsIndex[vertex][offset] = i;
      }
    }
    // the sum of weights not 1
    for (int i = 0; i < numVert; i++)
    {
      vec4 w = weights[i];
      float s = w.x + w.y + w.z + w.w;
      weights[i] *= 1.f / s;
    }
  }
  return create_mesh(mesh->mName.C_Str(), indices, vertices, normals, uv, weights, weightsIndex);
}

ModelAsset load_model(const char *path)
{

  Assimp::Importer importer;
  importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
  importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 1.f);

  importer.ReadFile(path, aiPostProcessSteps::aiProcess_Triangulate | aiPostProcessSteps::aiProcess_LimitBoneWeights |
                              aiPostProcessSteps::aiProcess_GenNormals | aiProcess_GlobalScale | aiProcess_FlipWindingOrder);

  const aiScene *scene = importer.GetScene();
  ModelAsset model;
  model.path = path;
  if (!scene)
  {
    engine::error("Filed to read model file \"%s\"", path);
    return model;
  }

  model.meshes.resize(scene->mNumMeshes);
  for (uint32_t i = 0; i < scene->mNumMeshes; i++)
  {
    model.meshes[i] = create_mesh(scene->mMeshes[i]);
  }

  engine::log("Model \"%s\" loaded", path);
  return model;
}