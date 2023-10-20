#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <array>

#include "Ray.h"

struct IntersectResult
{
	float HitDistance;
	glm::vec3 HitSurfaceNormal;
};

struct Material
{
	glm::vec3 Albedo{ 1.0f, 1.0f, 1.0f };
	float Roughness = 1.0f;
	float Metallic = 0.0f;

	glm::vec3 EmissionColor{ 0.0f };
	float EmissionPower = 0.0f;

	glm::vec3 getEmission() const { return EmissionColor * EmissionPower; }
};

class SceneObject
{
public:
	SceneObject(){}
	SceneObject(glm::vec3 pos, int mat) :
		Position(pos),MaterialIndex(mat){}
	virtual IntersectResult RayIntersect(const Ray& ray) const = 0;

public:
	glm::vec3 Position{ 0.0f };
	int MaterialIndex = 0;
};

class Sphere : public SceneObject
{
public:
	Sphere() {};
	Sphere(glm::vec3 pos, float rad, int mat) :
		SceneObject{pos, mat}, Radius(rad) {}

	IntersectResult RayIntersect(const Ray& ray) const override
	{
		const auto& sphere = *this;
		auto origin = ray.Origin - sphere.Position;

		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2 * glm::dot(origin, ray.Direction);
		float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

		float d = b * b - 4.f * a * c;
		if (d < 0)
			return IntersectResult{ -1.0f };

		float t = (-b - glm::sqrt(d)) / (2.f * a);
		return IntersectResult{ t,  glm::normalize(origin + ray.Direction * t) };
	}

public:
	float Radius = 1.0f;
};

class Plane : public SceneObject
{
public:
	Plane() {};
	Plane(glm::vec3 pos, glm::vec3 normal, int mat) :
		SceneObject{ pos, mat }, Normal(glm::normalize(normal)) {}

	IntersectResult RayIntersect(const Ray& ray) const override
	{
		const auto& plane = *this;
		float denom = glm::dot(plane.Normal, ray.Direction);

		if (glm::abs(denom) < 1e-6)
		{
			return IntersectResult{ -1.0f };
		}

		float t = glm::dot(plane.Position - ray.Origin, plane.Normal) / denom;

		if (t < 0.0f)
		{
			return IntersectResult{ -1.0f };
		}

		return IntersectResult{ t, Normal };
	}

public:
	glm::vec3 Normal{ 0.0f, 1.0f, 0.0f };
};

struct Scene
{
	std::vector<std::unique_ptr<SceneObject>> Objects;
	std::vector<Material> materials;
};