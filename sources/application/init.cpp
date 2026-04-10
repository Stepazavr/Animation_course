#include "scene.h"
#include "engine/animation/ozz_converter.h"

static glm::mat4 get_projective_matrix()
{
  const float fovY = 90.f * DegToRad;
  const float zNear = 0.01f;
  const float zFar = 500.f;
  return glm::perspective(fovY, engine::get_aspect_ratio(), zNear, zFar);
}

// Helper function to initialize a character with model and animation
void initialize_character(
    Scene& scene,
    const char* characterName,
    const char* modelPath,
    const char* animationPath,
    const char* texturePath,
    const glm::mat4& transform
) {
    auto material = make_material("character", "sources/shaders/character_vs.glsl", "sources/shaders/character_ps.glsl");
    material->set_property("mainTex", create_texture2d(texturePath));
    
    Character character;
    character.name = characterName;
    character.transform = transform;
    character.material = std::move(material);
    
    ModelAsset model = load_model(modelPath, character);
    character.meshes = model.meshes;
    
    load_animation(animationPath, character);
    
    scene.characters.push_back(character);
    scene.models.push_back(std::move(model));
}

void application_init(Scene &scene)
{
  scene.light.lightDirection = glm::normalize(glm::vec3(-1, -1, 0));
  scene.light.lightColor = glm::vec3(1.f);
  scene.light.ambient = glm::vec3(0.2f);
  scene.userCamera.projection = get_projective_matrix();

  engine::onWindowResizedEvent += [&](const std::pair<int, int> &) { scene.userCamera.projection = get_projective_matrix(); };

  ArcballCamera &cam = scene.userCamera.arcballCamera;
  cam.curZoom = cam.targetZoom = 0.5f;
  cam.maxdistance = 5.f;
  cam.distance = cam.curZoom * cam.maxdistance;
  cam.lerpStrength = 10.f;
  cam.mouseSensitivity = 0.5f;
  cam.wheelSensitivity = 0.05f;
  cam.targetPosition = glm::vec3(0.f, 1.f, 0.f);
  cam.targetRotation = cam.curRotation = glm::vec2(DegToRad * -90.f, DegToRad * -30.f);
  cam.rotationEnable = false;

  scene.userCamera.transform = calculate_transform(scene.userCamera.arcballCamera);

  engine::onMouseButtonEvent += [&](const SDL_MouseButtonEvent &e) { arccam_mouse_click_handler(e, scene.userCamera.arcballCamera); };
  engine::onMouseMotionEvent += [&](const SDL_MouseMotionEvent &e) { arccam_mouse_move_handler(e, scene.userCamera.arcballCamera, scene.userCamera.transform); };
  engine::onMouseWheelEvent += [&](const SDL_MouseWheelEvent &e) { arccam_mouse_wheel_handler(e, scene.userCamera.arcballCamera); };

  engine::onKeyboardEvent += [](const SDL_KeyboardEvent &e) { if (e.keysym.sym == SDLK_F5 && e.state == SDL_RELEASED) recompile_all_shaders(); };

  // Initialize MotusMan character
  initialize_character(
      scene,
      "MotusMan_v55",
      "resources/MotusMan_v55/MotusMan_v55.fbx",
      "resources/Animations/IPC/MOB1_Walk_F_Loop_IPC.fbx",
      "resources/MotusMan_v55/MCG_diff.jpg",
      glm::identity<glm::mat4>()
  );

  // Initialize Ruby character
  initialize_character(
      scene,
      "ruby",
      "resources/sketchfab/ruby.fbx",
      "resources/sketchfab/ruby.fbx",
      "resources/sketchfab/color.png",
      glm::translate(glm::mat4(1.f), glm::vec3(1, 0, 0))
  );

  std::fflush(stdout);
}