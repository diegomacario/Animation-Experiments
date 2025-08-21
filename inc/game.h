#ifndef GAME_H
#define GAME_H

#include "window.h"
#include "ModelViewerState.h"

class Game
{
public:

   Game();
   ~Game();

   Game(const Game&) = delete;
   Game& operator=(const Game&) = delete;

   Game(Game&&) = delete;
   Game& operator=(Game&&) = delete;

   bool  initialize(const std::string& title);
   void  executeGameLoop();

private:

   std::shared_ptr<ModelViewerState> mModelViewerState;

   std::shared_ptr<Window>           mWindow;
};

#endif
