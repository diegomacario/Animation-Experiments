#include <iostream>

#include "game.h"

#include "quat.h"

int main()
{
   testQuaternion();

   Game game;

   if (!game.initialize("Animation Experiments"))
   {
      std::cout << "Error - main - Failed to initialize the game" << "\n";
      return -1;
   }

   game.executeGameLoop();

   return 0;
}
