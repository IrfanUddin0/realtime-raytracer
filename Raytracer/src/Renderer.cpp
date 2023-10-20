#include "Renderer.h"

namespace Utils {
	static uint32_t ConvertToRGBA(const glm::vec4& col)
	{
		return (int)(col.a * 255.f) << 24 | (int)(col.b * 255.f) << 16 | (int)(col.g * 255.f) << 8 | (int)(col.r * 255.f);
	}

	static uint32_t PCG_Hash(uint32_t input)
	{
		uint32_t state = input * 747796405u + 2891336453u;
		uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
		return (word >> 22u) ^ word;
	}

	static float RandomFloat(uint32_t& seed)
	{
		seed = PCG_Hash(seed);
		return (float)seed / (float)UINT32_MAX;
	}

	static glm::vec3 InUnitSphere(uint32_t& seed)
	{
		return glm::normalize(glm::vec3(
			RandomFloat(seed) * 2.0f - 1.0f,
			RandomFloat(seed) * 2.0f - 1.0f,
			RandomFloat(seed) * 2.0f - 1.0f));
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

	glm::vec3 light = glm::vec3(0.0f);
	glm::vec3 throughput(1.0f);

	uint32_t seed = x + y * (float)m_FinalImage->GetWidth();
	seed *= m_FrameIndex;

	for (uint32_t i = 0; i < bounces; i++)
	{
		Renderer::HitPayload payload = TraceRay(ray);
		if (payload.HitDistance < 0.0f)
		{
			glm::vec3 skyColor = glm::vec3(0.0f, 0.0f, 0.0f);
			light += skyColor * throughput;
			break;
		}

		const SceneObject& obj = *payload.object;
		const Material& material = m_ActiveScene->materials[obj.MaterialIndex];

		throughput *= material.Albedo;
		light += material.getEmission() * throughput;

		ray.Origin = payload.WorldPosition + FLT_MIN * payload.WorldNormal;
		//ray.Direction = glm::reflect(ray.Direction, payload.WorldNormal + material.Roughness * Utils::InUnitSphere(seed));

		ray.Direction = glm::normalize(payload.WorldNormal + Utils::InUnitSphere(seed));
	}

	return glm::vec4(light, 1.0f);
}

Renderer::HitPayload Renderer::TraceRay(const Ray& ray)
{
	const SceneObject* closest = nullptr;
	float hitDistance = FLT_MAX;

	for (size_t i = 0; i < m_ActiveScene->Objects.size(); i++)
	{
		const SceneObject* objPtr = m_ActiveScene->Objects[i].get();
		float t = objPtr->RayIntersect(ray);
		if (t > 0 && t < hitDistance)
		{
			hitDistance = t;
			closest = objPtr;
		}
	}

	if (closest == nullptr)
		return Miss(ray);

	return ClosestHit(ray, hitDistance, closest);	
}

Renderer::HitPayload Renderer::Miss(const Ray& ray)
{
	Renderer::HitPayload payload;
	payload.HitDistance = -1.0f;
	return payload;
}

Renderer::HitPayload Renderer::ClosestHit(const Ray& ray, float hitDistance, const SceneObject* object)
{
	Renderer::HitPayload payload;
	payload.HitDistance = hitDistance;
	payload.object = object;

	const SceneObject& closest = *object;
	glm::vec3 origin = ray.Origin - closest.Position;
	payload.WorldPosition = origin + ray.Direction * hitDistance;
	payload.WorldNormal = object->getNormalAtIntersection(ray, hitDistance);
	payload.WorldPosition += closest.Position;

	return payload;
}