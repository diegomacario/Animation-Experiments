#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <glm/gtc/matrix_transform.hpp>

#include "resource_manager.h"
#include "shader_loader.h"
#include "TrackVisualizer.h"

TrackVisualizer::TrackVisualizer()
   : mWidthOfGraphSpace(100.0f)
   , mHeightOfGraphSpace(100.0f)
   , mNumGraphs(0)
   , mNumTiles(0)
   , mTileWidth(0.0f)
   , mTileHeight(0.0f)
   , mTileHorizontalOffset(0.0f)
   , mTileVerticalOffset(0.0f)
   , mGraphWidth(0.0f)
   , mGraphHeight(0.0f)
   , mSlopeLineScalingFactor(1.0f)
   , mReferenceLinesVAO(0)
   , mReferenceLinesVBO(0)
   , mTrackLinesVAOs{0, 0, 0, 0}
   , mTrackLinesVBOs{0, 0, 0, 0}
   , mKeyframePointsVAO(0)
   , mKeyframePointsVBO(0)
   , mSlopeLinesVAO(0)
   , mSlopeLinesVBO(0)
   , mTracks()
   , mReferenceLines()
   , mTrackLines()
   , mKeyframePoints()
   , mSlopeLines()
   , mTrackLinesColorPalette{glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.65f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)}
{
   glGenVertexArrays(1, &mReferenceLinesVAO);
   glGenBuffers(1, &mReferenceLinesVBO);

   glGenVertexArrays(4, mTrackLinesVAOs);
   glGenBuffers(4, mTrackLinesVBOs);

   glGenVertexArrays(1, &mKeyframePointsVAO);
   glGenBuffers(1, &mKeyframePointsVBO);

   glGenVertexArrays(1, &mSlopeLinesVAO);
   glGenBuffers(1, &mSlopeLinesVBO);

   mTrackShader = ResourceManager<Shader>().loadUnmanagedResource<ShaderLoader>("resources/shaders/graph.vert",
                                                                                "resources/shaders/graph.frag");
}

TrackVisualizer::~TrackVisualizer()
{
   glDeleteVertexArrays(1, &mReferenceLinesVAO);
   glDeleteBuffers(1, &mReferenceLinesVBO);

   glDeleteVertexArrays(4, mTrackLinesVAOs);
   glDeleteBuffers(4, mTrackLinesVBOs);

   glDeleteVertexArrays(1, &mKeyframePointsVAO);
   glDeleteBuffers(1, &mKeyframePointsVBO);

   glDeleteVertexArrays(1, &mSlopeLinesVAO);
   glDeleteBuffers(1, &mSlopeLinesVBO);
}

void TrackVisualizer::setTracks(std::vector<FastTransformTrack>& tracks)
{
   for (int i = 0; i < tracks.size(); ++i)
   {
      mTracks.push_back(tracks[i].GetRotationTrack());
   }

   mNumGraphs = static_cast<unsigned int>(mTracks.size());
   mNumTiles = static_cast<unsigned int>(glm::sqrt(mNumGraphs));
   while (mNumGraphs > (mNumTiles * mNumTiles))
   {
      ++mNumTiles;
   }

   mTileWidth = (mWidthOfGraphSpace * (1280.0f / 720.0f)) / mNumTiles;
   mTileHeight = mHeightOfGraphSpace / mNumTiles;
   mTileHorizontalOffset = mTileWidth * 0.1f * (720.0f / 1280.0f);
   mTileVerticalOffset = mTileHeight * 0.1f;
   mGraphWidth = mTileWidth - mTileHorizontalOffset;
   mGraphHeight = mTileHeight - mTileVerticalOffset;
   mSlopeLineScalingFactor = mTileWidth * 0.05f;

   initializeReferenceLines();
   initializeTrackLines();
   initializeKeyframePointsAndSlopeLines();

   // Load buffers

   glBindVertexArray(mReferenceLinesVAO);
   glBindBuffer(GL_ARRAY_BUFFER, mReferenceLinesVBO);
   glBufferData(GL_ARRAY_BUFFER, mReferenceLines.size() * sizeof(glm::vec3), &mReferenceLines[0], GL_STATIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindVertexArray(0);

   for (int i = 0; i < 4; ++i)
   {
      glBindVertexArray(mTrackLinesVAOs[i]);
      glBindBuffer(GL_ARRAY_BUFFER, mTrackLinesVBOs[i]);
      glBufferData(GL_ARRAY_BUFFER, mTrackLines[i].size() * sizeof(glm::vec3), &(mTrackLines[i][0]), GL_STATIC_DRAW);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindVertexArray(0);
   }

   glBindVertexArray(mKeyframePointsVAO);
   glBindBuffer(GL_ARRAY_BUFFER, mKeyframePointsVBO);
   glBufferData(GL_ARRAY_BUFFER, mKeyframePoints.size() * sizeof(glm::vec3), &mKeyframePoints[0], GL_STATIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindVertexArray(0);

   glBindVertexArray(mSlopeLinesVAO);
   glBindBuffer(GL_ARRAY_BUFFER, mSlopeLinesVBO);
   glBufferData(GL_ARRAY_BUFFER, mSlopeLines.size() * sizeof(glm::vec3), &mSlopeLines[0], GL_STATIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindVertexArray(0);

   // Configure VAOs

   int posAttribLocation = mTrackShader->getAttributeLocation("position");

   glBindVertexArray(mReferenceLinesVAO);
   glBindBuffer(GL_ARRAY_BUFFER, mReferenceLinesVBO);
   glEnableVertexAttribArray(posAttribLocation);
   glVertexAttribPointer(posAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindVertexArray(0);

   for (int i = 0; i < 4; ++i)
   {
      glBindVertexArray(mTrackLinesVAOs[i]);
      glBindBuffer(GL_ARRAY_BUFFER, mTrackLinesVBOs[i]);
      glEnableVertexAttribArray(posAttribLocation);
      glVertexAttribPointer(posAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindVertexArray(0);
   }

   glBindVertexArray(mKeyframePointsVAO);
   glBindBuffer(GL_ARRAY_BUFFER, mKeyframePointsVBO);
   glEnableVertexAttribArray(posAttribLocation);
   glVertexAttribPointer(posAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindVertexArray(0);

   glBindVertexArray(mSlopeLinesVAO);
   glBindBuffer(GL_ARRAY_BUFFER, mSlopeLinesVBO);
   glEnableVertexAttribArray(posAttribLocation);
   glVertexAttribPointer(posAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindVertexArray(0);
}

void TrackVisualizer::render()
{
   mTrackShader->use(true);

   glm::vec3 eye    = glm::vec3(0.0f, 0.0f, 5.0f);
   glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
   glm::vec3 up     = glm::vec3(0.0f, 1.0f, 0.0f);
   glm::mat4 view   = glm::lookAt(eye, center, up);

   float left   = 0.0f;
   float right  = mWidthOfGraphSpace * (1280.0f / 720.0f);
   float bottom = 0.0f;
   float top    = mWidthOfGraphSpace;
   float near   = 0.001f;
   float far    = 10.0f;
   glm::mat4 projection = glm::ortho(left, right, bottom, top, near, far);
   mTrackShader->setUniformMat4("projectionView", projection * view);

   mTrackShader->setUniformVec3("color", glm::vec3(1.0f, 1.0f, 1.0f));
   glBindVertexArray(mReferenceLinesVAO);
   glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mReferenceLines.size()));
   glBindVertexArray(0);

   for (int i = 0; i < 4; ++i)
   {
      mTrackShader->setUniformVec3("color", mTrackLinesColorPalette[i]);
      glBindVertexArray(mTrackLinesVAOs[i]);
      glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mTrackLines[i].size()));
      glBindVertexArray(0);
   }

#ifndef __EMSCRIPTEN__
   glPointSize(5.0f);
#endif
   mTrackShader->setUniformVec3("color", glm::vec3(0.0f, 0.0f, 1.0f));
   glBindVertexArray(mKeyframePointsVAO);
   //glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(mKeyframePoints.size()));
   glBindVertexArray(0);
#ifndef __EMSCRIPTEN__
   glPointSize(1.0f);
#endif

   mTrackShader->setUniformVec3("color", glm::vec3(1.0f, 0.0f, 0.0f));
   glBindVertexArray(mSlopeLinesVAO);
   //glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mSlopeLines.size()));
   glBindVertexArray(0);

   mTrackShader->use(false);
}

void TrackVisualizer::initializeReferenceLines()
{
   int trackIndex = 0;
   for (int j = 0; j < mNumTiles; ++j)
   {
      float yPosOfOriginOfGraph = (mTileHeight * j) + (mTileVerticalOffset / 2.0f);

      for (int i = 0; i < mNumTiles; ++i)
      {
         if (trackIndex >= mNumGraphs)
         {
            // Uncomment this line if you don't want to see reference lines for empty graphs
            //break;
         }

         float xPosOfOriginOfGraph = (mTileWidth * i) + (mTileHorizontalOffset / 2.0f);

         // Y axis
         mReferenceLines.push_back(glm::vec3(xPosOfOriginOfGraph, yPosOfOriginOfGraph, 0.1f));
         mReferenceLines.push_back(glm::vec3(xPosOfOriginOfGraph, yPosOfOriginOfGraph + mGraphHeight, 0.1f));

         // X axis
         mReferenceLines.push_back(glm::vec3(xPosOfOriginOfGraph, yPosOfOriginOfGraph, 0.1f));
         mReferenceLines.push_back(glm::vec3(xPosOfOriginOfGraph + mGraphWidth, yPosOfOriginOfGraph, 0.1f));

         ++trackIndex;
      }
   }
}

void TrackVisualizer::initializeTrackLines()
{
   int trackIndex = 0;
   for (int j = 0; j < mNumTiles; ++j)
   {
      float yPosOfOriginOfGraph = (mTileHeight * j) + (mTileVerticalOffset / 2.0f);

      for (int i = 0; i < mNumTiles; ++i)
      {
         if (trackIndex >= mNumGraphs)
         {
            break;
         }

         float xPosOfOriginOfGraph = (mTileWidth * i) + (mTileHorizontalOffset / 2.0f);

         float trackDuration = mTracks[trackIndex].GetEndTime() - mTracks[trackIndex].GetStartTime();

         glm::vec4 minSamples = glm::vec4(std::numeric_limits<float>::max());
         glm::vec4 maxSamples = glm::vec4(std::numeric_limits<float>::lowest());
         for (unsigned int sampleIndex = 0; sampleIndex < 150; ++sampleIndex)
         {
            float sampleIndexNormalized = static_cast<float>(sampleIndex) / 149.0f;

            Q::quat sample = mTracks[trackIndex].Sample(sampleIndexNormalized * trackDuration, false);

            if (sample.x < minSamples.x) minSamples.x = sample.x;
            if (sample.y < minSamples.y) minSamples.y = sample.y;
            if (sample.z < minSamples.z) minSamples.z = sample.z;
            if (sample.w < minSamples.w) minSamples.w = sample.w;
            if (sample.x > maxSamples.x) maxSamples.x = sample.x;
            if (sample.y > maxSamples.y) maxSamples.y = sample.y;
            if (sample.z > maxSamples.z) maxSamples.z = sample.z;
            if (sample.w > maxSamples.w) maxSamples.w = sample.w;
         }

         for (unsigned int sampleIndex = 1; sampleIndex < 150; ++sampleIndex)
         {
            float currSampleIndexNormalized = static_cast<float>(sampleIndex - 1) / 149.0f;
            float nextSampleIndexNormalized = static_cast<float>(sampleIndex) / 149.0f;

            float currX = xPosOfOriginOfGraph + (currSampleIndexNormalized * mGraphWidth);
            float nextX = xPosOfOriginOfGraph + (nextSampleIndexNormalized * mGraphWidth);

            Q::quat currSampleQuat = mTracks[trackIndex].Sample(currSampleIndexNormalized * trackDuration, false);
            Q::quat nextSampleQuat = mTracks[trackIndex].Sample(nextSampleIndexNormalized * trackDuration, false);

            glm::vec4 currSample = glm::vec4(currSampleQuat.x, currSampleQuat.y, currSampleQuat.z, currSampleQuat.w);
            glm::vec4 nextSample = glm::vec4(nextSampleQuat.x, nextSampleQuat.y, nextSampleQuat.z, nextSampleQuat.w);

            glm::vec4 currSampleNormalized, nextSampleNormalized;
            for (int k = 0; k < 4; ++k)
            {
               currSampleNormalized[k] = yPosOfOriginOfGraph + (((currSample[k] - minSamples[k]) / (maxSamples[k] - minSamples[k])) * mGraphHeight);
               nextSampleNormalized[k] = yPosOfOriginOfGraph + (((nextSample[k] - minSamples[k]) / (maxSamples[k] - minSamples[k])) * mGraphHeight);
            }

            mTrackLines[0].push_back(glm::vec3(currX, currSampleNormalized.x, 0.0f));
            mTrackLines[0].push_back(glm::vec3(nextX, nextSampleNormalized.x, 0.0f));

            mTrackLines[1].push_back(glm::vec3(currX, currSampleNormalized.y, 0.0f));
            mTrackLines[1].push_back(glm::vec3(nextX, nextSampleNormalized.y, 0.0f));

            mTrackLines[2].push_back(glm::vec3(currX, currSampleNormalized.z, 0.0f));
            mTrackLines[2].push_back(glm::vec3(nextX, nextSampleNormalized.z, 0.0f));

            mTrackLines[3].push_back(glm::vec3(currX, currSampleNormalized.w, 0.0f));
            mTrackLines[3].push_back(glm::vec3(nextX, nextSampleNormalized.w, 0.0f));
         }

         ++trackIndex;
      }
   }
}

void TrackVisualizer::initializeKeyframePointsAndSlopeLines()
{
   int trackIndex = 0;
   for (int j = 0; j < mNumTiles; ++j)
   {
      float yPosOfOriginOfGraph = (mTileHeight * j) + (mTileVerticalOffset / 2.0f);

      for (int i = 0; i < mNumTiles; ++i)
      {
         if (trackIndex >= mNumGraphs)
         {
            break;
         }

         float xPosOfOriginOfGraph = (mTileWidth * i) + (mTileHorizontalOffset / 2.0f);

         unsigned int numFrames = mTracks[trackIndex].GetNumberOfFrames();
         for (unsigned int frameIndex = 0; frameIndex < numFrames; ++frameIndex)
         {
            float currTime = mTracks[trackIndex].GetFrame(frameIndex).mTime;
            float currY = yPosOfOriginOfGraph + (mTracks[trackIndex].Sample(currTime, false).x * mGraphHeight); // TODO: Only using X right now
            float currX = xPosOfOriginOfGraph + (currTime * mGraphWidth);
            mKeyframePoints.push_back(glm::vec3(currX, currY, 0.9f));

            if (frameIndex > 0)
            {
               float prevY = yPosOfOriginOfGraph + (mTracks[trackIndex].Sample(currTime - 0.0005f, false).x * mGraphHeight); // TODO: Only using X right now
               float prevX = xPosOfOriginOfGraph + ((currTime - 0.0005f) * mGraphWidth);

               glm::vec3 thisVec = glm::vec3(currX, currY, 0.6f);
               glm::vec3 prevVec = glm::vec3(prevX, prevY, 0.6f);
               glm::vec3 handleVec = thisVec + normalizeWithZeroLengthCheck(prevVec - thisVec) * mSlopeLineScalingFactor;

               mSlopeLines.push_back(thisVec);
               mSlopeLines.push_back(handleVec);
            }

            if ((frameIndex < (numFrames - 1)) && (mTracks[trackIndex].GetInterpolation() != Interpolation::Constant))
            {
               float nextY = yPosOfOriginOfGraph + mTracks[trackIndex].Sample(currTime + 0.0005f, false).x * mGraphHeight; // TODO: Only using X right now
               float nextX = xPosOfOriginOfGraph + (currTime + 0.0005f) * mGraphWidth;

               glm::vec3 thisVec = glm::vec3(currX, currY, 0.6f);
               glm::vec3 nextVec = glm::vec3(nextX, nextY, 0.6f);
               glm::vec3 handleVec = thisVec + normalizeWithZeroLengthCheck(nextVec - thisVec) * mSlopeLineScalingFactor;

               mSlopeLines.push_back(thisVec);
               mSlopeLines.push_back(handleVec);
            }
         }

         ++trackIndex;
      }
   }
}
