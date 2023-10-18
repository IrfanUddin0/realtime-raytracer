#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Random.h"
#include "Walnut/Timer.h"

#include "Renderer.h"
#include "Camera.h"

#include <glm/gtc/type_ptr.hpp>

class ExampleLayer : public Walnut::Layer
{
public:
	ExampleLayer()
		: m_camera(87.f, 0.1f, 100.f)
	{
		m_scene.materials.push_back(Material{ { 1.0f, 0.0f, 0.0f }, 0.1f, 1.f });
		m_scene.materials.push_back(Material{ { 1.0f, 0.0f, 1.0f }, 0.1f, 1.f });
		m_scene.materials.push_back(Material{ { 1.0f, 1.0f, 1.0f }, 0.1f, 1.f , { 1.0f, 1.0f, 1.0f } , 5.0f});

		{
			Sphere sphere;
			sphere.Position = { 0.f,0.f,0.f };
			sphere.Radius = 1.f;
			sphere.MaterialIndex = 0;
			m_scene.Spheres.push_back(sphere);
		}
		{
			Sphere sphere;
			sphere.Position = { 0.f,-1001.f,0.f };
			sphere.Radius = 1000.f;
			sphere.MaterialIndex = 1;
			m_scene.Spheres.push_back(sphere);
		}
		{
			Sphere sphere;
			sphere.Position = { 5.f,5.f,-1.f };
			sphere.Radius = 2.f;
			sphere.MaterialIndex = 2;
			m_scene.Spheres.push_back(sphere);
		}
	}
	virtual void OnUIRender() override
	{
		ImGui::Begin("Settings");
		ImGui::Text("Render Time: %f ms",m_RenderTime);
		ImGui::Checkbox("Accumulate Samples", &m_Renderer.getSettings().Accumulate);
		if (ImGui::Button("Reset"))
			m_Renderer.ResetFrameIndex();
		ImGui::DragInt("Bounces", (int*)&m_Renderer.getBounces(),0.1f, 1, 128);
		ImGui::End();


		ImGui::Begin("Camera");
		if (
			ImGui::DragFloat("Field Of View", m_camera.getFOV(), 1, 1, 180) ||
			ImGui::DragFloat("Near Clip", m_camera.getNearClip(), 1, 0) ||
			ImGui::DragFloat("Far Clip", m_camera.getFarClip(), 1))
			m_camera.updateView();
		ImGui::End();


		ImGui::Begin("Scene");
		if (ImGui::Button("Create Sphere"))
			m_scene.Spheres.push_back(Sphere{ {1.f,0.f,0.f}, 0.5f });
		for (size_t i = 0; i < m_scene.Spheres.size(); i ++) {
			auto& obj = m_scene.Spheres[i];
			ImGui::PushID(&obj);
			if (ImGui::Button("Delete Sphere"))
			{
				m_scene.Spheres.erase(m_scene.Spheres.begin() + i);
				ImGui::PopID();
				break;
			}
			ImGui::DragFloat3("Position", glm::value_ptr(obj.Position), 0.01f);
			ImGui::DragFloat("Radius", &obj.Radius, 0.1f);
			ImGui::DragInt("Material Index", &obj.MaterialIndex, 1.0f, 0.0f, (int)m_scene.materials.size()-1);
			ImGui::Separator();
			ImGui::PopID();
		}
		ImGui::End();


		ImGui::Begin("Materials");
		if (ImGui::Button("Create Material"))
			m_scene.materials.push_back(Material{});
		for (auto& material : m_scene.materials) {
			ImGui::PushID(&material);
			ImGui::ColorEdit3("Colour", glm::value_ptr(material.Albedo), 0.1f);
			ImGui::DragFloat("Metallic", &material.Metallic, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Roughness", &material.Roughness, 0.01f, 0.0f, 1.0f);

			ImGui::ColorEdit3("Emission Color", glm::value_ptr(material.EmissionColor));
			ImGui::DragFloat("Emission Power", &material.EmissionPower, 0.01f, 0.0f, FLT_MAX);
			ImGui::Separator();
			ImGui::PopID();
		}
		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
		ImGui::Begin("Viewport");

		m_ViewportWidth = ImGui::GetContentRegionAvail().x;
		m_ViewportHeight = ImGui::GetContentRegionAvail().y;

		auto image = m_Renderer.GetFinalImage();
		if (image)
			ImGui::Image(image->GetDescriptorSet(), { (float)image->GetWidth(), (float)image->GetHeight()}, ImVec2(0, 1), ImVec2(1,0));

		ImGui::End();
		ImGui::PopStyleVar();

		Render();
	}

	virtual void OnUpdate(float ts) override
	{
		if (m_camera.OnUpdate(ts))
		{
			m_Renderer.ResetFrameIndex();
		}
	}

	void Render() {
		Walnut::Timer timer;

		m_Renderer.OnResize(m_ViewportWidth, m_ViewportHeight);
		m_camera.OnResize(m_ViewportWidth, m_ViewportHeight);
		m_Renderer.Render(m_scene, m_camera);

		m_RenderTime = timer.ElapsedMillis();
	}

private:
	Camera m_camera;
	Renderer m_Renderer;
	uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
	Scene m_scene;
	float m_RenderTime = 0;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Raytracer";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<ExampleLayer>();
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				app->Close();
			}
			ImGui::EndMenu();
		}
	});
	return app;
}