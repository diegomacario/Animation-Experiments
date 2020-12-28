#include "Intersection.h"

/*
   To determine if a ray intersects a triangle, we first check if the ray intersects the plane in which the triangle lies
   If it does, we then check if the hit point lies within the triangle

   The equation of a plane is the following:

   (P - A) dot N = 0

   Where A is any point on the plane and N is the normal of the plane
   The equation is only satisfied when the vector (P - A) is perpendicular to N, which tells us that P is on the plane

   If we expand the equation, replace P with the equation of a ray, and solve for t, we obtain the following:

   (P dot N) - (A dot N) = 0

   (P dot N) = (A dot N)

   ((origin + direction * t) dot N) = (A dot N)

   (origin dot N) + ((direction * t) dot N) = (A dot N)

   ((direction * t) dot N) = (A dot N) - (origin dot N)

   t = ((A dot N) - (origin dot N)) / (direction dot N)

   The equation above gives us the distance along the ray to the point where it intersects the plane
   Notice that if the denominator (direction dot N) is equal to zero, the equation blows up
   That makes sense because the denominator is only equal to zero when the ray and the normal of the plane are perpendicular,
   or in other words, when the ray and the plane are parallel, which means that the ray doesn't intersect the plane

   Once we know the distance along the ray to the point where it intersects the plane, we can calculate the hit point by plugging it into the equation of the ray

   hitPoint = origin + (direction * t)

   And now the question becomes: does the hit point lie inside the triangle? We can answer that question using barycentric coordinates
   Consider this triangle:

         C
        / \
       /   \
      /     \
     /   P   \
    /         \
   A-----------B

   Barycentric coordinates can be used to express the position of any point located in the triangle with three scalars:

   P = (u * A) + (v * B) + (w * C)

   Where 0 <= u, v, w <= 1 and u + v + w = 1

   To derive the rules above, picture the triangle as a 2D coordinate system whose origin is point A and whose axes are vectors (B - A) and (C - A)
   In this coordinate system, any point in the triangle can be expressed as follows:

   P = A + (u * (B - A)) + (v * (C - A))

   Where 0 <= u, v <= 1 and u + v = 1

   If we expand the equation above we obtain the following:

   P = A + (u * B) - (u * A) + (v * C) - (v * A)

   P = ((1 - u - v) * A) + (u * B) + (v * C)

   P = (w * A) + (u * B) + (v * C)

   Where the third barycentric coordinate is given by:

   w = 1 - u - v

   To determine if the hit point lies inside the triangle we calculate the barycentric coordinates and check that they are valid
*/

bool doesRayIntersectTriangle(const Ray& ray, const Triangle& triangle, glm::vec3& hitPoint)
{
   // Calculate the denominator of the solved plane equation:
   // t = ((A dot N) - (origin dot N)) / (direction dot N)
   float rayDirDotNormal = glm::dot(ray.direction, triangle.normal);

   // If the ray is perpendicular to the normal of the triangle, the ray and the plane in which the triangle lies are parallel
   // TODO: Use constant here
   if (rayDirDotNormal < 0.0000001f)
   {
      return false;
   }

   // Evaluate the solved plane equation:
   // t = ((A dot N) - (origin dot N)) / (direction dot N)
   float distAlongRayToHit = (glm::dot(triangle.vertexA, triangle.normal) - glm::dot(ray.origin, triangle.normal)) / rayDirDotNormal;

   // If the distance along to the ray to the hit point is negative, the plane in which the triangle lies is behind the origin of the ray
   if (distAlongRayToHit < 0)
   {
      return false;
   }

   // Calculate the hit point
   hitPoint = (ray.direction * distAlongRayToHit) + ray.origin;

   // Check if P is to the left of each edge to determine if it lies inside the triangle
   /*
                C
               / \
              /   \
             /     \
            /   P   \
           /         \
          A-----------B
   */

   glm::vec3 normalOfTriABC = glm::cross((triangle.vertexB - triangle.vertexA), (triangle.vertexC - triangle.vertexA));

   // If P is to the left of edge (C - B), the normal of sub-triangle BCP will point in the same direction as the normal of triangle ABC
   // If P is to the right of edge (C - B), the normal of sub-triangle BCP will point in the opposite direction of the normal of triangle ABC
   glm::vec3 normalOfSubTri = glm::cross((triangle.vertexC - triangle.vertexB), (hitPoint - triangle.vertexB));
   if (dot(normalOfSubTri, normalOfTriABC) < 0)
   {
      return false; // P is to the right of edge (C - B)
   }

   // If P is to the left of edge (B - A), the normal of sub-triangle ABP will point in the same direction as the normal of triangle ABC
   // If P is to the right of edge (B - A), the normal of sub-triangle ABP will point in the opposite direction of the normal of triangle ABC
   normalOfSubTri = glm::cross((triangle.vertexB - triangle.vertexA), (hitPoint - triangle.vertexA));
   if (dot(normalOfSubTri, normalOfTriABC) < 0)
   {
      return false; // P is to the right of edge (B - A)
   }

   // If P is to the left of edge (A - C), the normal of sub-triangle CAP will point in the same direction as the normal of triangle ABC
   // If P is to the right of edge (A - C), the normal of sub-triangle CAP will point in the opposite direction of the normal of triangle ABC
   normalOfSubTri = glm::cross((triangle.vertexA - triangle.vertexC), (hitPoint - triangle.vertexC));
   if (dot(normalOfSubTri, normalOfTriABC) < 0)
   {
      return false; // P is to the right of edge (A - C)
   }

   // P lies inside triangle ABC
   return true;
}
