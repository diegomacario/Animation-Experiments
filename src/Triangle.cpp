#include "Triangle.h"

Triangle::Triangle()
{

}

Triangle::Triangle(const glm::vec3& vA, const glm::vec3& vB, const glm::vec3& vC)
   : vertexA(vA)
   , vertexB(vB)
   , vertexC(vC)
{
   normal = glm::normalize(glm::cross(vertexB - vertexA, vertexC - vertexA));
}
