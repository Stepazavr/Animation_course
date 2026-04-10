#include "engine/event.h"
#include "application/scene.h"
#include "application/user_camera.h"
#include "application/arcball_camera.h"
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>

void update_animations(Scene& scene, float dt) {
	const float DISCRETE_FRAME_TIME = 1.0f / 10.0f; // x fps for discrete keyframes
	
	for (auto& character : scene.characters) {
		auto* anim_state = character.get_current_animation_state();
		if (anim_state && anim_state->animation && character.ozz_skeleton) {
			// Updates animation time.
			anim_state->animation_time += dt;
			if (anim_state->animation_time > anim_state->animation->duration()) {
				anim_state->animation_time = fmod(anim_state->animation_time, anim_state->animation->duration());
			}
			
			// Calculate sampling time: continuous or discrete based on g_samplingEnabled
			float sampling_time = anim_state->animation_time;
			if (!g_samplingEnabled) {
				// Discretize to nearest keyframe (no interpolation)
				sampling_time = floorf(anim_state->animation_time / DISCRETE_FRAME_TIME) * DISCRETE_FRAME_TIME;
			}
			
			// Samples animation at sampling_time.
			ozz::animation::SamplingJob sampling_job;
			sampling_job.animation = anim_state->animation;
			sampling_job.context = anim_state->sampling_context;
			sampling_job.ratio = sampling_time / anim_state->animation->duration();
			sampling_job.output = ozz::make_span(anim_state->local_transforms);
			if (!sampling_job.Run())
				continue;

			// Converts from local space to model space matrices.
			ozz::animation::LocalToModelJob ltm_job;
			ltm_job.skeleton = character.ozz_skeleton;
			ltm_job.input = ozz::make_span(anim_state->local_transforms);
			ltm_job.output = ozz::make_span(anim_state->model_space_matrices);
			if (!ltm_job.Run())
				continue;		
			// Skeleton is now updated with current animation pose
			// anim_state->model_space_matrices contains the animated joint transforms
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