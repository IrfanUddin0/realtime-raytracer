#include "Renderer.h"

namespace Utils {
	static uint32_t ConvertToRGBA(const glm::vec4& col)
	{
		return (int)(col.a * 255.f) << 24 | (int)(col.b * 255.f) << 16 | (int)(col.g * 255.f) << 8 | (int)(col.r * 255.f);
	}
}

void Renderer::Render(const Scene& scene, const Camera& camera)
{
	Ray ray;
	ray.Origin = camera.GetPosition();
	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); y++)
	{
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++)
		{
			ray.Direction = camera.GetRayDirections()[x + y * (float)m_FinalImage->GetWidth()];

			glm::vec4 col = TraceRay(scene, ray);
			col = glm::clamp(col, glm::vec4(0.f), glm::vec4(1.f));
			m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(col);
		}
	}
	m_FinalImage->SetData(m_ImageData);
}

void Renderer::OnResize(uint32_t width, uint32_t height)
{
	if (m_FinalImage)
	{
		if (m_FinalImage->GetWidth() == width && m_FinalImage->GetHeight() == height)
			return;

		m_FinalImage->Resize(width, height);
	}
	else
	{
		m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
	}

	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * height];
}

glm::vec4 Renderer::TraceRay(const Scene& scene, const Ray& ray)
{
	if(scene.Spheres.size() == 0)
		return glm::vec4(0, 0, 0, 1);

	const Sphere* closest = nullptr;
	float hitDistance = FLT_MAX;

	for (const auto& sphere : scene.Spheres)
	{
		auto origin = ray.Origin - sphere.Position;

		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2 * glm::dot(origin, ray.Direction);
		float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

		float d = b * b - 4.f * a * c;
		if (d < 0)
			continue;

		float t[2] =
		{
			(-b - glm::sqrt(d)) / (2.f * a),
			(-b + glm::sqrt(d)) / (2.f * a)
		};

		if (t[0] < hitDistance)
		{
			hitDistance = t[0];
			closest = &sphere;
		}
	}

	if(closest==nullptr)
		return glm::vec4(0, 0, 0, 1);

	glm::vec3 origin = ray.Origin - closest->Position;
	glm::vec3 hit = origin + ray.Direction * hitDistance;
	glm::vec3 normal = glm::normalize(hit);

	glm::vec3 light_dir(-1, -1, -1);
	light_dir = glm::normalize(light_dir);

	float face_light = glm::max(glm::dot(normal, -light_dir), 0.0f);

	return glm::vec4(closest->col * face_light, 1);
		
}
