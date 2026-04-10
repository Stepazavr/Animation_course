#include "ozz_converter.h"

#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/offline/animation_builder.h>

#include <engine/import/model.h>
#include <application/character.h>

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <assimp/scene.h>
#include <assimp/anim.h>

#include <string>
#include <vector>
#include <functional>

namespace {
	// Helper to decompose matrix into translation, rotation, scale
	bool decompose_matrix(const glm::mat4& matrix, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale) {
		translation = glm::vec3(matrix[3]);
		
		glm::vec3 col0(matrix[0]);
		glm::vec3 col1(matrix[1]);
		glm::vec3 col2(matrix[2]);
		
		scale.x = glm::length(col0);
		scale.y = glm::length(col1);
		scale.z = glm::length(col2);
		
		if (scale.x != 0.0f) col0 /= scale.x;
		if (scale.y != 0.0f) col1 /= scale.y;
		if (scale.z != 0.0f) col2 /= scale.z;
		
		glm::mat3 rot_matrix(col0, col1, col2);
		rotation = glm::quat_cast(rot_matrix);
		rotation = glm::normalize(rotation);
		
		return true;
	}
}

namespace ozz_converter {

	ozz::animation::Skeleton* convert_skeleton(const Skeleton& engine_skeleton) {
		if (engine_skeleton.bones.empty()) {
			return nullptr;
		}
		
		ozz::animation::offline::RawSkeleton raw_skeleton;
		
		// Обрабатываем все кости, используя parent_idx как источник истины
		// Находим все корни (кости без родителя или с невалидным parent_idx)
		std::vector<int> root_indices;
		std::vector<bool> processed(engine_skeleton.bones.size(), false);
		
		for (size_t i = 0; i < engine_skeleton.bones.size(); ++i) {
			const Bone& bone = engine_skeleton.bones[i];
			// Кость - корень, если:
			// 1. parent_idx == -1 (явно корень)
			// 2. parent_idx указывает на невалидный индекс
			if (bone.parent_idx == -1 || 
				bone.parent_idx < 0 || 
				bone.parent_idx >= (int)engine_skeleton.bones.size()) {
				root_indices.push_back(i);
			}
		}
		
		// Если корней не найдено, все кости считаются отдельными иерархиями
		if (root_indices.empty()) {
			for (size_t i = 0; i < engine_skeleton.bones.size(); ++i) {
				root_indices.push_back(i);
			}
		}
		
		// Lambda функция для рекурсивной обработки
		std::function<void(int, ozz::animation::offline::RawSkeleton::Joint&)> 
		process_bone = [&](int bone_idx, ozz::animation::offline::RawSkeleton::Joint& parent_joint) {
			if (bone_idx < 0 || bone_idx >= (int)engine_skeleton.bones.size() || processed[bone_idx]) {
				return;
			}
			
			processed[bone_idx] = true;
			const Bone& bone = engine_skeleton.bones[bone_idx];
			
			// Decompose local transform
			glm::vec3 translation;
			glm::quat rotation;
			glm::vec3 scale;
			decompose_matrix(bone.local_transform, translation, rotation, scale);
			
			// Create child joint
			ozz::animation::offline::RawSkeleton::Joint child_joint;
			child_joint.name = bone.name.c_str();
			child_joint.transform.translation = ozz::math::Float3(translation.x, translation.y, translation.z);
			child_joint.transform.rotation = ozz::math::Quaternion(rotation.x, rotation.y, rotation.z, rotation.w);
			child_joint.transform.scale = ozz::math::Float3(scale.x, scale.y, scale.z);
			
			parent_joint.children.push_back(child_joint);
			ozz::animation::offline::RawSkeleton::Joint& added_joint = parent_joint.children.back();
			
			// Процессируем ВСЕХ детей:
			// 1. Через children_indices (явно указанные)
			for (int child_idx : bone.children_indices) {
				if (!processed[child_idx]) {
					process_bone(child_idx, added_joint);
				}
			}
			
			// 2. Через поиск костей с parent_idx == bone_idx (на случай orphaned children)
			for (size_t i = 0; i < engine_skeleton.bones.size(); ++i) {
				if (!processed[i] && engine_skeleton.bones[i].parent_idx == bone_idx) {
					process_bone(i, added_joint);
				}
			}
		};
		
		// Обрабатываем корни
		for (int root_idx : root_indices) {
			if (!processed[root_idx]) {
				const Bone& root_bone = engine_skeleton.bones[root_idx];
				
				// Decompose root transform
				glm::vec3 translation;
				glm::quat rotation;
				glm::vec3 scale;
				decompose_matrix(root_bone.local_transform, translation, rotation, scale);
				
				// Create root joint
				ozz::animation::offline::RawSkeleton::Joint root_joint;
				root_joint.name = root_bone.name.c_str();
				root_joint.transform.translation = ozz::math::Float3(translation.x, translation.y, translation.z);
				root_joint.transform.rotation = ozz::math::Quaternion(rotation.x, rotation.y, rotation.z, rotation.w);
				root_joint.transform.scale = ozz::math::Float3(scale.x, scale.y, scale.z);
				
				raw_skeleton.roots.push_back(root_joint);
				ozz::animation::offline::RawSkeleton::Joint& added_root = raw_skeleton.roots.back();
				processed[root_idx] = true;
				
				// Обрабатываем детей корня
				for (int child_idx : root_bone.children_indices) {
					if (!processed[child_idx]) {
						process_bone(child_idx, added_root);
					}
				}
				
				// Обрабатываем orphaned дети
				for (size_t i = 0; i < engine_skeleton.bones.size(); ++i) {
					if (!processed[i] && engine_skeleton.bones[i].parent_idx == root_idx) {
						process_bone(i, added_root);
					}
				}
			}
		}
		
		// Validate raw skeleton
		if (!raw_skeleton.Validate()) {
			return nullptr;
		}
		
		// Build runtime skeleton
		ozz::animation::offline::SkeletonBuilder builder;
		auto unique_skeleton = builder(raw_skeleton);
		
		// Release the unique_ptr to get raw pointer
		// Note: caller is responsible for memory management
		return unique_skeleton.release();
	}

	ozz::animation::Animation* convert_animation(const aiAnimation* anim, const ozz::animation::Skeleton& ozz_skeleton, const Skeleton* engine_skeleton) {
		if (!anim || anim->mNumChannels == 0) {
			return nullptr;
		}
		
		ozz::animation::offline::RawAnimation raw_animation;
		raw_animation.name = anim->mName.C_Str();
		raw_animation.duration = static_cast<float>(anim->mDuration / anim->mTicksPerSecond);
		
		// Resize tracks - one per joint
		raw_animation.tracks.resize(ozz_skeleton.num_joints());
		
		// Process animation channels
		for (unsigned int channel_idx = 0; channel_idx < anim->mNumChannels; ++channel_idx) {
			const aiNodeAnim* channel = anim->mChannels[channel_idx];
			
			// Find joint index by name - search through skeleton joint names
			int joint_idx = -1;
			const char* channel_name = channel->mNodeName.C_Str();
			auto joint_names = ozz_skeleton.joint_names();
			for (int i = 0; i < ozz_skeleton.num_joints(); ++i) {
				if (std::string(joint_names[i]) == channel_name) {
					joint_idx = i;
					break;
				}
			}
			
			if (joint_idx < 0 || joint_idx >= ozz_skeleton.num_joints()) {
				continue;
			}
			
			auto& track = raw_animation.tracks[joint_idx];
			
			// Add translation keys
			for (unsigned int i = 0; i < channel->mNumPositionKeys; ++i) {
				const aiVectorKey& key = channel->mPositionKeys[i];
				float time = static_cast<float>(key.mTime / anim->mTicksPerSecond);
				if (time < 0.0f || time > raw_animation.duration) continue;
				
				const aiVector3D& v = key.mValue;
				ozz::animation::offline::RawAnimation::TranslationKey trans_key;
				trans_key.time = time;
				trans_key.value = ozz::math::Float3(v.x, v.y, v.z);
				track.translations.push_back(trans_key);
			}
			
			// Add rotation keys (Assimp: w,x,y,z -> ozz: x,y,z,w)
			for (unsigned int i = 0; i < channel->mNumRotationKeys; ++i) {
				const aiQuatKey& key = channel->mRotationKeys[i];
				float time = static_cast<float>(key.mTime / anim->mTicksPerSecond);
				if (time < 0.0f || time > raw_animation.duration) continue;
				
				const aiQuaternion& q = key.mValue;
				ozz::animation::offline::RawAnimation::RotationKey rot_key;
				rot_key.time = time;
				rot_key.value = ozz::math::Quaternion(q.x, q.y, q.z, q.w);
				track.rotations.push_back(rot_key);
			}
			
			// Add scale keys
			for (unsigned int i = 0; i < channel->mNumScalingKeys; ++i) {
				const aiVectorKey& key = channel->mScalingKeys[i];
				float time = static_cast<float>(key.mTime / anim->mTicksPerSecond);
				if (time < 0.0f || time > raw_animation.duration) continue;
				
				const aiVector3D& v = key.mValue;
				ozz::animation::offline::RawAnimation::ScaleKey scale_key;
				scale_key.time = time;
				scale_key.value = ozz::math::Float3(v.x, v.y, v.z);
				track.scales.push_back(scale_key);
			}
		}
		
		// Ensure we have at least one keyframe per track
		// For bones without animation data, use their local transform from engine skeleton (rest pose)
		for (size_t track_idx = 0; track_idx < raw_animation.tracks.size(); ++track_idx) {
			auto& track = raw_animation.tracks[track_idx];
			
			// If this track has animation data, ensure all channels have keyframes at time 0
			if (!track.translations.empty() || !track.rotations.empty() || !track.scales.empty()) {
				if (track.translations.empty()) {
					ozz::animation::offline::RawAnimation::TranslationKey key;
					key.time = 0.0f;
					key.value = ozz::math::Float3(0, 0, 0);
					track.translations.push_back(key);
				}
				if (track.rotations.empty()) {
					ozz::animation::offline::RawAnimation::RotationKey key;
					key.time = 0.0f;
					key.value = ozz::math::Quaternion(0, 0, 0, 1);
					track.rotations.push_back(key);
				}
				if (track.scales.empty()) {
					ozz::animation::offline::RawAnimation::ScaleKey key;
					key.time = 0.0f;
					key.value = ozz::math::Float3(1, 1, 1);
					track.scales.push_back(key);
				}
			}
			else if (engine_skeleton && track_idx < engine_skeleton->bones.size()) {
				// No animation data for this bone - use its rest pose from engine skeleton
				const Bone& bone = engine_skeleton->bones[track_idx];
				
				// Decompose the rest pose matrix
				glm::vec3 translation;
				glm::quat rotation;
				glm::vec3 scale;
				decompose_matrix(bone.local_transform, translation, rotation, scale);
				
				// Add identity keyframes with rest pose values
				ozz::animation::offline::RawAnimation::TranslationKey trans_key;
				trans_key.time = 0.0f;
				trans_key.value = ozz::math::Float3(translation.x, translation.y, translation.z);
				track.translations.push_back(trans_key);
				
				ozz::animation::offline::RawAnimation::RotationKey rot_key;
				rot_key.time = 0.0f;
				rot_key.value = ozz::math::Quaternion(rotation.x, rotation.y, rotation.z, rotation.w);
				track.rotations.push_back(rot_key);
				
				ozz::animation::offline::RawAnimation::ScaleKey scale_key;
				scale_key.time = 0.0f;
				scale_key.value = ozz::math::Float3(scale.x, scale.y, scale.z);
				track.scales.push_back(scale_key);
			}
			else {
				// Fallback: use identity transforms if we don't have engine skeleton data
				{
					ozz::animation::offline::RawAnimation::TranslationKey key;
					key.time = 0.0f;
					key.value = ozz::math::Float3(0, 0, 0);
					track.translations.push_back(key);
				}
				{
					ozz::animation::offline::RawAnimation::RotationKey key;
					key.time = 0.0f;
					key.value = ozz::math::Quaternion(0, 0, 0, 1);
					track.rotations.push_back(key);
				}
				{
					ozz::animation::offline::RawAnimation::ScaleKey key;
					key.time = 0.0f;
					key.value = ozz::math::Float3(1, 1, 1);
					track.scales.push_back(key);
				}
			}
		}
		
		// Validate raw animation
		if (!raw_animation.Validate()) {
			return nullptr;
		}
		
		// Build runtime animation
		ozz::animation::offline::AnimationBuilder builder;
		auto unique_animation = builder(raw_animation);
		
		// Release the unique_ptr to get raw pointer
		// Note: caller is responsible for memory management
		return unique_animation.release();
	}
}
