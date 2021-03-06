#include "lmpch.h"
#include "Editor.h"
#include "SceneWindow.h"
#include "ProfilerWindow.h"
#include "ConsoleWindow.h"
#include "HierarchyWindow.h"
#include "InspectorWindow.h"
#include "ApplicationInfoWindow.h"
#include "GraphicsInfoWindow.h"
#include "TextEditWindow.h"
#include "AssetWindow.h"
#include "EditorCamera.h"

#include "App/Application.h"
#include "Core/OS/Input.h"
#include "Core/OS/FileSystem.h"
#include "Core/Profiler.h"
#include "Core/Version.h"
#include "App/Engine.h"
#include "App/Scene.h"
#include "App/SceneManager.h"
#include "Events/ApplicationEvent.h"

#include "ECS/Component/Components.h"
#include "Scripting/ScriptComponent.h"

#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Graphics/Renderers/ForwardRenderer.h"
#include "Graphics/MeshFactory.h"
#include "Graphics/Layers/Layer3D.h"
#include "Graphics/Sprite.h"
#include "Graphics/Light.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Layers/LayerStack.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/MeshFactory.h"
#include "Graphics/Renderers/GridRenderer.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Graphics/ModelLoader/ModelLoader.h"
#include "Graphics/Environment.h"

#include "Utilities/AssetsManager.h"

#include "ImGui/ImGuiHelpers.h"

#include <imgui/imgui_internal.h>
#include <imgui/plugins/ImGuizmo.h>
#include <imgui/plugins/ImGuiAl/button/imguial_button.h>
#include <imgui/plugins/ImTextEditor.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <IconFontCppHeaders/IconsFontAwesome5Brands.h>

#include <imgui/plugins/ImFileBrowser.h>

static ImVec2 operator+(const ImVec2 &a, const ImVec2 &b) {
	return ImVec2(a.x + b.x, a.y + b.y);
}

namespace Lumos
{
	Editor::Editor(Application* app, u32 width, u32 height) : m_Application(app), m_Selected()
	{
	}

	Editor::~Editor()
	{
        //put back when stop overriding with a scene camera
		//delete m_EditorCamera;
	}

	void Editor::OnInit()
	{
#if 0
		const char *ini[] = { "editor.ini", "editor/editor.ini", "../editor/editor.ini" };
		bool fileFound = false;
		for (int i = 0; i < IM_ARRAYSIZE(ini); ++i) 
		{
			auto fexist = [](const char *f) -> bool 
			{
				FILE *fp = fopen(f, "rb");
                return fp ? (static_cast<void>(fclose(fp)), 1) : 0;
			};
			if (fexist(ini[i]))
			{
				ImGui::GetIO().IniFilename = ini[i];
				fileFound = true;
				break;
			}
		}

		if (!fileFound)
		{
			FileSystem::WriteFile("editor.ini", nullptr);
			ImGui::GetIO().IniFilename = "editor.ini";
		}
#endif	
		m_EditorCamera = new Camera(-20.0f, -40.0f, Maths::Vector3(-31.0f, 12.0f, 51.0f), 60.0f, 0.1f, 1000.0f, (float) m_Application->GetWindowSize().x / (float) m_Application->GetWindowSize().y);
		m_EditorCamera->SetCameraController(CreateRef<EditorCameraController>());
        
        //Todo : use this camera in editor
        delete m_EditorCamera;
        m_EditorCamera = nullptr;

		m_ComponentIconMap[typeid(Graphics::Light).hash_code()] = " " ICON_FA_LIGHTBULB " ";
		m_ComponentIconMap[typeid(Camera).hash_code()] = ICON_FA_CAMERA;
		m_ComponentIconMap[typeid(SoundComponent).hash_code()] = ICON_FA_VOLUME_UP;
		m_ComponentIconMap[typeid(Graphics::Sprite).hash_code()] = ICON_FA_IMAGE;
		m_ComponentIconMap[typeid(Maths::Transform).hash_code()] = ICON_FA_VECTOR_SQUARE;
		m_ComponentIconMap[typeid(Physics2DComponent).hash_code()] = ICON_FA_SQUARE;
		m_ComponentIconMap[typeid(Physics3DComponent).hash_code()] = ICON_FA_CUBE;
		m_ComponentIconMap[typeid(MeshComponent).hash_code()] = ICON_FA_SHAPES;
		m_ComponentIconMap[typeid(MaterialComponent).hash_code()] = ICON_FA_PAINT_BRUSH;
        m_ComponentIconMap[typeid(ScriptComponent).hash_code()] = ICON_FA_SCROLL;
        m_ComponentIconMap[typeid(Graphics::Environment).hash_code()] = ICON_FA_GLOBE;

        m_Windows.emplace_back(CreateRef<ConsoleWindow>());
		m_Windows.emplace_back(CreateRef<SceneWindow>());
		m_Windows.emplace_back(CreateRef<ProfilerWindow>());
		m_Windows.back()->SetActive(false);
		m_Windows.emplace_back(CreateRef<InspectorWindow>());
		m_Windows.emplace_back(CreateRef<HierarchyWindow>());
		m_Windows.emplace_back(CreateRef<GraphicsInfoWindow>());
		m_Windows.back()->SetActive(false);
		m_Windows.emplace_back(CreateRef<ApplicationInfoWindow>());
        //m_Windows.emplace_back(CreateRef<AssetWindow>());


		for (auto& window : m_Windows)
			window->SetEditor(this);
    
        m_DebugDrawFlags = 0;//EditorDebugFlags::MeshBoundingBoxes | EditorDebugFlags::CameraFrustum | EditorDebugFlags::SpriteBoxes;

		m_ShowImGuiDemo = false;

		ImGuiHelpers::SetTheme(ImGuiHelpers::Dark);
    
		m_Selected = entt::null;
    
        m_PreviewTexture = nullptr;
	}
    
    bool IsTextFile(const String& filePath)
    {
        String extension = StringFormat::GetFilePathExtension(filePath);
    
        if(extension == "txt" || extension == "glsl" || extension == "shader" || extension == "vert" || extension == "frag" || extension == "lua" || extension == "Lua")
            return true;
    
        return false;
    }
    
    bool IsAudioFile(const String& filePath)
    {
        String extension = StringFormat::GetFilePathExtension(filePath);
    
        if(extension == "ogg" || extension == "wav")
            return true;
    
        return false;
    }
    
    bool IsModelFile(const String& filePath)
    {
        String extension = StringFormat::GetFilePathExtension(filePath);
    
        if(extension == "obj" || extension == "gltf" || extension == "glb" || extension == "fbx" || extension == "FBX")
            return true;
    
        return false;
    }

	void Editor::OnImGui()
	{
		LUMOS_PROFILE_FUNC;
		DrawMenuBar();

		BeginDockSpace(false);
      
		for (auto& window : m_Windows)
		{
			if (window->Active())
				window->OnImGui();
		}

        if(m_ShowImGuiDemo)
            ImGui::ShowDemoWindow(&m_ShowImGuiDemo);

		m_View2D = m_EditorCamera->IsOrthographic();

		if (m_ShowGrid)
		{
			if (m_3DGridLayer == nullptr)
			{
				m_3DGridLayer = new Layer3D(new Graphics::GridRenderer(u32(Application::Instance()->m_SceneViewWidth), u32(Application::Instance()->m_SceneViewHeight), true), "Grid");
				Application::Instance()->PushLayerInternal(m_3DGridLayer, true, false);
			}
		}
		else if(m_3DGridLayer)
		{
			Application::Instance()->GetLayerStack()->PopOverlay(m_3DGridLayer);
			m_3DGridLayer = nullptr;
		}
        
        ShowPreview();

        m_FileBrowserWindow.OnImGui();

		EndDockSpace();
	}

	void Editor::DrawMenuBar()
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Exit")) { Application::Instance()->SetAppState(AppState::Closing); }
                
                if(ImGui::MenuItem("Open File"))
                {
                    m_FileBrowserWindow.SetCallback(BIND_FILEBROWSER_FN(Editor::FileOpenCallback));
                    m_FileBrowserWindow.Open();
                }

				if (ImGui::BeginMenu("Style"))
				{
					if (ImGui::MenuItem("Dark", "")) { ImGuiHelpers::SetTheme(ImGuiHelpers::Dark); }
					if (ImGui::MenuItem("Black", "")) { ImGuiHelpers::SetTheme(ImGuiHelpers::Black); }
					if (ImGui::MenuItem("Grey", "")) { ImGuiHelpers::SetTheme(ImGuiHelpers::Grey); }
					if (ImGui::MenuItem("Light", "")) { ImGuiHelpers::SetTheme(ImGuiHelpers::Light); }
					if (ImGui::MenuItem("Cherry", "")) { ImGuiHelpers::SetTheme(ImGuiHelpers::Cherry); }
                    if (ImGui::MenuItem("Blue", "")) { ImGuiHelpers::SetTheme(ImGuiHelpers::Blue); }
					if (ImGui::MenuItem("Cinder", "")) { ImGuiHelpers::SetTheme(ImGuiHelpers::Cinder); }
					if (ImGui::MenuItem("Classic", "")) { ImGuiHelpers::SetTheme(ImGuiHelpers::Classic); }
					if (ImGui::MenuItem("ClassicDark", "")) {ImGuiHelpers::SetTheme(ImGuiHelpers::ClassicDark); }
					if (ImGui::MenuItem("ClassicLight", "")) { ImGuiHelpers::SetTheme(ImGuiHelpers::ClassicLight); }
					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
				if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
				ImGui::Separator();
				if (ImGui::MenuItem("Cut", "CTRL+X")) {}
				if (ImGui::MenuItem("Copy", "CTRL+C")) {}
				if (ImGui::MenuItem("Paste", "CTRL+V")) {}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Windows"))
			{
				for (auto& window : m_Windows)
				{
					if (ImGui::MenuItem(window->GetName().c_str(), "", &window->Active(), true)) { window->SetActive(true); }
				}
                
                if (ImGui::MenuItem("ImGui Demo", "", &m_ShowImGuiDemo, true)) { m_ShowImGuiDemo = true; }

				ImGui::EndMenu();
			}
            
            if (ImGui::BeginMenu("Scenes"))
            {
                auto scenes = Application::Instance()->GetSceneManager()->GetSceneNames();
                
                for(size_t i = 0; i < scenes.size(); i++)
                {
                    auto name = scenes[i];
                    if (ImGui::MenuItem(name.c_str()))
                    {
                        Application::Instance()->GetSceneManager()->SwitchScene(name);
                    }
                }
                ImGui::EndMenu();
            }

			if (ImGui::BeginMenu("Entity"))
			{
				auto& registry = m_Application->GetSceneManager()->GetCurrentScene()->GetRegistry();

				if (ImGui::MenuItem("CreateEmpty"))
				{
					registry.create();
				}

				if (ImGui::MenuItem("Cube"))
				{
					auto entity = registry.create();
					registry.emplace<MeshComponent>(entity, Graphics::CreatePrimative(Graphics::PrimitiveType::Cube));
					registry.emplace<NameComponent>(entity, "Cube");
					registry.emplace<Maths::Transform>(entity);
				}

				if (ImGui::MenuItem("Sphere"))
				{
					auto entity = registry.create();
					registry.emplace<MeshComponent>(entity, Graphics::CreatePrimative(Graphics::PrimitiveType::Sphere));
					registry.emplace<NameComponent>(entity, "Sphere");
					registry.emplace<Maths::Transform>(entity);
				}

				if (ImGui::MenuItem("Pyramid"))
				{
					auto entity = registry.create();
					registry.emplace<MeshComponent>(entity, Graphics::CreatePrimative(Graphics::PrimitiveType::Pyramid));
					registry.emplace<NameComponent>(entity, "Pyramid");
					registry.emplace<Maths::Transform>(entity);
				}

				if (ImGui::MenuItem("Plane"))
				{
					auto entity = registry.create();
					registry.emplace<MeshComponent>(entity, Graphics::CreatePrimative(Graphics::PrimitiveType::Plane));
					registry.emplace<NameComponent>(entity, "Plane");
					registry.emplace<Maths::Transform>(entity);
				}
				
				if (ImGui::MenuItem("Cylinder"))
				{
					auto entity = registry.create();
					registry.emplace<MeshComponent>(entity, Graphics::CreatePrimative(Graphics::PrimitiveType::Cylinder));
					registry.emplace<NameComponent>(entity, "Cylinder");
					registry.emplace<Maths::Transform>(entity);
				}

				if (ImGui::MenuItem("Capsule"))
				{
					auto entity = registry.create();
					registry.emplace<MeshComponent>(entity, Graphics::CreatePrimative(Graphics::PrimitiveType::Capsule));
					registry.emplace<NameComponent>(entity, "Capsule");
					registry.emplace<Maths::Transform>(entity);
				}

				ImGui::EndMenu();
			}
        
            if (ImGui::BeginMenu("Graphics"))
            {
                if (ImGui::MenuItem("Compile Shaders")) { RecompileShaders(); }
                ImGui::EndMenu();
            }
        
            if(ImGui::BeginMenu("About"))
            {
                auto& version = Lumos::LumosVersion;
                ImGui::Text("Version : %d.%d.%d", version.major, version.minor, version.patch);
                ImGui::Separator();

                String githubMenuText = ICON_FA_GITHUB" Github";
                if (ImGui::MenuItem(githubMenuText.c_str()))
                {
                    #ifdef LUMOS_PLATFORM_WINDOWS
                    //TODO
                    #else
                    #ifndef LUMOS_PLATFORM_IOS
                    system("open https://www.github.com/jmorton06/Lumos");
                    #endif
                    #endif
                }

                ImGui::EndMenu();
            }
            
            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x / 2.0f);

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.2f, 0.7f, 0.0f));

            bool selected;
            {
                selected = m_Application->GetEditorState() == EditorState::Play;
                if (selected)
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));

                if (ImGui::Button(ICON_FA_PLAY, ImVec2(19.0f, 19.0f)))
                    m_Application->SetEditorState(EditorState::Play);

                ImGuiHelpers::Tooltip("Play");

                if (selected)
                    ImGui::PopStyleColor();
            }

            ImGui::SameLine();

            {
                selected = m_Application->GetEditorState() == EditorState::Paused;
                if (selected)
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));

                if (ImGui::Button(ICON_FA_PAUSE,  ImVec2(19.0f, 19.0f)))
                    m_Application->SetEditorState(EditorState::Paused);

                ImGuiHelpers::Tooltip("Pause");

                if (selected)
                    ImGui::PopStyleColor();
            }

            ImGui::SameLine();

            {
                selected = m_Application->GetEditorState() == EditorState::Next;
                if (selected)
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));

                if (ImGui::Button(ICON_FA_STEP_FORWARD, ImVec2(19.0f, 19.0f)))
                    m_Application->SetEditorState(EditorState::Next);

                ImGuiHelpers::Tooltip("Next");

                if (selected)
                    ImGui::PopStyleColor();
            }

			ImGui::PopStyleColor();
        
            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 80.0f);
        
            bool setNewValue = false;
            String RenderAPI = "";
            static auto renderAPI = Graphics::GraphicsContext::GetRenderAPI();
        
            bool needsRestart = false;
            if(renderAPI != Graphics::GraphicsContext::GetRenderAPI())
            {
                needsRestart = true;
            }
        
            switch (renderAPI)
            {
    #ifdef LUMOS_RENDER_API_OPENGL
            case Graphics::RenderAPI::OPENGL: RenderAPI = "OpenGL"; break;
    #endif
    
    #ifdef LUMOS_RENDER_API_VULKAN
            case Graphics::RenderAPI::VULKAN: RenderAPI = "Vulkan"; break;
    #endif
    
    #ifdef LUMOS_RENDER_API_DIRECT3D
            case DIRECT3D: RenderAPI = "Direct3D"; break;
    #endif
            default: break;
            }
        
            if(needsRestart)
                RenderAPI = "*" + RenderAPI;
            if (ImGui::BeginMenu(RenderAPI.c_str()))
            {
    #ifdef LUMOS_RENDER_API_OPENGL
            if (ImGui::MenuItem("OpenGL","", renderAPI == Graphics::RenderAPI::OPENGL)) { setNewValue = true; renderAPI = Graphics::RenderAPI::OPENGL;}
    #endif

    #ifdef LUMOS_RENDER_API_VULKAN
    #if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_IOS)
            if (ImGui::MenuItem("Vulkan ( MoltenVK )","", renderAPI == Graphics::RenderAPI::VULKAN)) { setNewValue = true; renderAPI = Graphics::RenderAPI::VULKAN;}
    #else
            if (ImGui::MenuItem("Vulkan","", renderAPI == Graphics::RenderAPI::VULKAN)) { setNewValue = true; renderAPI = Graphics::RenderAPI::VULKAN;}
    #endif
    #endif

    #ifdef LUMOS_RENDER_API_DIRECT3D
            if (ImGui::MenuItem("Direct3D","", renderAPI == Graphics::RenderAPI::DIRECT3D)) { setNewValue = true; renderAPI = Graphics::RenderAPI::DIRECT3D;}
    #endif
                ImGui::EndMenu();
            }
        
            if(needsRestart)
                ImGuiHelpers::Tooltip("Restart needed to switch Render API");
        
            if(setNewValue)
            {
                auto& appProps = m_Application->m_InitialProperties;
                if(appProps.FilePath != "")
                {
                    std::string physicalPath;
                    if (VFS::Get()->ResolvePhysicalPath(appProps.FilePath, physicalPath))
                    {
                        auto configFile = FileSystem::ReadTextFile(physicalPath);
                    
                        std::string toReplace("renderAPI=");
                        size_t pos = configFile.find(toReplace);
                    
                        if (pos != std::string::npos)
                        {
                            configFile.replace(pos + 10, 1, StringFormat::ToString(int(renderAPI)));
                        
                            FileSystem::WriteTextFile(physicalPath, configFile);
                        }
                    }
                }
            }
        
        
			ImGui::EndMainMenuBar();
		}
	}
    
	void Editor::OnImGuizmo()
	{
		if (m_Selected == entt::null || m_ImGuizmoOperation == 4)
			return;

		if (m_ShowGizmos)
		{
			Maths::Matrix4 view = m_EditorCamera->GetViewMatrix();
			Maths::Matrix4 proj = m_EditorCamera->GetProjectionMatrix();

#ifdef LUMOS_RENDER_API_VULKAN
			if (Graphics::GraphicsContext::GetRenderAPI() == Graphics::RenderAPI::VULKAN)
				proj.m11_ *= -1.0f;
#endif

			view = view.Transpose();
			proj = proj.Transpose();

			ImGuizmo::SetDrawlist();
			ImGuizmo::SetOrthographic(m_EditorCamera->IsOrthographic());

			auto& registry = m_Application->GetSceneManager()->GetCurrentScene()->GetRegistry();
			auto transform = registry.try_get<Maths::Transform>(m_Selected);
			if (transform != nullptr)
			{
				Maths::Matrix4 model = transform->GetWorldMatrix();
				model = model.Transpose();

				float snapAmount[3] = { m_SnapAmount  , m_SnapAmount , m_SnapAmount };
				float delta[16];

				ImGuizmo::Manipulate(Maths::ValuePointer(view), Maths::ValuePointer(proj), static_cast<ImGuizmo::OPERATION>(m_ImGuizmoOperation), ImGuizmo::LOCAL, Maths::ValuePointer(model), delta, m_SnapQuizmo ? snapAmount : nullptr);

				if (ImGuizmo::IsUsing())
				{
					if (static_cast<ImGuizmo::OPERATION>(m_ImGuizmoOperation) == ImGuizmo::OPERATION::SCALE)
					{
						auto mat = Maths::Matrix4(delta).Transpose();
						transform->SetLocalScale(transform->GetLocalScale() * mat.Scale());
					}
					else
					{
						auto mat = Maths::Matrix4(delta).Transpose() * transform->GetLocalMatrix();
						transform->SetLocalTransform(mat);

						auto physics2DComponent = registry.try_get<Physics2DComponent>(m_Selected);

						if (physics2DComponent)
						{
							physics2DComponent->GetPhysicsObject()->SetPosition({ mat.Translation().x, mat.Translation().y });
						}
						else
						{
							auto physics3DComponent = registry.try_get<Physics3DComponent>(m_Selected);
							if (physics3DComponent)
							{
								physics3DComponent->GetPhysicsObject()->SetPosition(mat.Translation());
								physics3DComponent->GetPhysicsObject()->SetOrientation(mat.Rotation());
							}
						}
					}

				}
			}
		}

			if (m_Selected != entt::null && m_ShowViewSelected)
			{
				auto& registry = m_Application->GetSceneManager()->GetCurrentScene()->GetRegistry();

				auto transform = registry.try_get<Maths::Transform>(m_Selected);

#if 0
				Maths::Matrix4 view = m_EditorCamera->GetViewMatrix();

				float camDistance = transform ? (m_EditorCamera->GetPosition() - transform->GetWorldMatrix().Translation()).Length() : 0.0f;

				auto window = ImGui::GetCurrentWindow();
				auto size = 128.0f;
				auto windowPos = window->Pos;
				auto windowSize = window->Size;
				auto viewManipulatePos = ImVec2(windowPos.x + windowSize.x - size, size / 2.0f - 20.0f + windowPos.y);

				//view = Maths::Matrix4::Inverse(view);
				ImGuizmo::ViewManipulate(Maths::ValuePointer(view), camDistance, viewManipulatePos, ImVec2(size, size), 0x10101010);
				view = Maths::Matrix4::Inverse(view);
				auto quat = view.ToQuaternion();
				auto euler = quat.ToEuler();
				camera->SetPitch(euler.x);
				camera->SetYaw(euler.y);
				camera->SetPosition(view.Translation());
#else
				float pos[3] = { m_EditorCamera->GetPosition().x, m_EditorCamera->GetPosition().y, m_EditorCamera->GetPosition().z };
				float rot[3] = { m_EditorCamera->GetPitch(), m_EditorCamera->GetYaw(), m_EditorCamera->GetRoll() };
				float scale[3] = { 1.0f, 1.0f, 1.0f };
				float view[16];
				ImGuizmo::RecomposeMatrixFromComponents(pos, rot, scale, view);

				float camDistance = 1.0f;// transform ? (Application::Instance()->GetSceneManager()->GetCurrentScene()->GetCamera()->GetPosition() - transform->GetWorldMatrix().Translation()).Length() : 0.0f;

				auto window = ImGui::GetCurrentWindow();
				auto size = 128.0f;
				auto windowPos = window->Pos;
				auto windowSize = window->Size;
				auto viewManipulatePos = ImVec2(windowPos.x + windowSize.x - size, size / 2.0f - 20.0f + windowPos.y);

				ImGuizmo::ViewManipulate(view, camDistance, viewManipulatePos, ImVec2(size, size), 0x10101010);
				ImGuizmo::DecomposeMatrixToComponents(view, pos, rot, scale);

				//if (modified)
				{
					m_EditorCamera->SetPitch(rot[0]);
					m_EditorCamera->SetYaw(rot[1]);
					m_EditorCamera->SetRoll(rot[2]);
					m_EditorCamera->SetPosition({ pos[0], pos[1], pos[2] });
				}
#endif
			}
	}

	void Editor::BeginDockSpace(bool infoBar)
	{
        static bool p_open = true;
        static bool opt_fullscreen_persistant = true;
        static ImGuiDockNodeFlags opt_flags = ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoCloseButton;
        bool opt_fullscreen = opt_fullscreen_persistant;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags =  ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen)
        {
            ImGuiViewport* viewport = ImGui::GetMainViewport();

            auto pos = viewport->Pos;
            auto size = viewport->Size;
			bool menuBar = true;
			if (menuBar)
			{
				const float infoBarSize = 19.0f;
				pos.y += infoBarSize;
				size.y -= infoBarSize;
			}

            if (infoBar)
            {
                const float infoBarSize = 24.0f;
                pos.y += infoBarSize;
                size.y -= infoBarSize;
            }
            
            ImGui::SetNextWindowPos(pos);
            ImGui::SetNextWindowSize(size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }
        
        // When using ImGuiDockNodeFlags_PassthruDockspace, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
        if (opt_flags & ImGuiDockNodeFlags_DockSpace)
            window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("MyDockspace", &p_open, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

		if (ImGui::DockBuilderGetNode(ImGui::GetID("MyDockspace")) == nullptr)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
			ImGui::DockBuilderRemoveNode(dockspace_id);
			ImGui::DockBuilderAddNode(dockspace_id);
			ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetIO().DisplaySize);

			ImGuiID dock_main_id   = dockspace_id;
			ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.2f, nullptr, &dock_main_id);
			ImGuiID dock_id_left   = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);
            ImGuiID dock_id_right  = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.3f, nullptr, &dock_main_id);
			ImGuiID dock_id_middle = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.8f, nullptr, &dock_main_id);

			ImGui::DockBuilderDockWindow("###scene", dock_id_middle);
			ImGui::DockBuilderDockWindow("###inspector", dock_id_right);
			ImGui::DockBuilderDockWindow("###hierarchy", dock_id_left);
			ImGui::DockBuilderDockWindow("###console", dock_id_bottom);
            ImGui::DockBuilderDockWindow("###profiler", dock_id_bottom);
            ImGui::DockBuilderDockWindow("Assets", dock_id_bottom);
			ImGui::DockBuilderDockWindow("Dear ImGui Demo", dock_id_left);
            ImGui::DockBuilderDockWindow("GraphicsInfo", dock_id_left);
			ImGui::DockBuilderDockWindow("ApplicationInfo", dock_id_left);
			ImGui::DockBuilderFinish(dockspace_id);
		}
		
		  // Dockspace
          ImGuiIO& io = ImGui::GetIO();
          if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
          {
              ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
              ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), opt_flags);
          }
	}

	void Editor::EndDockSpace()
	{
		ImGui::End();
	}

	void Editor::OnNewScene(Scene * scene)
	{
        m_Selected = entt::null;
		m_CurrentSceneAspectRatio = 0.0f;

		for (auto window : m_Windows)
		{
			window->OnNewScene(scene);
		}
	}

	void Editor::Draw2DGrid(ImDrawList* drawList, const ImVec2& cameraPos, const ImVec2& windowPos, const ImVec2& canvasSize, const float factor, const float thickness)
	{	
		static const auto graduation = 10;
		float GRID_SZ = canvasSize.y * 0.5f / factor;
		const ImVec2& offset = { canvasSize.x * 0.5f - cameraPos.x * GRID_SZ, canvasSize.y * 0.5f + cameraPos.y * GRID_SZ };

		ImU32 GRID_COLOR = IM_COL32(200, 200, 200, 40);
		float gridThickness = 1.0f;

		const auto& gridColor = GRID_COLOR;
		auto smallGraduation = GRID_SZ / graduation;
		const auto& smallGridColor = IM_COL32(100, 100, 100, smallGraduation);

		for (float x = -GRID_SZ; x < canvasSize.x + GRID_SZ; x += GRID_SZ)
		{
			auto localX = floorf(x + fmodf(offset.x, GRID_SZ));
			drawList->AddLine(ImVec2{ localX, 0.0f } + windowPos, ImVec2{ localX, canvasSize.y } +windowPos, gridColor, gridThickness);

			if (smallGraduation > 5.0f)
			{
				for (int i = 1; i < graduation; ++i)
				{
					const auto graduation = floorf(localX + smallGraduation * i);
					drawList->AddLine(ImVec2{ graduation, 0.0f } +windowPos, ImVec2{ graduation, canvasSize.y } +windowPos, smallGridColor, 1.0f);
				}
			}
			
		}

		for (float y = -GRID_SZ; y < canvasSize.y + GRID_SZ; y += GRID_SZ)
		{
			auto localY = floorf(y + fmodf(offset.y, GRID_SZ));
			drawList->AddLine(ImVec2{ 0.0f, localY } +windowPos, ImVec2{ canvasSize.x, localY } +windowPos, gridColor, gridThickness);

			if (smallGraduation > 5.0f)
			{
				for (int i = 1; i < graduation; ++i)
				{
					const auto graduation = floorf(localY + smallGraduation * i);
					drawList->AddLine(ImVec2{ 0.0f, graduation } +windowPos, ImVec2{ canvasSize.x, graduation } +windowPos, smallGridColor,1.0f);
				}
			}
		}
	}

	void Editor::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Editor::OnWindowResize));

		m_Application->OnEvent(e);
	}
    
    Maths::Ray Editor::GetScreenRay(int x, int y, Camera* camera, int width, int height)
    {
        if (!camera)
            return Maths::Ray();
    
        float screenX = (float)x / (float)width;
        float screenY = (float)y / (float)height;

        bool flipY = false;
                
        #ifdef LUMOS_RENDER_API_OPENGL
        if(Graphics::GraphicsContext::GetRenderAPI() == Graphics::RenderAPI::OPENGL)
            flipY = true;
        #endif
        return camera->GetScreenRay(screenX, screenY, flipY);
    }

	void Editor::OnUpdate(const TimeStep& ts)
	{
        auto& registry = m_Application->GetSceneManager()->GetCurrentScene()->GetRegistry();
        auto cameraView = registry.view<Camera>();
        if(!cameraView.empty())
        {
            m_EditorCamera = &registry.get<Camera>(cameraView.front());
        }
		auto cameraController = m_EditorCamera ? m_EditorCamera->GetController() : nullptr;

		if (cameraController && Application::Instance()->GetSceneActive())
		{
			const Maths::Vector2 mousePos = Input::GetInput()->GetMousePosition();

			cameraController->HandleMouse(m_EditorCamera, ts.GetMillis(), mousePos.x, mousePos.y);
			cameraController->HandleKeyboard(m_EditorCamera, ts.GetMillis());
		}
    
		if (Input::GetInput()->GetKeyPressed(InputCode::Key::F))
		{
			if (registry.valid(m_Selected))
			{
				auto transform = registry.try_get<Maths::Transform>(m_Selected);
				if (transform)
					FocusCamera(transform->GetWorldPosition(), 2.0f, 2.0f);
			}
		}

		if (m_TransitioningCamera)
		{
            if (m_CameraTransitionStartTime < 0.0f)
                m_CameraTransitionStartTime = ts.GetElapsedMillis();

            float focusProgress = Maths::Min((ts.GetElapsedMillis() - m_CameraTransitionStartTime) / m_CameraTransitionSpeed, 1.f);
            Maths::Vector3 newCameraPosition = m_CameraStartPosition.Lerp(m_CameraDestination, focusProgress);
            m_EditorCamera->SetPosition(newCameraPosition);

            if (m_EditorCamera->GetPosition().Equals(m_CameraDestination))
                m_TransitioningCamera = false;
        }
    
        if (!Input::GetInput()->GetMouseHeld(InputCode::MouseKey::ButtonRight))
        {
            if (Input::GetInput()->GetKeyPressed(InputCode::Key::Q))
            {
                SetImGuizmoOperation(4);
            }

            if (Input::GetInput()->GetKeyPressed(InputCode::Key::W))
            {
                SetImGuizmoOperation(0);
            }

            if (Input::GetInput()->GetKeyPressed(InputCode::Key::E))
            {
                SetImGuizmoOperation(1);
            }
        
            if (Input::GetInput()->GetKeyPressed(InputCode::Key::R))
            {
                SetImGuizmoOperation(2);
            }
        
            if (Input::GetInput()->GetKeyPressed(InputCode::Key::T))
            {
                SetImGuizmoOperation(3);
            }
        
            if (Input::GetInput()->GetKeyPressed(InputCode::Key::Y))
            {
                ToggleSnap();
            }
        }
    }

	void Editor::BindEventFunction()
	{
		m_Application->GetWindow()->SetEventCallback(BIND_EVENT_FN(Editor::OnEvent));
	}

	void Editor::FocusCamera(const Maths::Vector3& point, float distance, float speed)
	{
        if(m_EditorCamera->IsOrthographic())
        {
            m_EditorCamera->SetPosition(point);
            m_EditorCamera->SetScale(distance / 2.0f);
        }
        else
        {
            m_TransitioningCamera = true;

            m_CameraDestination = point + m_EditorCamera->GetForwardDirection() * distance;
            m_CameraTransitionStartTime = -1.0f;
            m_CameraTransitionSpeed = 1.0f/speed;
            m_CameraStartPosition = m_EditorCamera->GetPosition();
        }
	}

	bool Editor::OnWindowResize(WindowResizeEvent & e)
	{
		return false;
	}
    
    void Editor::RecompileShaders()
    {
        Lumos::Debug::Log::Info("Recompiling shaders");

    #ifdef LUMOS_RENDER_API_VULKAN
    #ifdef LUMOS_PLATFORM_WINDOWS
        //String filePath = ROOT_DIR"/Assets/shaders/CompileShadersWindows.bat";
        //system(filePath.c_str());
    #elif LUMOS_PLATFORM_MACOS
        String filePath = ROOT_DIR"/Assets/shaders/CompileShadersMac.sh";
        system(filePath.c_str());
    #endif
    #endif
    }
    
    void Editor::DebugDraw()
    {
        auto& registry = Application::Instance()->GetSceneManager()->GetCurrentScene()->GetRegistry();

        if(m_DebugDrawFlags & EditorDebugFlags::MeshBoundingBoxes)
        {
            auto group = registry.group<MeshComponent>(entt::get<Maths::Transform>);

             for(auto entity : group)
             {
                 const auto &[mesh, trans] = group.get<MeshComponent, Maths::Transform>(entity);

                 if (mesh.GetMesh() && mesh.GetMesh()->GetActive())
                 {
                     auto& worldTransform = trans.GetWorldMatrix();

                     auto bbCopy = mesh.GetMesh()->GetBoundingBox()->Transformed(worldTransform);
                     DebugRenderer::DebugDraw(bbCopy, Maths::Vector4(0.1f,0.9f,0.1f,0.4f), true);
                 }
             }
        }
    
        if(m_DebugDrawFlags & EditorDebugFlags::SpriteBoxes)
        {
           auto group = registry.group<Graphics::Sprite>(entt::get<Maths::Transform>);

            for(auto entity : group)
            {
                const auto &[sprite, trans] = group.get<Graphics::Sprite, Maths::Transform>(entity);

            {
                    auto& worldTransform = trans.GetWorldMatrix();

                    auto bb = Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetPosition() + sprite.GetScale()));
                    bb.Transform(trans.GetWorldMatrix());
                    DebugRenderer::DebugDraw(bb, Maths::Vector4(0.1f, 0.9f, 0.1f, 0.4f), true);
                }
            }
        }

        if(m_DebugDrawFlags & EditorDebugFlags::CameraFrustum)
        {
            auto cameraGroup = registry.group<Camera>(entt::get<Maths::Transform>);

            for(auto entity : cameraGroup)
            {
                const auto &[camera, trans] = cameraGroup.get<Camera, Maths::Transform>(entity);

                {
                    DebugRenderer::DebugDraw(camera.GetFrustum(), Maths::Vector4(0.9f));
                }
            }
        }
    
        if(registry.valid(m_Selected))
        {
            auto transform = registry.try_get<Maths::Transform>(m_Selected);
        
            auto meshComponent = registry.try_get<MeshComponent>(m_Selected);
            if(transform && meshComponent)
            {
                if (meshComponent->GetMesh() && meshComponent->GetMesh()->GetActive())
                {
                    auto& worldTransform = transform->GetWorldMatrix();

                    auto bbCopy = meshComponent->GetMesh()->GetBoundingBox()->Transformed(worldTransform);
                    DebugRenderer::DebugDraw(bbCopy, Maths::Vector4(0.1f, 0.9f, 0.1f, 0.4f), true);
                }
            }
        
            auto sprite = registry.try_get<Graphics::Sprite>(m_Selected);
            if(transform && sprite)
            {
                {
                    auto& worldTransform = transform->GetWorldMatrix();

                    auto bb = Maths::BoundingBox(Maths::Rect(sprite->GetPosition(), sprite->GetPosition() + sprite->GetScale()));
                    bb.Transform(worldTransform);
                    DebugRenderer::DebugDraw(bb, Maths::Vector4(0.1f, 0.9f, 0.1f, 0.4f), true);
                }
            }
        
            auto camera = registry.try_get<Camera>(m_Selected);
            if(camera)
            {
                DebugRenderer::DebugDraw(camera->GetFrustum(), Maths::Vector4(0.9f));
            }
        
            auto light = registry.try_get<Graphics::Light>(m_Selected);
            if(light)
            {
                DebugRenderer::DebugDraw(light, Maths::Vector4(light->m_Colour.ToVector3(), 0.2f));
            }
        
            auto sound = registry.try_get<SoundComponent>(m_Selected);
            if(sound)
            {
                DebugRenderer::DebugDraw(sound->GetSoundNode(), Maths::Vector4(0.8f,0.8f,0.8f,0.2f));
            }
        }
    }
    
    void Editor::SelectObject(const Maths::Ray& ray)
    {
        auto& registry = Application::Instance()->GetSceneManager()->GetCurrentScene()->GetRegistry();
        float closestEntityDist = Maths::M_INFINITY;
        entt::entity currentClosestEntity = entt::null;

        auto group = registry.group<MeshComponent>(entt::get<Maths::Transform>);

        static Timer timer;
        static float timeSinceLastSelect = 0.0f;

        for(auto entity : group)
        {
           const auto &[mesh, trans] = group.get<MeshComponent, Maths::Transform>(entity);

           if (mesh.GetMesh() && mesh.GetMesh()->GetActive())
           {
               auto& worldTransform = trans.GetWorldMatrix();

               auto bbCopy = mesh.GetMesh()->GetBoundingBox()->Transformed(worldTransform);
               float dist = ray.HitDistance(bbCopy);
           
               if(dist < Maths::M_INFINITY)
               {
                    if(dist < closestEntityDist)
                    {
                        closestEntityDist = dist;
                        currentClosestEntity = entity;
                    }
               }
           }
        }

        if(m_Selected != entt::null)
        {
            if(m_Selected == currentClosestEntity)
            {
                if(timer.GetMS(1.0f) - timeSinceLastSelect < 1.0f)
                {
                    auto& trans = registry.get<Maths::Transform>(m_Selected);
                    auto& mesh = registry.get<MeshComponent>(m_Selected);
                    auto bb = mesh.GetMesh()->GetBoundingBox()->Transformed(trans.GetWorldMatrix());

                    FocusCamera(trans.GetWorldPosition(), (bb.max_ - bb.min_).Length());
                }
                else
                {
                    currentClosestEntity = entt::null;
                }
            }

            timeSinceLastSelect = timer.GetMS(1.0f);
            m_Selected = currentClosestEntity;
            return;
        }
    
        auto spriteGroup = registry.group<Graphics::Sprite>(entt::get<Maths::Transform>);

        for(auto entity : spriteGroup)
        {
            const auto &[sprite, trans] = spriteGroup.get<Graphics::Sprite, Maths::Transform>(entity);

            auto& worldTransform = trans.GetWorldMatrix();
            auto bb = Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetPosition() + sprite.GetScale()));
            bb.Transform(trans.GetWorldMatrix());
            float dist = ray.HitDistance(bb);
      
            if(dist < Maths::M_INFINITY)
            {
            	if(dist < closestEntityDist)
				{
					closestEntityDist = dist;
					currentClosestEntity = entity;
				}
            }
        }

        if(m_Selected != entt::null)
		{
			if(m_Selected == currentClosestEntity)
			{
				auto& trans = registry.get<Maths::Transform>(m_Selected);
				auto& sprite = registry.get<Graphics::Sprite>(m_Selected);
				auto bb = Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetPosition() + sprite.GetScale()));

				FocusCamera(trans.GetWorldPosition(), (bb.max_ - bb.min_).Length());
			}
		}


        m_Selected = currentClosestEntity;
    }

	void Editor::OpenTextFile(const String& filePath)
	{
		String physicalPath;
        if (!VFS::Get()->ResolvePhysicalPath(filePath, physicalPath))
        {
            Debug::Log::Error("Failed to Load Lua script {0}", filePath );
            return;
        }

    
        for(int i = 0; i < int(m_Windows.size()); i++)
        {
            EditorWindow* w = m_Windows[i].get();
            if(w->GetSimpleName() == "TextEdit")
            {
                m_Windows.erase (m_Windows.begin()+i);
                break;
            }
        }

		m_Windows.emplace_back(CreateRef<TextEditWindow>(physicalPath));
		m_Windows.back()->SetEditor(this);
	}

	void Editor::RemoveWindow(EditorWindow* window)
	{
		for(int i = 0; i < int(m_Windows.size()); i++)
		{
			EditorWindow* w = m_Windows[i].get();
			if(w == window)
            {
                m_Windows.erase (m_Windows.begin()+i);
                return;
            }
		}
	}
    
    void Editor::ShowPreview()
    {
        ImGui::Begin("Preview");
        if(m_PreviewTexture)
            ImGuiHelpers::Image(m_PreviewTexture.get(),{ 200, 200});
        ImGui::End();
    }
    
    void Editor::OnRender()
    {
        DrawPreview();
    }
    
    void Editor::DrawPreview()
    {
        if(!m_PreviewTexture)
        {
            m_PreviewTexture = Ref<Graphics::Texture2D>(Graphics::Texture2D::Create());
            m_PreviewTexture->BuildTexture(Graphics::TextureFormat::RGBA32, 200, 200, false, false);

            m_PreviewRenderer = CreateRef<Graphics::ForwardRenderer>(200, 200, false, false);
            m_PreviewSphere = Ref<Graphics::Mesh>(Graphics::CreateSphere());
        
            m_PreviewRenderer->SetRenderTarget(m_PreviewTexture.get());
        }
    
        Maths::Matrix4 proj = Maths::Matrix4::Perspective(0.1f, 10.0f, 200.0f / 200.0f, 60.0f);
        Maths::Matrix4 view = Maths::Matrix3x4(Maths::Vector3(0.0f,0.0f,3.0f), Maths::Quaternion::EulerAnglesToQuaternion(0.0f,0.0f,0.0f), Maths::Vector3(1.0f)).Inverse().ToMatrix4();
    
        m_PreviewRenderer->Begin();
        m_PreviewRenderer->BeginScene(proj, view);
        m_PreviewRenderer->SubmitMesh(m_PreviewSphere.get(), nullptr,  Maths::Matrix4(), Maths::Matrix4());
        m_PreviewRenderer->SetSystemUniforms(m_PreviewRenderer->GetShader());
        m_PreviewRenderer->Present();
        m_PreviewRenderer->End();
    }

    void Editor::FileOpenCallback(const String& filePath)
    {
        if(IsTextFile(filePath))
            OpenTextFile(filePath);
        else if(IsModelFile(filePath))
        {
            auto entity = ModelLoader::LoadModel(filePath, m_Application->GetSceneManager()->GetCurrentScene()->GetRegistry());
            m_Selected = entity;
        }
        else if(IsAudioFile(filePath))
        {
            AssetsManager::Sounds()->LoadAsset(StringFormat::GetFileName(filePath), filePath);

            auto soundNode = Ref<SoundNode>(SoundNode::Create());
            soundNode->SetSound(AssetsManager::Sounds()->Get(StringFormat::GetFileName(filePath)).get());
            soundNode->SetVolume(1.0f);
            soundNode->SetPosition(Maths::Vector3(0.1f, 10.0f, 10.0f));
            soundNode->SetLooping(true);
            soundNode->SetIsGlobal(false);
            soundNode->SetPaused(false);
            soundNode->SetReferenceDistance(1.0f);
            soundNode->SetRadius(30.0f);
        
            auto& registry = m_Application->GetSceneManager()->GetCurrentScene()->GetRegistry();
            entt::entity e = registry.create();
            registry.emplace<SoundComponent>(e, soundNode);
            m_Selected = e;
        }
    }
}
