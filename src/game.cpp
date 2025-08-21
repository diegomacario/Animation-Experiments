#include <iostream>

#include "ModelViewerState.h"
#include "game.h"

Game::Game()
   : mModelViewerState()
   , mWindow()
{

}

Game::~Game()
{

}

bool Game::initialize(const std::string& title)
{
   // Initialize the window
   mWindow = std::make_shared<Window>(title);

   if (!mWindow->initialize())
   {
      std::cout << "Error - Game::initialize - Failed to initialize the window" << "\n";
      return false;
   }

    mModelViewerState = std::make_shared<ModelViewerState>(mWindow);

   return true;
}

void Game::executeGameLoop()
{
   double currentFrame = 0.0;
   double lastFrame    = 0.0;
   float  deltaTime    = 0.0f;

   while (!mWindow->shouldClose())
   {
      currentFrame = glfwGetTime();
      deltaTime    = static_cast<float>(currentFrame - lastFrame);
      lastFrame    = currentFrame;

      mModelViewerState->processInput(deltaTime);
      mModelViewerState->update(deltaTime);
      mModelViewerState->render();
   }
}
