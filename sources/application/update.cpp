#include "engine/event.h"
#include "application/scene.h"
#include "application/user_camera.h"
#include "application/arcball_camera.h"
#include "application/third_person_controller.h"
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <SDL2/SDL.h>

void update_animations(Scene& scene, float dt) {
	const float DISCRETE_FRAME_TIME = 1.0f / 10.0f; // x fps for discrete keyframes
	
	// Handle keyboard input for animation selection (on first character if not in t-pose mode)
	if (!scene.use_t_pose && !scene.characters.empty()) {
		int num_keys = 0;
		const uint8_t* keyboard_state = SDL_GetKeyboardState(&num_keys);
		
		auto& character = scene.characters[0];
		
		// Animation state order (note: Rest Pose at index 0 is created in load_model):
		// Index 0: Rest Pose
		// Index 1: Idle
		// Index 2: Walk
		// Index 3: Jog
		//
		// Keyboard controls:
		// W + Shift -> Jog (index 3)
		// W only -> Walk (index 2)
		// Nothing -> Idle (index 1)
		
		bool w_pressed = keyboard_state[SDL_SCANCODE_W];
		bool shift_pressed = keyboard_state[SDL_SCANCODE_LSHIFT] || keyboard_state[SDL_SCANCODE_RSHIFT];
		
		int target_animation = 1; // Default to Idle (index 1)
		
		if (w_pressed && shift_pressed) {
			// W + Shift -> Jog
			target_animation = 3;
		} else if (w_pressed) {
			// W only -> Walk
			target_animation = 2;
		}
		// else target_animation stays 1 (Idle)
		
		// Update animation if changed
		if (character.current_animation_index != target_animation) {
			if (target_animation < character.animation_states.size()) {
				character.current_animation_index = target_animation;
				auto* anim_state = character.get_current_animation_state();
				if (anim_state) {
					anim_state->animation_time = 0.f;
				}
			}
		}
	}
	
	for (auto& character : scene.characters) {
		// In t-pose mode, use rest pose (index 0), otherwise use current_animation_index
		int pose_index = scene.use_t_pose ? 0 : character.current_animation_index;
		
		if (pose_index >= 0 && pose_index < character.animation_states.size()) {
			auto* anim_state = &character.animation_states[pose_index];
			
			if (anim_state->animation && character.ozz_skeleton) {
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
}

void application_update(Scene &scene)
{
  float dt = engine::get_delta_time();
  
  // Update animations
  update_animations(scene, dt);
  
  // Update cameras and character rotation for third person mode
  if (scene.use_third_person_camera) {
    if (!scene.characters.empty()) {
      // Extract position from character transform
      glm::vec3 characterPos = glm::vec3(scene.characters[0].transform[3]);
      
      // Rotate character 180 degrees around Y axis for third person camera
      glm::mat4 rotation180Y = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0, 1, 0));
      scene.characters[0].transform = glm::translate(glm::mat4(1.0f), characterPos) * rotation180Y;
      
      // Update third person controller to follow character
      scene.thirdPersonController.targetPosition = characterPos;
    }
    third_person_controller_update(scene.thirdPersonController, scene.userCamera.transform, dt);
  } else {
    // Reset character rotation to normal when not in third person mode
    if (!scene.characters.empty()) {
      glm::vec3 characterPos = glm::vec3(scene.characters[0].transform[3]);
      scene.characters[0].transform = glm::translate(glm::mat4(1.0f), characterPos);
    }
    // Update arcball camera
    arcball_camera_update(scene.userCamera.arcballCamera, scene.userCamera.transform, dt);
  }
}