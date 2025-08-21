#include <iostream>

#include "game.h"

#include "Transform.h"

int main()
{
   testTransform();

   Game game;

   if (!game.initialize("Animation Experiments"))
   {
      std::cout << "Error - main - Failed to initialize the game" << "\n";
      return -1;
   }

   game.executeGameLoop();

   return 0;
}
