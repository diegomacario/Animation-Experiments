#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <array>
#include <random>

#include "play_state.h"

PlayState::PlayState(const std::shared_ptr<FiniteStateMachine>&     finiteStateMachine,
                     const std::shared_ptr<Window>&                 window,
                     const std::shared_ptr<Camera>&                 camera,
                     const std::shared_ptr<Shader>&                 gameObject3DShader,
                     const std::shared_ptr<Shader>&                 lineShader,
                     const std::shared_ptr<GameObject3D>&           table,
                     const std::shared_ptr<GameObject3D>&           teapot)
   : mFSM(finiteStateMachine)
   , mWindow(window)
   , mCamera(camera)
   , mGameObject3DShader(gameObject3DShader)
   , mLineShader(lineShader)
   , mTable(table)
   , mTeapot(teapot)
   , mWorldXAxis(glm::vec3(0.0f), glm::vec3(20.0f, 0.0f, 0.0f), glm::vec3(0.0f), 0.0f, glm::vec3(0.0f), 0.0f, glm::vec3(1.0f, 0.0f, 0.0f)) // Red
   , mWorldYAxis(glm::vec3(0.0f), glm::vec3(0.0f, 20.0f, 0.0f), glm::vec3(0.0f), 0.0f, glm::vec3(0.0f), 0.0f, glm::vec3(0.0f, 1.0f, 0.0f)) // Green
   , mWorldZAxis(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 20.0f), glm::vec3(0.0f), 0.0f, glm::vec3(0.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)) // Blue

   , mLocalXAxis(glm::vec3(0.0f), glm::vec3(16.0f, 0.0f, 0.0f), glm::vec3(0.0f), 0.0f, glm::vec3(0.0f), 0.0f, glm::vec3(1.0f, 1.0f, 0.0f)) //
   , mLocalYAxis(glm::vec3(0.0f), glm::vec3(0.0f, 16.0f, 0.0f), glm::vec3(0.0f), 0.0f, glm::vec3(0.0f), 0.0f, glm::vec3(0.0f, 1.0f, 1.0f)) //
   , mLocalZAxis(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 16.0f), glm::vec3(0.0f), 0.0f, glm::vec3(0.0f), 0.0f, glm::vec3(1.0f, 0.0f, 1.0f)) //
{

}

void PlayState::enter()
{
   resetCamera();
   resetScene();
}

void PlayState::processInput(float deltaTime)
{
   // Close the game
   if (mWindow->keyIsPressed(GLFW_KEY_ESCAPE)) { mWindow->setShouldClose(true); }

   // Make the game full screen or windowed
   if (mWindow->keyIsPressed(GLFW_KEY_F) && !mWindow->keyHasBeenProcessed(GLFW_KEY_F))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_F);
      mWindow->setFullScreen(!mWindow->isFullScreen());

      // In the play state, the following rules are applied to the cursor:
      // - Fullscreen: Cursor is always disabled
      // - Windowed with a free camera: Cursor is disabled
      // - Windowed with a fixed camera: Cursor is enabled
      if (mWindow->isFullScreen())
      {
         // Disable the cursor when fullscreen
         mWindow->enableCursor(false);
         if (mCamera->isFree())
         {
            // Going from windowed to fullscreen changes the position of the cursor, so we reset the first move flag to avoid a jump
            mWindow->resetFirstMove();
         }
      }
      else if (!mWindow->isFullScreen())
      {
         if (mCamera->isFree())
         {
            // Disable the cursor when windowed with a free camera
            mWindow->enableCursor(false);
            // Going from fullscreen to windowed changes the position of the cursor, so we reset the first move flag to avoid a jump
            mWindow->resetFirstMove();
         }
         else
         {
            // Enable the cursor when windowed with a fixed camera
            mWindow->enableCursor(true);
         }
      }
   }

   // Change the number of samples used for anti aliasing
   if (mWindow->keyIsPressed(GLFW_KEY_1) && !mWindow->keyHasBeenProcessed(GLFW_KEY_1))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_1);
      mWindow->setNumberOfSamples(1);
   }
   else if (mWindow->keyIsPressed(GLFW_KEY_2) && !mWindow->keyHasBeenProcessed(GLFW_KEY_2))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_2);
      mWindow->setNumberOfSamples(2);
   }
   else if (mWindow->keyIsPressed(GLFW_KEY_4) && !mWindow->keyHasBeenProcessed(GLFW_KEY_4))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_4);
      mWindow->setNumberOfSamples(4);
   }
   else if (mWindow->keyIsPressed(GLFW_KEY_8) && !mWindow->keyHasBeenProcessed(GLFW_KEY_8))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_8);
      mWindow->setNumberOfSamples(8);
   }

   // Reset the camera
   if (mWindow->keyIsPressed(GLFW_KEY_R)) { resetCamera(); }

   // Make the camera free or fixed
   if (mWindow->keyIsPressed(GLFW_KEY_C) && !mWindow->keyHasBeenProcessed(GLFW_KEY_C))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_C);
      mCamera->setFree(!mCamera->isFree());

      if (!mWindow->isFullScreen())
      {
         if (mCamera->isFree())
         {
            // Disable the cursor when windowed with a free camera
            mWindow->enableCursor(false);
         }
         else
         {
            // Enable the cursor when windowed with a fixed camera
            mWindow->enableCursor(true);
         }
      }

      mWindow->resetMouseMoved();
   }

   // Move and orient the camera
   if (mCamera->isFree())
   {
      // Move
      if (mWindow->keyIsPressed(GLFW_KEY_W)) { mCamera->processKeyboardInput(Camera::MovementDirection::Forward, deltaTime); }
      if (mWindow->keyIsPressed(GLFW_KEY_S)) { mCamera->processKeyboardInput(Camera::MovementDirection::Backward, deltaTime); }
      if (mWindow->keyIsPressed(GLFW_KEY_A)) { mCamera->processKeyboardInput(Camera::MovementDirection::Left, deltaTime); }
      if (mWindow->keyIsPressed(GLFW_KEY_D)) { mCamera->processKeyboardInput(Camera::MovementDirection::Right, deltaTime); }

      // Orient
      if (mWindow->mouseMoved())
      {
         mCamera->processMouseMovement(mWindow->getCursorXOffset(), mWindow->getCursorYOffset());
         mWindow->resetMouseMoved();
      }

      // Zoom
      if (mWindow->scrollWheelMoved())
      {
         mCamera->processScrollWheelMovement(mWindow->getScrollYOffset());
         mWindow->resetScrollWheelMoved();
      }
   }
}

void PlayState::update(float deltaTime)
{

}

//bool show_demo_window = true;
//bool show_another_window = false;
//ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

void PlayState::rotateSceneByMultiplyingCurrentRotationFromTheLeft(const quat& rot)
{
   mTeapot->rotateByMultiplyingCurrentRotationFromTheLeft(rot);
   mLocalXAxis.rotateByMultiplyingCurrentRotationFromTheLeft(rot);
   mLocalYAxis.rotateByMultiplyingCurrentRotationFromTheLeft(rot);
   mLocalZAxis.rotateByMultiplyingCurrentRotationFromTheLeft(rot);
}

void PlayState::rotateSceneByMultiplyingCurrentRotationFromTheRight(const quat& rot)
{
   mTeapot->rotateByMultiplyingCurrentRotationFromTheRight(rot);
   mLocalXAxis.rotateByMultiplyingCurrentRotationFromTheRight(rot);
   mLocalYAxis.rotateByMultiplyingCurrentRotationFromTheRight(rot);
   mLocalZAxis.rotateByMultiplyingCurrentRotationFromTheRight(rot);
}

void PlayState::render()
{
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();

   //if (show_demo_window)
      //ImGui::ShowDemoWindow(&show_demo_window);

   {
      //static float f = 0.0f;
      //static int counter = 0;

      ImGui::Begin("Rotation Controller");                    // Create a window called "Rotation Controller" and append into it.

      //ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
      //ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
      //ImGui::Checkbox("Another Window", &show_another_window);

      //ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
      //ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

      //if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
      //    counter++;
      //ImGui::SameLine();
      //ImGui::Text("counter = %d", counter);

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

      ImGui::Spacing();
      ImGui::Spacing();
      ImGui::Spacing();

      if (ImGui::Button("Reset rotation"))
      {
         mTeapot->setRotation(quat());
         mLocalXAxis.setRotation(quat());
         mLocalYAxis.setRotation(quat());
         mLocalZAxis.setRotation(quat());
      }

      ImGui::Spacing();
      ImGui::Spacing();
      ImGui::Spacing();

      if (ImGui::Button("Rotate by 45 degrees around Y from the left"))
      {
         quat rot = angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
         rotateSceneByMultiplyingCurrentRotationFromTheLeft(rot);
      }

      if (ImGui::Button("Rotate by 45 degrees around Z from the left"))
      {
         quat rot = angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
         rotateSceneByMultiplyingCurrentRotationFromTheLeft(rot);
      }

      if (ImGui::Button("Rotate by 45 degrees around Y from the right"))
      {
         quat rot = angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
         rotateSceneByMultiplyingCurrentRotationFromTheRight(rot);
      }

      if (ImGui::Button("Rotate by 90 degrees around Z from the right"))
      {
         quat rot = angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
         rotateSceneByMultiplyingCurrentRotationFromTheRight(rot);
      }

      if (ImGui::Button("Rotate by 45 degrees around (1, 1, 1) from the left"))
      {
         quat rot = angleAxis(glm::radians(45.0f), glm::vec3(1.0f, 1.0f, 1.0f));
         rotateSceneByMultiplyingCurrentRotationFromTheLeft(rot);
      }

      if (ImGui::Button("Rotate by 90 degrees around X from the right"))
      {
         quat rot = angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
         rotateSceneByMultiplyingCurrentRotationFromTheRight(rot);
      }

      if (ImGui::Button("Rotate by 90 degrees around X from the left"))
      {
         quat rot = angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
         rotateSceneByMultiplyingCurrentRotationFromTheLeft(rot);
      }

      if (ImGui::Button("Qp (90 Z) * Qch (90 X) * Teapot"))
      {
         quat rotZParent = angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
         quat rotXChild  = angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
         quat rot = rotZParent * rotXChild;
         rotateSceneByMultiplyingCurrentRotationFromTheLeft(rot);
      }

      if (ImGui::Button("Qch (90 X) * Qp (90 Z) * Teapot"))
      {
         quat rotZParent = angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
         quat rotXChild  = angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
         quat rot =  rotXChild * rotZParent;
         rotateSceneByMultiplyingCurrentRotationFromTheLeft(rot);
      }

      ImGui::Spacing();
      ImGui::Spacing();
      ImGui::Spacing();

      static float fwd[3] = { 1.0f, 1.0f, -1.0f };
      static float up[3] = { 0.0f, 1.0f, 0.0f };
      ImGui::InputFloat3("Forward Vector", fwd);
      ImGui::InputFloat3("Up Vector", up);
      if (ImGui::Button("Look Rotate"))
      {
         quat lookRot = lookRotation(glm::vec3(fwd[0], fwd[1], fwd[2]), glm::vec3(up[0], up[1], up[2]));

         //mTeapot->setRotation(lookRot);
         //mLocalXAxis.setRotation(lookRot);
         //mLocalYAxis.setRotation(lookRot);
         //mLocalZAxis.setRotation(lookRot);
         rotateSceneByMultiplyingCurrentRotationFromTheLeft(lookRot);
      }

      ImGui::Spacing();
      ImGui::Spacing();
      ImGui::Spacing();

      static float from[3] = { 0.0f, 0.0f, 1.0f };
      static float to[3] = { 1.0f, 0.0f, -1.0f };
      ImGui::InputFloat3("From Vector", from);
      ImGui::InputFloat3("To Vector", to);
      if (ImGui::Button("From To"))
      {
         quat fromToRot = fromTo(glm::vec3(from[0], from[1], from[2]), glm::vec3(to[0], to[1], to[2]));

         //mTeapot->setRotation(fromToRot);
         //mLocalXAxis.setRotation(fromToRot);
         //mLocalYAxis.setRotation(fromToRot);
         //mLocalZAxis.setRotation(fromToRot);
         rotateSceneByMultiplyingCurrentRotationFromTheLeft(fromToRot);
      }

      ImGui::Spacing();
      ImGui::Spacing();
      ImGui::Spacing();

      static bool enableInterpolation = false;
      static const char* interpolationAlgorithms[] = {"Mix", "NLerp", "Slerp"};
      static int selectedItem = 0;
      static float t = 0.0f;
      ImGui::Checkbox("Enable Interpolation Mode", &enableInterpolation);
      ImGui::Combo("Interpolation Algo", &selectedItem, interpolationAlgorithms, 3);
      ImGui::SliderFloat("Interpolation Val", &t, 0.0f, 1.0f);
      if (enableInterpolation)
      {
         quat start = angleAxis(glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
         quat end   = angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * angleAxis(glm::radians(135.0f), glm::vec3(0.0f, 1.0f, 0.0f));
         quat rot;
         switch (selectedItem)
         {
         case 0:
            rot = mix(start, end, t);
            break;
         case 1:
            rot = nlerp(start, end, t);
            break;
         case 2:
            rot = slerp(start, end, t);
            break;
         default:
            std::cout << "Unknown combo box value!" << '\n';
         }

         mTeapot->setRotation(rot);
         mLocalXAxis.setRotation(rot);
         mLocalYAxis.setRotation(rot);
         mLocalZAxis.setRotation(rot);
      }

      ImGui::End();
   }

   // 3. Show another simple window.
   //if (show_another_window)
   //{
   //   ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
   //   ImGui::Text("Hello from another window!");
   //   if (ImGui::Button("Close Me"))
   //      show_another_window = false;
   //   ImGui::End();
   //}

   mWindow->clearAndBindMultisampleFramebuffer();

   // Enable depth testing for 3D objects
   glEnable(GL_DEPTH_TEST);

   mLineShader->use();
   mLineShader->setMat4("projectionView", mCamera->getPerspectiveProjectionViewMatrix());

   mWorldXAxis.render(*mLineShader);
   mWorldYAxis.render(*mLineShader);
   mWorldZAxis.render(*mLineShader);

   mLocalXAxis.render(*mLineShader);
   mLocalYAxis.render(*mLineShader);
   mLocalZAxis.render(*mLineShader);

   mGameObject3DShader->use();
   mGameObject3DShader->setMat4("projectionView", mCamera->getPerspectiveProjectionViewMatrix());
   mGameObject3DShader->setVec3("cameraPos", mCamera->getPosition());

   mTable->render(*mGameObject3DShader);

   // Disable face culling so that we render the inside of the teapot
   glDisable(GL_CULL_FACE);
   mTeapot->render(*mGameObject3DShader);
   glEnable(GL_CULL_FACE);

   ImGui::Render();
   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

   mWindow->generateAntiAliasedImage();

   mWindow->swapBuffers();
   mWindow->pollEvents();
}

void PlayState::exit()
{

}

void PlayState::resetScene()
{

}

void PlayState::resetCamera()
{
   mCamera->reposition(glm::vec3(30.0f, 30.0f, 30.0f),
                       glm::vec3(0.0f, 1.0f, 0.0f),
                       -45.0f,
                       -30.0f,
                       45.0f);
}
