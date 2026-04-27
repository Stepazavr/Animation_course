#include "engine/event.h"
#include "application/scene.h"
#include "application/user_camera.h"
#include "application/arcball_camera.h"
#include "application/third_person_controller.h"
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/blending_job.h>
#include <SDL2/SDL.h>

extern bool g_blending_enabled;
extern bool g_samplingEnabled;

// Animation state order (note: Rest Pose at index 0 is created in load_model):
// Index 0: Rest Pose
// Index 1: Idle
// Index 2: Walk
// Index 3: Jog
//
// Keyboard controls:
// WASD + Shift -> Jog (index 3)
// WASD only -> Walk (index 2)
// Nothing -> Idle (index 1)

static void reset_animation_time(Character& character, int start_idx = 1, int end_idx = 3) {
	for (int i = start_idx; i <= end_idx; ++i) {
		if (i >= character.animation_states.size())
			continue;
		character.animation_states[i].animation_time = 0.f;
	}
}

static void update_animation_index_from_keyboard(Scene& scene) {
	if (scene.use_t_pose || scene.characters.empty())
		return;

	int num_keys = 0;
	const uint8_t* keyboard_state = SDL_GetKeyboardState(&num_keys);
	
	auto& character = scene.characters[0];
	
	bool w_pressed = keyboard_state[SDL_SCANCODE_W];
	bool a_pressed = keyboard_state[SDL_SCANCODE_A];
	bool s_pressed = keyboard_state[SDL_SCANCODE_S];
	bool d_pressed = keyboard_state[SDL_SCANCODE_D];
	bool shift_pressed = keyboard_state[SDL_SCANCODE_LSHIFT] || keyboard_state[SDL_SCANCODE_RSHIFT];
	
	bool anyMovementKeyPressed = w_pressed || a_pressed || s_pressed || d_pressed;
	
	int target_animation = 1;
	if (anyMovementKeyPressed) {
		target_animation = shift_pressed ? 3 : 2;
	}
	
	if (character.current_animation_index != target_animation && target_animation < character.animation_states.size()) {
		character.current_animation_index = target_animation;
		auto* anim_state = character.get_current_animation_state();
		if (anim_state) {
			anim_state->animation_time = 0.f;
		}
	}
}

static void update_blending_parameters(Character& character, int& inv_loop_duration) {
	float blend_ratio = (character.current_speed - Character::MIN_SPEED) / 
		               (Character::MAX_SPEED - Character::MIN_SPEED);
	blend_ratio = glm::clamp(blend_ratio, 0.f, 1.f);
	
	const float kNumIntervals = 2.f;
	const float kInterval = 1.f / kNumIntervals;
	
	float weight_idle = 0.f, weight_walk = 0.f, weight_jog = 0.f;
	for (int i = 0; i < 3; ++i) {
		const float med = i * kInterval;
		const float x = blend_ratio - med;
		const float y = ((x < 0.f ? x : -x) + kInterval) * kNumIntervals;
		float weight = glm::max(0.f, y);
		
		if (i == 0) weight_idle = weight;
		else if (i == 1) weight_walk = weight;
		else if (i == 2) weight_jog = weight;
	}
	
	const int relevant_sampler = static_cast<int>((blend_ratio - 1e-3f) * kNumIntervals);
	int sampler_l_idx = glm::min(1 + relevant_sampler, 2);
	int sampler_r_idx = glm::min(sampler_l_idx + 1, 3);
	
	float weight_l = (relevant_sampler == 0) ? weight_idle : weight_walk;
	float weight_r = (relevant_sampler == 0) ? weight_walk : weight_jog;
	
	float duration_l = character.animation_states[sampler_l_idx].animation ? 
		character.animation_states[sampler_l_idx].animation->duration() : 1.f;
	float duration_r = character.animation_states[sampler_r_idx].animation ? 
		character.animation_states[sampler_r_idx].animation->duration() : 1.f;
	
	const float loop_duration = duration_l * weight_l + duration_r * weight_r;
	inv_loop_duration = (loop_duration > 0.f) ? (1.f / loop_duration) : 1.f;
	
	character.animation_states[1].animation_time = fmod(character.animation_states[1].animation_time, 
		character.animation_states[1].animation ? character.animation_states[1].animation->duration() : 1.f);
	character.animation_states[2].animation_time = fmod(character.animation_states[2].animation_time, 
		character.animation_states[2].animation ? character.animation_states[2].animation->duration() : 1.f);
	character.animation_states[3].animation_time = fmod(character.animation_states[3].animation_time, 
		character.animation_states[3].animation ? character.animation_states[3].animation->duration() : 1.f);
}

static void sample_animations(Character& character, float dt, int inv_loop_duration, int start_idx = 1, int end_idx = 3) {
	const float DISCRETE_FRAME_TIME = 1.0f / 30.0f;
	
	for (int i = start_idx; i <= end_idx; ++i) {
		if (i >= character.animation_states.size())
			continue;
			
		auto* anim_state = &character.animation_states[i];
		if (!anim_state->animation || !character.ozz_skeleton)
			continue;
		

		float sync_factor = character.animation_states[i].animation->duration() * inv_loop_duration;
		sync_factor = (inv_loop_duration != -1.f) ? sync_factor : 1.f;
		anim_state->animation_time += dt * sync_factor;
		if (anim_state->animation_time > anim_state->animation->duration()) {
			anim_state->animation_time = fmod(anim_state->animation_time, anim_state->animation->duration());
		}
		
		float sampling_time = anim_state->animation_time;
		if (!g_samplingEnabled) {
			sampling_time = floorf(anim_state->animation_time / DISCRETE_FRAME_TIME) * DISCRETE_FRAME_TIME;
		}
		
		ozz::animation::SamplingJob sampling_job;
		sampling_job.animation = anim_state->animation;
		sampling_job.context = anim_state->sampling_context;
		sampling_job.ratio = sampling_time / anim_state->animation->duration();
		sampling_job.output = ozz::make_span(anim_state->local_transforms);
		sampling_job.Run();
	}
}

static void blend_animations(Character& character) {
	if (character.blended_locals.empty() && !character.animation_states[1].local_transforms.empty()) {
		character.blended_locals.resize(character.animation_states[1].local_transforms.size());
	}
	
	if (!character.blended_locals.empty()) {
		float blend_ratio = (character.current_speed - Character::MIN_SPEED) / 
			               (Character::MAX_SPEED - Character::MIN_SPEED);
		blend_ratio = glm::clamp(blend_ratio, 0.f, 1.f);
		
		const float kNumIntervals = 2.f;
		const float kInterval = 1.f / kNumIntervals;
		
		float weight_idle = 0.f, weight_walk = 0.f, weight_jog = 0.f;
		for (int i = 0; i < 3; ++i) {
			const float med = i * kInterval;
			const float x = blend_ratio - med;
			const float y = ((x < 0.f ? x : -x) + kInterval) * kNumIntervals;
			float weight = glm::max(0.f, y);
			
			if (i == 0) weight_idle = weight;
			else if (i == 1) weight_walk = weight;
			else if (i == 2) weight_jog = weight;
		}
		
		ozz::animation::BlendingJob::Layer layers[3];
		layers[0].transform = ozz::make_span(character.animation_states[1].local_transforms);
		layers[0].weight = weight_idle;
		layers[1].transform = ozz::make_span(character.animation_states[2].local_transforms);
		layers[1].weight = weight_walk;
		layers[2].transform = ozz::make_span(character.animation_states[3].local_transforms);
		layers[2].weight = weight_jog;
		
		ozz::animation::BlendingJob blend_job;
		blend_job.threshold = ozz::animation::BlendingJob().threshold;
		blend_job.layers = layers;
		blend_job.rest_pose = character.ozz_skeleton->joint_rest_poses();
		blend_job.output = ozz::make_span(character.blended_locals);
		
		if (blend_job.Run()) {
			ozz::animation::LocalToModelJob ltm_job;
			ltm_job.skeleton = character.ozz_skeleton;
			ltm_job.input = ozz::make_span(character.blended_locals);
			int pose_index = character.current_animation_index;
			ltm_job.output = ozz::make_span(character.animation_states[pose_index].model_space_matrices);
			ltm_job.Run();
		}
	}
}

void update_animations(Scene& scene, float dt) {
	update_animation_index_from_keyboard(scene);
	
	for (auto& character : scene.characters) {
		int pose_index = scene.use_t_pose ? 0 : character.current_animation_index;
		bool should_blend = g_blending_enabled && character.is_blending && !scene.use_t_pose && 
			character.ozz_skeleton && character.animation_states.size() >= 4;
		
		// Reset animation time when blending ends
		if (character.was_blending_previous_frame && !should_blend) {
			reset_animation_time(character, pose_index, pose_index);
			character.was_blending_previous_frame = false;
		}
		
		// Mark blending state for next frame
		if (should_blend) {
			character.was_blending_previous_frame = true;
		}

		int inv_loop_duration = -1.f;
		if (should_blend) {
			update_blending_parameters(character, inv_loop_duration);
			sample_animations(character, dt, inv_loop_duration);
			blend_animations(character);
		} else {
			sample_animations(character, dt, inv_loop_duration, pose_index, pose_index);
			
			if (pose_index >= 0 && pose_index < character.animation_states.size()) {
				auto* anim_state = &character.animation_states[pose_index];
				if (anim_state->animation && character.ozz_skeleton) {
					ozz::animation::LocalToModelJob ltm_job;
					ltm_job.skeleton = character.ozz_skeleton;
					ltm_job.input = ozz::make_span(anim_state->local_transforms);
					ltm_job.output = ozz::make_span(anim_state->model_space_matrices);
					ltm_job.Run();
				}
			}
		}
	}
}

void application_update(Scene &scene)
{
  float dt = engine::get_delta_time();
  update_animations(scene, dt);
  
  if (scene.use_third_person_camera) {
    int num_keys = 0;
    const uint8_t* keyboard_state = SDL_GetKeyboardState(&num_keys);
    
    bool w_pressed = keyboard_state[SDL_SCANCODE_W];
    bool a_pressed = keyboard_state[SDL_SCANCODE_A];
    bool s_pressed = keyboard_state[SDL_SCANCODE_S];
    bool d_pressed = keyboard_state[SDL_SCANCODE_D];
    
    float targetRotationY = scene.characterRotationY;
    
    if (w_pressed && !s_pressed) {
      if (a_pressed && !d_pressed) {
        targetRotationY = 135.f;
      } else if (d_pressed && !a_pressed) {
        targetRotationY = 225.f;
      } else {
        targetRotationY = 180.f;
      }
    } else if (s_pressed && !w_pressed) {
      if (a_pressed && !d_pressed) {
        targetRotationY = 45.f;
      } else if (d_pressed && !a_pressed) {
        targetRotationY = 315.f;
      } else {
        targetRotationY = 0.f;
      }
    } else if (a_pressed && !d_pressed) {
      targetRotationY = 90.f;
    } else if (d_pressed && !a_pressed) {
      targetRotationY = 270.f;
    }
    
    float rotationDelta = targetRotationY - scene.characterRotationY;
    if (rotationDelta > 180.f) rotationDelta -= 360.f;
    if (rotationDelta < -180.f) rotationDelta += 360.f;
    
    float rotationLerpSpeed = 8.f;
    scene.characterRotationY += rotationDelta * glm::min(1.f, rotationLerpSpeed * dt);
    
    if (scene.characterRotationY < 0.f) scene.characterRotationY += 360.f;
    if (scene.characterRotationY >= 360.f) scene.characterRotationY -= 360.f;
    
    if (!scene.characters.empty()) {
      auto& character = scene.characters[0];
      glm::vec3 characterPos = glm::vec3(character.transform[3]);
      
      bool anyKeyPressed = w_pressed || a_pressed || s_pressed || d_pressed;
      bool shift_pressed = keyboard_state[SDL_SCANCODE_LSHIFT] || keyboard_state[SDL_SCANCODE_RSHIFT];
      
      float targetSpeed = Character::speed_idle;
      if (anyKeyPressed) {
        targetSpeed = shift_pressed ? Character::speed_wasd_shift : Character::speed_wasd;
      }
      
      float speedDifference = targetSpeed - character.current_speed;
      character.is_blending = speedDifference != 0.f;
      
      if (speedDifference != 0.f) {
        float speedChangePerSecond = (speedDifference > 0.f) ? character.speedChangePerSecond : -character.speedChangePerSecond; 
        character.current_speed += speedChangePerSecond * dt;

        if (glm::abs(character.current_speed - targetSpeed) < 0.01f) {
          character.current_speed = targetSpeed;
          character.is_blending = false;
        }
      } else {
        character.current_speed = targetSpeed;
        character.is_blending = false;
      }
      
      if (anyKeyPressed) {
        glm::vec3 localMovement = glm::vec3(0.f, 0.f, 1.f);
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(scene.characterRotationY), glm::vec3(0, 1, 0));
        glm::vec3 worldMovement = glm::vec3(rotationMatrix * glm::vec4(localMovement, 0.f));
        characterPos += worldMovement * character.current_speed * dt;
      }
      
      glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), glm::radians(scene.characterRotationY), glm::vec3(0, 1, 0));
      character.transform = glm::translate(glm::mat4(1.0f), characterPos) * rotationY;
      
      third_person_camera_update(
        scene.thirdPersonController,
        scene.userCamera.transform,
        characterPos,
        scene.characterRotationY,
        dt);
    }
  } else {
    if (!scene.characters.empty()) {
      glm::vec3 characterPos = glm::vec3(scene.characters[0].transform[3]);
      scene.characters[0].transform = glm::translate(glm::mat4(1.0f), characterPos);
      scene.characterRotationY = 0.f;
    }
    arcball_camera_update(scene.userCamera.arcballCamera, scene.userCamera.transform, dt);
  }
}