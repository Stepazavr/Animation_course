#pragma once

#include "engine/import/model.h"
#include <assimp/scene.h>
#include <ozz/animation/runtime/skeleton.h>

struct Skeleton;

namespace ozz {
	namespace animation {
		class Skeleton;
		class Animation;
	}
}

namespace ozz_converter {
	ozz::animation::Skeleton* convert_skeleton(const Skeleton& engine_skeleton);
	ozz::animation::Animation* convert_animation(const aiAnimation* anim, const ozz::animation::Skeleton& ozz_skeleton);
}
