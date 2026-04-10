#include "render/mesh.h"
#include <vector>
#include <3dmath.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include "engine/api.h"
#include "glad/glad.h"
#include "engine/animation/ozz_converter.h"
#include <ozz/animation/runtime/sampling_job.h>

#include "import/model.h"
#include "application/character.h"

inline glm::mat4 to_mat4(const aiMatrix4x4& from)
{
	glm::mat4 to;
	to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
	to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
	to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
	to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
	return to;
}

void extract_skeleton(Skeleton& skeleton, const aiNode* node, int parent_idx)
{
	int current_idx = skeleton.bones.size();
	Bone bone;
	bone.name = node->mName.C_Str();
	bone.parent_idx = parent_idx;
	bone.local_transform = to_mat4(node->mTransformation);
	skeleton.bones.push_back(bone);

	if (parent_idx != -1)
	{
		skeleton.bones[parent_idx].children_indices.push_back(current_idx);
	}

	for (uint32_t i = 0; i < node->mNumChildren; i++)
	{
		extract_skeleton(skeleton, node->mChildren[i], current_idx);
	}
}

MeshPtr create_mesh(const aiMesh *mesh, Skeleton& skeleton)
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
      
	  for (int bone_idx = 0; bone_idx < skeleton.bones.size(); ++bone_idx)
	  {
		  if (skeleton.bones[bone_idx].name == bone->mName.C_Str())
		  {
			  skeleton.bones[bone_idx].offset_matrix = to_mat4(bone->mOffsetMatrix);
			  break;
		  }
	  }

      for (unsigned j = 0; j < bone->mNumWeights; j++)
      {
        int vertex = bone->mWeights[j].mVertexId;
        int offset = weightsOffset[vertex]++;
        weights[vertex][offset] = bone->mWeights[j].mWeight;
        
        for (int bone_idx = 0; bone_idx < skeleton.bones.size(); ++bone_idx)
        {
            if (skeleton.bones[bone_idx].name == bone->mName.C_Str())
            {
                weightsIndex[vertex][offset] = bone_idx;
                break;
            }
        }
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

ModelAsset load_model(const char *path, Character& character)
{
  Skeleton& skeleton = character.skeleton;
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

  extract_skeleton(skeleton, scene->mRootNode, -1);

  for (uint32_t i = 0; i < scene->mNumMeshes; i++)
  {
    model.meshes.push_back(create_mesh(scene->mMeshes[i], skeleton));
  }

  const int nj = skeleton.bones.size();
  character.inverse_bind_matrices.resize(nj);
  for (int i = 0; i < nj; ++i)
    character.inverse_bind_matrices[i] = skeleton.bones[i].offset_matrix;

  character.ozz_skeleton = ozz_converter::convert_skeleton(skeleton);

  engine::log("Model \"%s\" loaded", path);

  return model;
}

void load_animation(const char *path, Character& character)
{
  if (!character.ozz_skeleton) {
    engine::log("Cannot load animation: skeleton not initialized for character '%s'", character.name.c_str());
    return;
  }

  Assimp::Importer importer;
  importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
  importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 1.f);

  importer.ReadFile(path, aiPostProcessSteps::aiProcess_Triangulate | aiPostProcessSteps::aiProcess_LimitBoneWeights |
                              aiPostProcessSteps::aiProcess_GenNormals | aiProcess_GlobalScale | aiProcess_FlipWindingOrder);

  const aiScene *scene = importer.GetScene();
  if (!scene) {
    engine::error("Failed to read animation file \"%s\"", path);
    return;
  }

  if (scene->mNumAnimations > 0) {
    const aiAnimation* first_animation = scene->mAnimations[0];
    character.ozz_animation = ozz_converter::convert_animation(first_animation, *character.ozz_skeleton);
    
    if (character.ozz_animation) {
      engine::log("Character '%s': Animation \"%s\" loaded from \"%s\" (duration: %.2fs)", 
        character.name.c_str(),
        first_animation->mName.C_Str(),
        path,
        static_cast<float>(first_animation->mDuration / first_animation->mTicksPerSecond));
      
      // Initialize animation data containers
      character.local_transforms.resize(character.ozz_skeleton->num_joints());
      character.model_space_matrices.resize(character.ozz_skeleton->num_joints());
      
      // Create sampling context for animation (only if not already created)
      if (!character.sampling_context) {
        character.sampling_context = ozz::New<ozz::animation::SamplingJob::Context>(character.ozz_skeleton->num_joints());
      }
    }
    else {
      engine::log("Failed to convert animation \"%s\"", first_animation->mName.C_Str());
    }
  }
  else {
    engine::log("No animations found in file \"%s\"", path);
  }
}