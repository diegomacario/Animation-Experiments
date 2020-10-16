#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include "game.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

int main(int argc, char* argv[])
{
   glm::mat4 myMat(1.0f);
   myMat = glm::translate(glm::mat4(1.0f), glm::vec3(7.0f, 8.0f, 9.0f));
   float m1 = myMat[0][0];
   float m2 = myMat[0][1];
   float m3 = myMat[0][2];
   float m4 = myMat[0][3];
   float m5 = myMat[1][0];
   float m6 = myMat[1][1];
   float m7 = myMat[1][2];
   float m8 = myMat[1][3];
   float m9 = myMat[2][0];
   float m10 = myMat[2][1];
   float m11 = myMat[2][2];
   float m12 = myMat[2][3];
   float m13 = myMat[3][0];
   float m14 = myMat[3][1];
   float m15 = myMat[3][2];
   float m16 = myMat[3][3];

   glm::vec4 col0 = myMat[0]; // Basis x
   glm::vec4 col1 = myMat[1]; // Basis y
   glm::vec4 col2 = myMat[2]; // Basis z
   glm::vec4 col3 = myMat[3];

   std::cout << col0.x << col0.y << col0.z << col0.w << '\n';
   std::cout << col1.x << col1.y << col1.z << col1.w << '\n';
   std::cout << col2.x << col2.y << col2.z << col2.w << '\n';
   std::cout << col3.x << col3.y << col3.z << col3.w << "\n\n";

   // w, x, y, z
   glm::quat myQuatA(4.0f, 1.0f, 2.0f, 3.0f);
   glm::quat myQuatB(8.0f, 5.0f, 6.0f, 7.0f);
   glm::quat resGlm = myQuatA * myQuatB;

   // x, y, z, w
   Q::quat myQuatAA(1.0f, 2.0f, 3.0f, 4.0f);
   Q::quat myQuatBB(5.0f, 6.0f, 7.0f, 8.0f);
   Q::quat resGab = myQuatAA * myQuatBB;
   Q::quat resGab2 = myQuatBB * myQuatAA;

   std::cout << "glm::quat AB: " << resGlm.x << " " << resGlm.y << " " << resGlm.z << " " << resGlm.w << '\n';
   std::cout << "quat AB:      " << resGab.x << " " << resGab.y << " " << resGab.z << " " << resGab.w << '\n';
   std::cout << "quat BA:      " << resGab2.x << " " << resGab2.y << " " << resGab2.z << " " << resGab2.w << "\n\n";

   Game game;

   if (!game.initialize("Animation-Experiments"))
   {
      std::cout << "Error - main - Failed to initialize the game" << "\n";
      return -1;
   }

   game.executeGameLoop();

   return 0;
}
