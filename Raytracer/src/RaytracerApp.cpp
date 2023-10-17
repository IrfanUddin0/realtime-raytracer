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
		{
			Sphere sphere;
			sphere.col = { 1,0,0 };
			sphere.Position = { 0,0,0 };
			sphere.Radius = 0.5;
			m_scene.Spheres.push_back(sphere);
		}
		{
			Sphere sphere;
			sphere.col = { 0,1,1 };
			sphere.Position = { 0,0,-10 };
			sphere.Radius = 5;
			m_scene.Spheres.push_back(sphere);
		}
	}
	virtual void OnUIRender() override
	{
		ImGui::Begin("Settings");
		ImGui::Text("Render Time: %f ms",m_RenderTime);
		if (ImGui::Button("Render"))
		{
			Render();
		}
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
			m_scene.Spheres.push_back(Sphere{ {1.f,0.f,0.f}, 0.5f, {0.f,0.f,0.f} });
		for (auto& obj : m_scene.Spheres) {
			ImGui::PushID(&obj);
			ImGui::DragFloat3("Position", glm::value_ptr(obj.Position), 0.1f);
			ImGui::DragFloat("Radius", &obj.Radius, 0.1f);
			ImGui::ColorEdit3("Colour", glm::value_ptr(obj.col), 0.1f);
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
		m_camera.OnUpdate(ts);
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