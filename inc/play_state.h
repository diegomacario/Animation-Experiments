#ifndef PLAY_STATE_H
#define PLAY_STATE_H

#include <array>

#include "game.h"
#include "line.h"

class PlayState : public State
{
public:

   PlayState(const std::shared_ptr<FiniteStateMachine>&     finiteStateMachine,
             const std::shared_ptr<Window>&                 window,
             const std::shared_ptr<Camera>&                 camera,
             const std::shared_ptr<Shader>&                 gameObject3DShader,
             const std::shared_ptr<Shader>&                 lineShader,
             const std::shared_ptr<GameObject3D>&           table,
             const std::shared_ptr<GameObject3D>&           teapot);
   ~PlayState() = default;

   PlayState(const PlayState&) = delete;
   PlayState& operator=(const PlayState&) = delete;

   PlayState(PlayState&&) = delete;
   PlayState& operator=(PlayState&&) = delete;

   void enter() override;
   void processInput(float deltaTime) override;
   void update(float deltaTime) override;
   void render() override;
   void exit() override;

private:

   void resetScene();

   void resetCamera();

   void rotateSceneByMultiplyingCurrentRotationFromTheLeft(const Q::quat& rot);
   void rotateSceneByMultiplyingCurrentRotationFromTheRight(const Q::quat& rot);

   std::shared_ptr<FiniteStateMachine>     mFSM;

   std::shared_ptr<Window>                 mWindow;

   std::shared_ptr<Camera>                 mCamera;

   std::shared_ptr<Shader>                 mGameObject3DShader;
   std::shared_ptr<Shader>                 mLineShader;

   std::shared_ptr<GameObject3D>           mTable;
   std::shared_ptr<GameObject3D>           mTeapot;

   Line                                    mWorldXAxis;
   Line                                    mWorldYAxis;
   Line                                    mWorldZAxis;

   Line                                    mLocalXAxis;
   Line                                    mLocalYAxis;
   Line                                    mLocalZAxis;
};

#endif
