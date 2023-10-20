#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <array>

#include "Ray.h"

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
	SceneObject(glm::vec3 pos, float rad, int mat) :
		Position(pos),Radius(rad),MaterialIndex(mat){}
	virtual float RayIntersect(const Ray& ray) const { return -1.0f; };

public:
	glm::vec3 Position{ 0.0f };
	float Radius = 0.5f;
	int MaterialIndex = 0;
};

class Sphere : public SceneObject
{
public:
	Sphere() {};
	Sphere(glm::vec3 pos, float rad, int mat) :
		SceneObject{pos, rad, mat} {}

	float RayIntersect(const Ray& ray) const override
	{
		const auto& sphere = *this;
		auto origin = ray.Origin - sphere.Position;

		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2 * glm::dot(origin, ray.Direction);
		float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

		float d = b * b - 4.f * a * c;
		if (d < 0)
			return -1.0f;

		return (-b - glm::sqrt(d)) / (2.f * a);
	}
};

struct Scene
{
	std::vector<std::unique_ptr<SceneObject>> Objects;
	std::vector<Material> materials;
};