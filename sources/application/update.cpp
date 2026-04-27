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

void update_animations(Scene& scene, float dt) {
	const float DISCRETE_FRAME_TIME = 1.0f / 30.0f; // 30 fps for discrete keyframes
	
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
		// WASD + Shift -> Jog (index 3)
		// WASD only -> Walk (index 2)
		// Nothing -> Idle (index 1)
		
		bool w_pressed = keyboard_state[SDL_SCANCODE_W];
		bool a_pressed = keyboard_state[SDL_SCANCODE_A];
		bool s_pressed = keyboard_state[SDL_SCANCODE_S];
		bool d_pressed = keyboard_state[SDL_SCANCODE_D];
		bool shift_pressed = keyboard_state[SDL_SCANCODE_LSHIFT] || keyboard_state[SDL_SCANCODE_RSHIFT];
		
		bool anyMovementKeyPressed = w_pressed || a_pressed || s_pressed || d_pressed;
		
		int target_animation = 1; // Default to Idle (index 1)
		
		if (anyMovementKeyPressed) {
			if (shift_pressed) {
				// WASD + Shift -> Jog
				target_animation = 3;
			} else {
				// WASD only -> Walk
				target_animation = 2;
			}
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
		
		// Blending logic: if blending is enabled and character is blending
		if (g_blending_enabled && character.is_blending && !scene.use_t_pose && character.ozz_skeleton && character.animation_states.size() >= 4) {
			// We have at least 4 animations: Index 0: Rest Pose, Index 1: Idle, Index 2: Walk, Index 3: Jog
			
			// All three animations are sampled with the same time ratio (synchronized playback)
			// Idle, Walk, and Jog animations need to be sampled and blended
			
			// First, sample all three animations (indices 1, 2, 3)
			for (int i = 1; i <= 3; ++i) {
				if (i < character.animation_states.size()) {
					auto* anim_state = &character.animation_states[i];
					
					if (anim_state->animation && character.ozz_skeleton) {
						// Updates animation time.
						anim_state->animation_time += dt;
						if (anim_state->animation_time > anim_state->animation->duration()) {
							anim_state->animation_time = fmod(anim_state->animation_time, anim_state->animation->duration());
						}
						
						// Calculate sampling time
						float sampling_time = anim_state->animation_time;
						if (!g_samplingEnabled) {
							sampling_time = floorf(anim_state->animation_time / DISCRETE_FRAME_TIME) * DISCRETE_FRAME_TIME;
						}
						
						// Sample animation
						ozz::animation::SamplingJob sampling_job;
						sampling_job.animation = anim_state->animation;
						sampling_job.context = anim_state->sampling_context;
						sampling_job.ratio = sampling_time / anim_state->animation->duration();
						sampling_job.output = ozz::make_span(anim_state->local_transforms);
						if (!sampling_job.Run())
							continue;
					}
				}
			}
			
			// Ensure blended_locals buffer is properly sized
			if (character.blended_locals.empty() && !character.animation_states[1].local_transforms.empty()) {
				character.blended_locals.resize(character.animation_states[1].local_transforms.size());
			}
			
			if (!character.blended_locals.empty()) {
				// Calculate blend weights based on current_speed
				// Speed range: [MIN_SPEED=0, MAX_SPEED=3] with DIVIDE_SPEED=2
				// Idle-Walk: [0, 2], Walk-Jog: [2, 3]
				
				float weight_idle = 0.f, weight_walk = 0.f, weight_jog = 0.f;
				
				if (character.current_speed < Character::DIVIDE_SPEED) {
					// Blending between Idle and Walk
					float t = character.current_speed / Character::DIVIDE_SPEED;  // [0, 1]
					weight_idle = 1.f - t;
					weight_walk = t;
					weight_jog = 0.f;
				} else {
					// Blending between Walk and Jog
					float t = (character.current_speed - Character::DIVIDE_SPEED) / 
						      (Character::MAX_SPEED - Character::DIVIDE_SPEED);  // [0, 1]
					weight_idle = 0.f;
					weight_walk = 1.f - t;
					weight_jog = t;
				}
				
				// Prepare blending layers
				ozz::animation::BlendingJob::Layer layers[3];
				layers[0].transform = ozz::make_span(character.animation_states[1].local_transforms);  // Idle
				layers[0].weight = weight_idle;
				layers[1].transform = ozz::make_span(character.animation_states[2].local_transforms);  // Walk
				layers[1].weight = weight_walk;
				layers[2].transform = ozz::make_span(character.animation_states[3].local_transforms);  // Jog
				layers[2].weight = weight_jog;
				
				// Setup blending job
				ozz::animation::BlendingJob blend_job;
				blend_job.threshold = ozz::animation::BlendingJob().threshold;  // Use default threshold
				blend_job.layers = layers;
				blend_job.rest_pose = character.ozz_skeleton->joint_rest_poses();
				blend_job.output = ozz::make_span(character.blended_locals);
				
				// Run blending
				if (blend_job.Run()) {
					// Convert from local space to model space matrices using blended result
					ozz::animation::LocalToModelJob ltm_job;
					ltm_job.skeleton = character.ozz_skeleton;
					ltm_job.input = ozz::make_span(character.blended_locals);
					ltm_job.output = ozz::make_span(character.animation_states[pose_index].model_space_matrices);
					
					if (!ltm_job.Run())
						continue;
				}
			}
		} else {
			// Regular sampling without blending
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
}

void application_update(Scene &scene)
{
  float dt = engine::get_delta_time();
  
  // Update animations
  update_animations(scene, dt);
  
  // Update cameras and character rotation for third person mode
  if (scene.use_third_person_camera) {
    // Get keyboard state for WASD input
    int num_keys = 0;
    const uint8_t* keyboard_state = SDL_GetKeyboardState(&num_keys);
    
    bool w_pressed = keyboard_state[SDL_SCANCODE_W];
    bool a_pressed = keyboard_state[SDL_SCANCODE_A];
    bool s_pressed = keyboard_state[SDL_SCANCODE_S];
    bool d_pressed = keyboard_state[SDL_SCANCODE_D];
    
    // Calculate target rotation based on WASD input
    // W = 180°, S = 0°, A = 90°, D = 270°
    // Combinations: W+A=135°, W+D=225°, S+A=45°, S+D=315°
    float targetRotationY = scene.characterRotationY;  // Keep current rotation if no keys pressed
    
    if (w_pressed && !s_pressed) {
      if (a_pressed && !d_pressed) {
        targetRotationY = 135.f;  // W+A
      } else if (d_pressed && !a_pressed) {
        targetRotationY = 225.f;  // W+D
      } else {
        targetRotationY = 180.f;  // W only
      }
    } else if (s_pressed && !w_pressed) {
      if (a_pressed && !d_pressed) {
        targetRotationY = 45.f;   // S+A
      } else if (d_pressed && !a_pressed) {
        targetRotationY = 315.f;  // S+D (or -45)
      } else {
        targetRotationY = 0.f;    // S only
      }
    } else if (a_pressed && !d_pressed) {
      targetRotationY = 90.f;     // A only
    } else if (d_pressed && !a_pressed) {
      targetRotationY = 270.f;    // D only (or -90)
    }
    // If no keys or opposite keys pressed, keep current rotation
    
    // Smooth interpolation to target rotation
    float rotationDelta = targetRotationY - scene.characterRotationY;
    // Handle wraparound (shortest path)
    if (rotationDelta > 180.f) rotationDelta -= 360.f;
    if (rotationDelta < -180.f) rotationDelta += 360.f;
    
    float rotationLerpSpeed = 8.f;  // Rotation speed
    scene.characterRotationY += rotationDelta * glm::min(1.f, rotationLerpSpeed * dt);
    
    // Normalize rotation to 0-360 range
    if (scene.characterRotationY < 0.f) scene.characterRotationY += 360.f;
    if (scene.characterRotationY >= 360.f) scene.characterRotationY -= 360.f;
    
    if (!scene.characters.empty()) {
      auto& character = scene.characters[0];
      
      // Extract position from character transform
      glm::vec3 characterPos = glm::vec3(character.transform[3]);
      
      // Calculate target speed based on WASD input
      // Movement is always forward in local character space (local +Z)
      bool anyKeyPressed = w_pressed || a_pressed || s_pressed || d_pressed;
      bool shift_pressed = keyboard_state[SDL_SCANCODE_LSHIFT] || keyboard_state[SDL_SCANCODE_RSHIFT];
      
      float targetSpeed = Character::speed_idle;  // Default: idle
      if (anyKeyPressed) {
        if (shift_pressed) {
          targetSpeed = Character::speed_wasd_shift;  // WASD + Shift
        } else {
          targetSpeed = Character::speed_wasd;  // WASD only
        }
      }
      
      // Interpolate current speed to target speed
      float speedDifference = targetSpeed - character.current_speed;

			character.is_blending = speedDifference != 0.f;
      
      if (speedDifference > 0.f) {
        if (targetSpeed == Character::speed_wasd) {
        } else if (targetSpeed == Character::speed_wasd_shift) {
        }
      } else if (speedDifference < 0.f) {
        if (targetSpeed == Character::speed_wasd) {
				} else if (targetSpeed == Character::speed_idle) {
        } else if (targetSpeed == Character::speed_idle) {
        }
		  }
      
      if (speedDifference != 0.f) {
        float speedChangePerSecond = 2.0f;
				speedChangePerSecond = (speedDifference > 0.f) ? speedChangePerSecond : -speedChangePerSecond; 
        character.current_speed += speedChangePerSecond * dt;

				if (glm::abs(character.current_speed - targetSpeed) < 0.1f) {
					character.current_speed = targetSpeed; // Snap to target if close enough
					character.is_blending = false;
				}
			} else {
				character.current_speed = targetSpeed;
				character.is_blending = false;
      }


      
      if (anyKeyPressed) {
        // Movement is always forward in local space (rotate to world space)
        glm::vec3 localMovement = glm::vec3(0.f, 0.f, 1.f);  // Forward in local space
        
        // Transform local movement to world space using character's Y rotation
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(scene.characterRotationY), glm::vec3(0, 1, 0));
        glm::vec3 worldMovement = glm::vec3(rotationMatrix * glm::vec4(localMovement, 0.f));
        
        // Apply movement with current speed
        characterPos += worldMovement * character.current_speed * dt;
      }
      
      // Apply rotation based on WASD input
      glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), glm::radians(scene.characterRotationY), glm::vec3(0, 1, 0));
      character.transform = glm::translate(glm::mat4(1.0f), characterPos) * rotationY;
      
      // Update third person camera to follow character
      third_person_camera_update(
        scene.thirdPersonController,
        scene.userCamera.transform,
        characterPos,
        scene.characterRotationY,
        dt);
    }
  } else {
    // Reset character rotation to normal when not in third person mode
    if (!scene.characters.empty()) {
      glm::vec3 characterPos = glm::vec3(scene.characters[0].transform[3]);
      scene.characters[0].transform = glm::translate(glm::mat4(1.0f), characterPos);
      scene.characterRotationY = 0.f;
    }
    // Update arcball camera
    arcball_camera_update(scene.userCamera.arcballCamera, scene.userCamera.transform, dt);
  }
}