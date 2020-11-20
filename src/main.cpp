#include <iostream>

#include "game.h"

int main(int argc, char* argv[])
{
   Game game;

   if (!game.initialize("Animation Experiments"))
   {
      std::cout << "Error - main - Failed to initialize the game" << "\n";
      return -1;
   }

   game.executeGameLoop();

   return 0;
}
