#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"

#include "scene.h"

static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);

static void show_info()
{
  if (ImGui::Begin("Info"))
  {
    ImGui::Text("ESC - exit");
    ImGui::Text("F5 - recompile shaders");
    ImGui::Text("Left Mouse Button and Wheel - controll camera");
  }
  ImGui::End();
}

static void manipulate_character(Character &character, const UserCamera &camera)
{
  ImGuizmo::BeginFrame();
  const glm::mat4 &projection = camera.projection;
  const glm::mat4 &transform = camera.transform;
  mat4 cameraView = inverse(transform);
  ImGuiIO &io = ImGui::GetIO();
  ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

  glm::mat4 globNodeTm = character.transform;

  ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(projection), mCurrentGizmoOperation, mCurrentGizmoMode,
                       glm::value_ptr(globNodeTm));

  character.transform = globNodeTm;
}

static void show_characters(Scene &scene)
{
  if (ImGui::Begin("Scene"))
  {
    // implement showing characters when only one character can be selected
    static uint32_t selectedCharacter = -1u;
    for (size_t i = 0; i < scene.characters.size(); i++)
    {
      const Character &character = scene.characters[i];
      ImGui::PushID(i);
      if (ImGui::Selectable(character.name.c_str(), selectedCharacter == i, ImGuiSelectableFlags_AllowDoubleClick))
      {
        selectedCharacter = i;
        if (ImGui::IsMouseDoubleClicked(0))
        {
          scene.userCamera.arcballCamera.targetPosition = vec3(character.transform[3]) + vec3(0, 1, 0);
        }
      }
      if (selectedCharacter == i)
      {
        const float INDENT = 15.0f;
        ImGui::Indent(INDENT);
        ImGui::Text("Meshes: %zu", character.meshes.size());
        ImGui::Unindent(INDENT);
      }
      ImGui::PopID();
    }
    if (selectedCharacter < scene.characters.size())
    {
      Character &character = scene.characters[selectedCharacter];
      manipulate_character(character, scene.userCamera);
    }
  }
  ImGui::End();
}

void render_imguizmo(ImGuizmo::OPERATION &mCurrentGizmoOperation, ImGuizmo::MODE &mCurrentGizmoMode)
{
  if (ImGui::Begin("gizmo window"))
  {
    if (ImGui::IsKeyPressed('Z'))
      mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed('E'))
      mCurrentGizmoOperation = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed('R')) // r Key
      mCurrentGizmoOperation = ImGuizmo::SCALE;
    if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
      mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
      mCurrentGizmoOperation = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
      mCurrentGizmoOperation = ImGuizmo::SCALE;

    if (mCurrentGizmoOperation != ImGuizmo::SCALE)
    {
      if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
        mCurrentGizmoMode = ImGuizmo::LOCAL;
      ImGui::SameLine();
      if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
        mCurrentGizmoMode = ImGuizmo::WORLD;
    }
  }
  ImGui::End();
}

static void show_models(Scene &scene)
{
  if (ImGui::Begin("Models"))
  {
    static uint32_t selectedModel = -1u;
    for (size_t i = 0; i < scene.models.size(); i++)
    {
      const ModelAsset &model = scene.models[i];
      if (ImGui::Selectable(model.path.c_str(), selectedModel == i))
      {
        selectedModel = i;
      }
      if (selectedModel == i)
      {
        ImGui::Indent(15.0f);
        ImGui::Text("Path: %s", model.path.c_str());
        ImGui::Text("Meshes: %zu", model.meshes.size());
        for (size_t j = 0; j < model.meshes.size(); j++)
        {
          const MeshPtr &mesh = model.meshes[j];
          ImGui::Text("%s", mesh->name.c_str());
        }
        ImGui::Unindent(15.0f);
      }
    }
  }
  ImGui::End();
}

void application_imgui_render(Scene &scene)
{
  render_imguizmo(mCurrentGizmoOperation, mCurrentGizmoMode);

  show_info();
  show_characters(scene);
  show_models(scene);
}