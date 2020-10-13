#include <iostream>

#include "shader_loader.h"
#include "texture_loader.h"
#include "model_loader.h"
#include "play_state.h"
#include "game.h"

Game::Game()
   : mFSM()
   , mWindow()
   , mCamera()
   , mModelManager()
   , mTextureManager()
   , mShaderManager()
   , mTable()
   , mTeapot()
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

   mCamera = std::make_shared<Camera>(glm::vec3(30.0f, 30.0f, 30.0f),
                                      glm::vec3(0.0f, 1.0f, 0.0f),
                                      -45.0f,
                                      -30.0f,
                                      45.0f,       // Fovy
                                      aspectRatio, // Aspect ratio
                                      0.1f,        // Near
                                      130.0f,      // Far
                                      20.0f,       // Movement speed
                                      0.1f);       // Mouse sensitivity

   // Initialize the 3D shader
   auto gameObj3DShader = mShaderManager.loadResource<ShaderLoader>("game_object_3D",
                                                                    "resources/shaders/game_object_3D.vs",
                                                                    "resources/shaders/game_object_3D.fs");
   gameObj3DShader->use();
   gameObj3DShader->setVec3("pointLights[0].worldPos", glm::vec3(0.0f, 0.0f, 100.0f));
   gameObj3DShader->setVec3("pointLights[0].color", glm::vec3(1.0f, 1.0f, 1.0f));
   gameObj3DShader->setFloat("pointLights[0].constantAtt", 1.0f);
   gameObj3DShader->setFloat("pointLights[0].linearAtt", 0.01f);
   gameObj3DShader->setFloat("pointLights[0].quadraticAtt", 0.0f);
   gameObj3DShader->setInt("numPointLightsInScene", 1);

   // Initialize the line shader
   auto lineShader = mShaderManager.loadResource<ShaderLoader>("line",
                                                               "resources/shaders/line.vs",
                                                               "resources/shaders/line.fs");

   // Load the models
   mModelManager.loadResource<ModelLoader>("table", "resources/models/table/table.obj");
   mModelManager.loadResource<ModelLoader>("teapot", "resources/models/teapot/teapot.obj");

   mTable = std::make_shared<GameObject3D>(mModelManager.getResource("table"),
                                           glm::vec3(0.0f, -1.96875f * (7.5f / 2.5f) * 2.5f, 0.0f),
                                           0.0f,
                                           glm::vec3(0.0f, 0.0f, 0.0f),
                                           1.0f);

   mTeapot = std::make_shared<GameObject3D>(mModelManager.getResource("teapot"),
                                            glm::vec3(0.0f),
                                            0.0f,
                                            glm::vec3(0.0f, 0.0f, 0.0f),
                                            7.5f / 2.5f);

   // Create the FSM
   mFSM = std::make_shared<FiniteStateMachine>();

   // Initialize the states
   std::unordered_map<std::string, std::shared_ptr<State>> mStates;

   mStates["play"] = std::make_shared<PlayState>(mFSM,
                                                 mWindow,
                                                 mCamera,
                                                 gameObj3DShader,
                                                 lineShader,
                                                 mTable,
                                                 mTeapot);

   // Initialize the FSM
   mFSM->initialize(std::move(mStates), "play");

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
