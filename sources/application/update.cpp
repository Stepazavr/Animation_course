#include "engine/event.h"
#include "application/scene.h"
#include "application/user_camera.h"
#include "application/arcball_camera.h"
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>

void update_animations(Scene& scene, float dt) {
	for (auto& character : scene.characters) {
		if (character.ozz_animation && character.ozz_skeleton && g_samplingEnabled) {
			// Updates animation time.
			character.animation_time += dt;
			if (character.animation_time > character.ozz_animation->duration()) {
				character.animation_time = fmod(character.animation_time, character.ozz_animation->duration());
			}
			// Samples animation at t = animation_time.
			ozz::animation::SamplingJob sampling_job;
			sampling_job.animation = character.ozz_animation;
			sampling_job.context = character.sampling_context;
			sampling_job.ratio = character.animation_time / character.ozz_animation->duration();
			sampling_job.output = ozz::make_span(character.local_transforms);
			if (!sampling_job.Run()) {
				continue;
			}

			// Converts from local space to model space matrices.
			ozz::animation::LocalToModelJob ltm_job;
			ltm_job.skeleton = character.ozz_skeleton;
			ltm_job.input = ozz::make_span(character.local_transforms);
			ltm_job.output = ozz::make_span(character.model_space_matrices);
			if (!ltm_job.Run()) {
				continue;
			}		
			// Skeleton is now updated with current animation pose
			// character.model_space_matrices contains the animated joint transforms
		}
	}
}

void application_update(Scene &scene)
{
  float dt = engine::get_delta_time();
  
  // Update arcball camera
  arcball_camera_update(scene.userCamera.arcballCamera, scene.userCamera.transform, dt);
  
  // Update animations
  update_animations(scene, dt);
}