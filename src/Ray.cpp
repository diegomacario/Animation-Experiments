#include "Ray.h"

Ray::Ray()
   : origin(0.0f)
   , direction(0.0f)
{

}

Ray::Ray(const glm::vec3& ori, const glm::vec3& dir)
   : origin(ori)
   , direction(dir)
{

}
