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

	if (m_FrameIndex == 1)
		memset(m_AccumulationData, 0.0f, m_FinalImage->GetWidth() * m_FinalImage->GetHeight() * sizeof(glm::vec4));

	std::for_each(std::execution::par, m_VerticalIter.begin(), m_VerticalIter.end(),
		[this](uint32_t y)
		{
			std::for_each(std::execution::par, m_HorizontalIter.begin(), m_HorizontalIter.end(),
			[this, y](uint32_t x)
				{
					glm::vec4 col = RayGen(x, y);
					m_AccumulationData[x + y * m_FinalImage->GetWidth()] += col;

					glm::vec4 accumulatedCol = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
					accumulatedCol /= (float)m_FrameIndex;

					accumulatedCol = glm::clamp(accumulatedCol, glm::vec4(0.f), glm::vec4(1.f));
					m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(accumulatedCol);
				});
		});

	m_FinalImage->SetData(m_ImageData);

	if (m_Settings.Accumulate)
		m_FrameIndex++;
	else
		m_FrameIndex = 1;
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
	ResetFrameIndex();

	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * height];

	delete[] m_AccumulationData;
	m_AccumulationData = new glm::vec4[width * height];

	m_HorizontalIter.resize(width);
	m_VerticalIter.resize(height);

	for (uint32_t i = 0; i < width; i++) { m_HorizontalIter[i] = i; }
	for (uint32_t i = 0; i < height; i++) { m_VerticalIter[i] = i; }
}

glm::vec4 Renderer::RayGen(uint32_t x, uint32_t y)
{
	Ray ray;
	ray.Origin = m_ActiveCamera->GetPosition();
	ray.Direction = m_ActiveCamera->GetRayDirections()[x + y * (float)m_FinalImage->GetWidth()];

	glm::vec3 out_col = glm::vec3(0.0f);
	float multiplier_falloff = 1.0f;
	for (uint32_t i = 0; i < bounces; i++)
	{
		Renderer::HitPayload payload = TraceRay(ray);
		if (payload.HitDistance < 0.0f)
		{
			glm::vec3 skyColor = glm::vec3(0.7f, 0.9f, 1.0f);
			out_col += skyColor * multiplier_falloff;
			break;
		}

		glm::vec3 light_dir(-1, -1, -1);
		light_dir = glm::normalize(light_dir);

		const Sphere& sphere = m_ActiveScene->Spheres[payload.objectIndex];
		const Material& material = m_ActiveScene->materials[sphere.MaterialIndex];

		float face_light = glm::max(glm::dot(payload.WorldNormal, -light_dir), 0.0f);
		auto albedo = material.Albedo;
		out_col += albedo * face_light * multiplier_falloff;
		multiplier_falloff *= 0.5;

		ray.Origin = payload.WorldPosition + FLT_MIN * payload.WorldNormal;
		ray.Direction = glm::reflect(ray.Direction,
			payload.WorldNormal + material.Roughness * Walnut::Random::Vec3(-0.5f, 0.5f));
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