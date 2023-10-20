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

struct Point {
	float x, y, z;
};

struct Triangle {
	Point points[3];
};

class Model : public SceneObject {
public:
	Model(const std::vector<Triangle>& triangles, glm::vec3 pos, int mat) :
		SceneObject{ pos, mat }, Triangles(triangles) {}

	IntersectResult RayIntersect(const Ray& ray) const override {
		IntersectResult closestIntersection{ FLT_MAX, glm::vec3(0.0f) };
		for (const Triangle& triangle : Triangles) {
			IntersectResult t = IntersectTriangle(ray, triangle);
			if (t.HitDistance > 0.0f && t.HitDistance < closestIntersection.HitDistance) {
				closestIntersection = t;
			}
		}
		return closestIntersection.HitDistance == FLT_MAX ? IntersectResult { -1.0f }  : closestIntersection;
	}

private:
	std::vector<Triangle> Triangles;

	IntersectResult IntersectTriangle(const Ray& ray, const Triangle& triangle) const {
		glm::vec3 v0(triangle.points[0].x, triangle.points[0].y, triangle.points[0].z);
		glm::vec3 v1(triangle.points[1].x, triangle.points[1].y, triangle.points[1].z);
		glm::vec3 v2(triangle.points[2].x, triangle.points[2].y, triangle.points[2].z);

		v0 += Position;
		v1 += Position;
		v2 += Position;

		glm::vec3 e1 = v1 - v0;
		glm::vec3 e2 = v2 - v0;

		// Calculate the normal of the triangle
		glm::vec3 h = glm::cross(ray.Direction, e2);
		float a = glm::dot(e1, h);

		if (std::abs(a) < 1e-6)
			return IntersectResult{ -1.0f };

		float f = 1.0f / a;
		glm::vec3 s = ray.Origin - v0;
		float u = f * glm::dot(s, h);

		if (u < 0.0f || u > 1.0f)
			return IntersectResult{ -1.0f };

		glm::vec3 q = glm::cross(s, e1);
		float v = f * glm::dot(ray.Direction, q);

		if (v < 0.0f || u + v > 1.0f)
			return IntersectResult{ -1.0f };

		float t = f * glm::dot(e2, q);

		if (t > 0.0f) {
			return IntersectResult{ t, h };
		}
		return IntersectResult{ -1.0f };
	}
};

struct Scene
{
	std::vector<std::unique_ptr<SceneObject>> Objects;
	std::vector<Material> materials;
};