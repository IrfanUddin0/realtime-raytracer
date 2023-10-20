#pragma once
#include "Walnut/Image.h"
#include "Walnut/Random.h"
#include <memory>
#include <glm/glm.hpp>
#include <execution>

#include "Camera.h"
#include "Ray.h"
#include "Scene.h"

class Renderer
{
public:
	struct Settings
	{
		bool Accumulate = true;
	};
	Renderer() = default;
	void Render(const Scene& scene, const Camera& camera);
	void OnResize(uint32_t width, uint32_t height);

	std::shared_ptr < Walnut::Image> GetFinalImage() const { return m_FinalImage; };
	void ResetFrameIndex() { m_FrameIndex = 1; }
	Settings& getSettings() { return m_Settings; }
	uint32_t& getBounces() { return bounces; }
	const uint32_t& getFrameIndex() { return m_FrameIndex; }
private:
	struct HitPayload
	{
		float HitDistance;
		glm::vec3 WorldPosition;
		glm::vec3 WorldNormal;
		const SceneObject* object;
	};
	glm::vec4 RayGen(uint32_t x, uint32_t y);
	HitPayload TraceRay(const Ray& ray);
	HitPayload ClosestHit(const Ray& ray, IntersectResult hitDistance, const SceneObject* object);
	HitPayload Miss(const Ray& ray);
private:
	Settings m_Settings;
	std::shared_ptr<Walnut::Image> m_FinalImage;
	uint32_t* m_ImageData = nullptr;
	glm::vec4* m_AccumulationData = nullptr;

	const Scene* m_ActiveScene = nullptr;
	const Camera* m_ActiveCamera = nullptr;

	uint32_t m_FrameIndex = 1;
	uint32_t bounces = 32;

	std::vector<uint32_t> m_HorizontalIter;
	std::vector<uint32_t> m_VerticalIter;
};

