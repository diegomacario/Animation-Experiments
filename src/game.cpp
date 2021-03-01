#include <iostream>

#include "ModelViewerState.h"
#include "IKState.h"
#include "MovementState.h"
#include "IKMovementState.h"
#include "game.h"

Game::Game()
   : mFSM()
   , mWindow()
   , mCamera()
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

   // Initialize the camera
   float widthInPix = 1280.0f;
   float heightInPix = 720.0f;
   float aspectRatio = (widthInPix / heightInPix);

   mCamera = std::make_shared<Camera>(glm::vec3(0.0f, 0.0f, 0.0f),
                                      glm::vec3(0.0f, 0.0f, -1.0f),
                                      glm::vec3(0.0f, 1.0f, 0.0f),
                                      45.0f,       // Fovy
                                      aspectRatio, // Aspect ratio
                                      0.1f,        // Near
                                      130.0f,      // Far
                                      10.0f,       // Movement speed
                                      0.1f);       // Mouse sensitivity

   // Create the FSM
   mFSM = std::make_shared<FiniteStateMachine>();

   // Initialize the states
   std::unordered_map<std::string, std::shared_ptr<State>> mStates;

   mStates["viewer"] = std::make_shared<ModelViewerState>(mFSM,
                                                          mWindow,
                                                          mCamera);

   mStates["movement"] = std::make_shared<MovementState>(mFSM,
                                                         mWindow);

   mStates["ik"] = std::make_shared<IKState>(mFSM,
                                             mWindow,
                                             mCamera);

   mStates["ik_movement"] = std::make_shared<IKMovementState>(mFSM,
                                                              mWindow);

   // Initialize the FSM
   mFSM->initialize(std::move(mStates), "viewer");

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

      mFSM->processInputInCurrentState(deltaTime);
      mFSM->updateCurrentState(deltaTime);
      mFSM->renderCurrentState();
   }
}
