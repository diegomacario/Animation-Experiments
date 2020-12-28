#ifndef INTERSECTION_H
#define INTERSECTION_H

#include "Ray.h"
#include "Triangle.h"

bool doesRayIntersectTriangle(const Ray& ray, const Triangle& triangle, glm::vec3& hitPoint);

#endif
