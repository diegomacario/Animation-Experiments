#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include "game.h"

#ifdef __EMSCRIPTEN__
void gameLoop(void* game)
{
   static_cast<Game*>(game)->executeGameLoop();
}
#endif

int main()
{
   Game game;

   if (!game.initialize("Animation Experiments"))
   {
      std::cout << "Error - main - Failed to initialize the game" << "\n";
      return -1;
   }

#ifdef __EMSCRIPTEN__
   emscripten_set_main_loop_arg(gameLoop, static_cast<void*>(&game), 0, true);
#else
   game.executeGameLoop();
#endif

   return 0;
}
