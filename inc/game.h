#ifndef GAME_H
#define GAME_H

#ifndef USE_THIRD_PERSON_CAMERA
#include "camera.h"
#endif
#include "window.h"
#include "state.h"
#include "finite_state_machine.h"

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

#ifdef __EMSCRIPTEN__
   void  updateWindowDimensions(int width, int height);
#endif

private:

   std::shared_ptr<FiniteStateMachine>     mFSM;

   std::shared_ptr<Window>                 mWindow;

#ifndef USE_THIRD_PERSON_CAMERA
   std::shared_ptr<Camera>                 mCamera;
#endif
};

#endif
