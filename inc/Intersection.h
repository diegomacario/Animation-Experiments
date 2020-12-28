#ifndef INTERSECTION_H
#define INTERSECTION_H

#include <vector>

#include "Ray.h"
#include "Triangle.h"
#include "AnimatedMesh.h"

bool DoesRayIntersectTriangle(const Ray& ray, const Triangle& triangle, glm::vec3& hitPoint);

std::vector<Triangle> GetTrianglesFromMesh(AnimatedMesh& mesh);
std::vector<Triangle> GetTrianglesFromMeshes(std::vector<AnimatedMesh>& mesh);

#endif
