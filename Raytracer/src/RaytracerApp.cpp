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
			m_scene.Objects.push_back(std::make_unique<Sphere>(sphere));
		}
		{
			Sphere sphere;
			sphere.Position = { 5.f,5.f,-1.f };
			sphere.Radius = 2.f;
			sphere.MaterialIndex = 2;
			m_scene.Objects.push_back(std::make_unique<Sphere>(sphere));
		}
		{
			Plane plane;
			plane.Position = { 0.0f, -1.0f, 0.0f };
			plane.Normal = { 0.0f, 1.0f, 0.0f };
			plane.MaterialIndex = 1;
			m_scene.Objects.push_back(std::make_unique<Plane>(plane));
		}
		{
			std::vector<Triangle> cubeTriangles;

			// Define the vertices of the cube
			glm::vec3 vertices[8] = {
				glm::vec3(-0.5, -0.5, -0.5),
				glm::vec3(0.5, -0.5, -0.5),
				glm::vec3(0.5, 0.5, -0.5),
				glm::vec3(-0.5, 0.5, -0.5),
				glm::vec3(-0.5, -0.5, 0.5),
				glm::vec3(0.5, -0.5, 0.5),
				glm::vec3(0.5, 0.5, 0.5),
				glm::vec3(-0.5, 0.5, 0.5)
			};

			// Define the indices for the triangles that make up each face
			int indices[6][6] = {
				{0, 1, 2, 0, 2, 3}, // Front face
				{1, 5, 6, 1, 6, 2}, // Right face
				{5, 4, 7, 5, 7, 6}, // Back face
				{4, 0, 3, 4, 3, 7}, // Left face
				{3, 2, 6, 3, 6, 7}, // Top face
				{4, 5, 1, 4, 1, 0}  // Bottom face
			};

			// Create triangles for each face
			for (int i = 0; i < 6; i++) {
				Triangle t1;
				t1.points[0] = { vertices[indices[i][0]].x, vertices[indices[i][0]].y, vertices[indices[i][0]].z };
				t1.points[1] = { vertices[indices[i][1]].x, vertices[indices[i][1]].y, vertices[indices[i][1]].z };
				t1.points[2] = { vertices[indices[i][2]].x, vertices[indices[i][2]].y, vertices[indices[i][2]].z };

				Triangle t2;
				t2.points[0] = { vertices[indices[i][3]].x, vertices[indices[i][3]].y, vertices[indices[i][3]].z };
				t2.points[1] = { vertices[indices[i][4]].x, vertices[indices[i][4]].y, vertices[indices[i][4]].z };
				t2.points[2] = { vertices[indices[i][5]].x, vertices[indices[i][5]].y, vertices[indices[i][5]].z };

				cubeTriangles.push_back(t1);
				cubeTriangles.push_back(t2);
			}

			// Instantiate a Model object with the cube triangles
			Model cubeModel(cubeTriangles, glm::vec3(-2.0f, 0.0f, 2.0f), 0);

			m_scene.Objects.push_back(std::make_unique<Model>(cubeModel));
		}
	}
	virtual void OnUIRender() override
	{
		ImGui::Begin("Settings");
		ImGui::Text("Render Time: %f ms",m_RenderTime);
		ImGui::Text("Sample No.: %i", m_Renderer.getFrameIndex());
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
			m_scene.Objects.push_back(std::make_unique<Sphere>(Sphere({ 1.f,0.f,0.f }, 0.5f, 0)));
		for (size_t i = 0; i < m_scene.Objects.size(); i ++) {
			auto& obj = m_scene.Objects[i];
			ImGui::PushID(&obj);
			if (ImGui::Button("Delete Object"))
			{
				m_scene.Objects.erase(m_scene.Objects.begin() + i);
				ImGui::PopID();
				break;
			}
			ImGui::DragFloat3("Position", glm::value_ptr(obj->Position), 0.01f);

			if(dynamic_cast<const Sphere*>(obj.get()))
				ImGui::DragFloat("Radius", &dynamic_cast<Sphere*>(obj.get())->Radius, 0.1f);
			
			ImGui::DragInt("Material Index", &obj->MaterialIndex, 1.0f, 0.0f, (int)m_scene.materials.size()-1);
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