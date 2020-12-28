#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <glm/glm.hpp>

/*
         C
        / \
       /   \
      /     \
     /       \
    /         \
   A-----------B
*/

struct Triangle
{
   Triangle();
   Triangle(const glm::vec3& vA, const glm::vec3& vB, const glm::vec3& vC);

   // Vertices must be specified in a CCWISE order
   glm::vec3 vertexA, vertexB, vertexC;
   glm::vec3 normal;
};

#endif
