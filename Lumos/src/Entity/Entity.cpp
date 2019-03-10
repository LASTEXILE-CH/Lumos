#include "LM.h"
#include "Entity.h"

#include "Graphics/Renderers/DebugRenderer.h"
#include "App/Scene.h"
#include "App/Application.h"
#include "Graphics/GBuffer.h"
#include "Graphics/RenderManager.h"
#include "Graphics/API/Context.h"

#include <imgui/imgui.h>
#include <imgui/plugins/ImGuizmo.h>

namespace Lumos
{
	Entity::Entity(Scene* scene) : m_Name("Unnamed"), m_pScene(scene), m_pParent(nullptr), m_BoundingRadius(1), m_FrustumCullFlags(0), m_UpdateTransforms(false)
	{
        Init();
	}

	Entity::Entity(const String& name,Scene* scene) : m_Name(name), m_pScene(scene), m_pParent(nullptr), m_BoundingRadius(1),
	                                     m_FrustumCullFlags(0), m_UpdateTransforms(false)
	{
        Init();
	}

	Entity::~Entity()
	{
	}
    
    void Entity::Init()
    {
        m_DefaultTransformComponent = std::make_unique<TransformComponent>(maths::Matrix4());
    }

	void Entity::AddComponent(std::unique_ptr<LumosComponent> component)
	{
		LUMOS_CORE_ASSERT(component->GetType(),"");
		component->SetEntity(this);
		component->Init();
		m_Components[component->GetType()] = std::move(component);
	}

	void Entity::OnRenderObject()
	{
		for(const auto& component : m_Components)
		{
			component.second->OnRenderComponent();
		}
	}

	void Entity::OnUpdateObject(float dt)
	{
		for (const auto& component : m_Components)
		{
			component.second->OnUpdateComponent(dt);

			if (m_UpdateTransforms)
				component.second->OnUpdateTransform(maths::Matrix4());
		}
	}

	void Entity::AddChildObject(Entity* child)
	{
		m_vpChildren.push_back(child);
		child->m_pParent = this;
		child->m_pScene = this->m_pScene;
	}
	void Entity::DebugDraw(uint64 debugFlags)
	{
		if (debugFlags & DEBUGDRAW_FLAGS_BOUNDING_RADIUS)
		{
			maths::Vector4 boundRadiusCol(0.3f, 0.6f, 0.4f, 0.8f);
			boundRadiusCol.SetW(0.2f);
			if (GetComponent<TransformComponent>())
				DebugRenderer::DrawPointNDT(GetComponent<TransformComponent>()->m_WorldSpaceTransform.GetPositionVector(), m_BoundingRadius, boundRadiusCol);
		}
		
		for(auto& component: m_Components)
		{
			component.second->DebugDraw(debugFlags);
		}
	}

	void Entity::OnGuizmo()
	{
		maths::Matrix4 view = m_pScene->GetCamera()->GetViewMatrix();
		maths::Matrix4 proj = m_pScene->GetCamera()->GetProjectionMatrix();

#ifdef LUMOS_RENDER_API_VULKAN
		if(graphics::Context::GetContext()->GetRenderAPI() == RenderAPI::VULKAN)
			proj[5] *= -1.0f;
#endif
		ImGuizmo::SetDrawlist();

		maths::Matrix4 model = maths::Matrix4();
		if (this->GetComponent<TransformComponent>() != nullptr)
			model = GetComponent<TransformComponent>()->m_WorldSpaceTransform;
		ImGuizmo::Manipulate(view.values, proj.values, ImGuizmo::SCALE, ImGuizmo::WORLD, model.values, NULL, NULL);

		if (this->GetComponent<TransformComponent>() != nullptr)
			GetComponent<TransformComponent>()->SetBothTransforms(model);
	}
    
    void Entity::OnIMGUI()
    {
        ImGuiWindowFlags window_flags = 0;
        ImGui::Begin(m_Name.c_str(), NULL, window_flags);

        for(auto& component: m_Components)
        {
            component.second->OnIMGUI();
        }
        
        ImGui::End();
        
    }
}
