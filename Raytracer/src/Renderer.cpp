#include "Renderer.h"

namespace Utils {
	static uint32_t ConvertToRGBA(const glm::vec4& col)
	{
		return (int)(col.a * 255.f) << 24 | (int)(col.b * 255.f) << 16 | (int)(col.g * 255.f) << 8 | (int)(col.r * 255.f);
	}
}

void Renderer::Render(const Scene& scene, const Camera& camera)
{
	m_ActiveScene = &scene;
	m_ActiveCamera = &camera;

	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); y++)
	{
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++)
		{
			glm::vec4 col = RayGen(x, y);
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

glm::vec4 Renderer::RayGen(uint32_t x, uint32_t y)
{
	Ray ray;
	ray.Origin = m_ActiveCamera->GetPosition();
	ray.Direction = m_ActiveCamera->GetRayDirections()[x + y * (float)m_FinalImage->GetWidth()];

	glm::vec3 out_col = glm::vec3(0.0f);
	float multiplier_falloff = 1.0f;
	int bounces = 4;
	for (int i = 0; i < bounces; i++)
	{
		Renderer::HitPayload payload = TraceRay(ray);
		if (payload.HitDistance < 0.0f)
		{
			glm::vec3 skyColor = glm::vec3(0.1f, 0.8f, 1.0f);
			out_col += skyColor * multiplier_falloff;
			break;
		}

		glm::vec3 light_dir(-1, -1, -1);
		light_dir = glm::normalize(light_dir);

		float face_light = glm::max(glm::dot(payload.WorldNormal, -light_dir), 0.0f);
		out_col += m_ActiveScene->Spheres[payload.objectIndex].col * face_light * multiplier_falloff;
		multiplier_falloff *= 0.7;

		ray.Origin = payload.WorldPosition + FLT_MIN * payload.WorldNormal;
		ray.Direction = glm::reflect(ray.Direction, payload.WorldNormal);
	}

	return glm::vec4(out_col, 1.0f);
}

Renderer::HitPayload Renderer::TraceRay(const Ray& ray)
{
	int closest = -1;
	float hitDistance = FLT_MAX;

	for (size_t i = 0; i < m_ActiveScene->Spheres.size(); i++)
	{
		const auto sphere = m_ActiveScene->Spheres[i];
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

		if (t[0] > 0 && t[0] < hitDistance)
		{
			hitDistance = t[0];
			closest = (int)i;
		}
	}

	if (closest < 0.0f)
		return Miss(ray);

	return ClosestHit(ray, hitDistance, closest);	
}

Renderer::HitPayload Renderer::Miss(const Ray& ray)
{
	Renderer::HitPayload payload;
	payload.HitDistance = -1.0f;
	return payload;
}

Renderer::HitPayload Renderer::ClosestHit(const Ray& ray, float hitDistance, int objectIndex)
{
	Renderer::HitPayload payload;
	payload.HitDistance = hitDistance;
	payload.objectIndex = objectIndex;

	const Sphere& closest = m_ActiveScene->Spheres[objectIndex];
	glm::vec3 origin = ray.Origin - closest.Position;
	payload.WorldPosition = origin + ray.Direction * hitDistance;
	payload.WorldNormal = glm::normalize(payload.WorldPosition);
	payload.WorldPosition += closest.Position;

	return payload;
}