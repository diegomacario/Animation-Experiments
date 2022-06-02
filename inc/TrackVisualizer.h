#ifndef TRACK_VISUALIZER_H
#define TRACK_VISUALIZER_H

#include <memory>

#include "Track.h"
#include "shader.h"

class TrackVisualizer
{
public:

   TrackVisualizer();
   ~TrackVisualizer();

   void setTrack(const ScalarTrack& track);

   void render();

private:

   void initializeReferenceLines();
   void initializeTrackLines();
   void initializeKeyframePointsAndSlopeLines();

   float                    mWidthOfGraphSpace;
   float                    mHeightOfGraphSpace;
   unsigned int             mNumGraphs;
   unsigned int             mNumTiles;
   float                    mTileWidth;
   float                    mTileHeight;
   float                    mTileHorizontalOffset;
   float                    mTileVerticalOffset;
   float                    mSlopeLineScalingFactor;

   unsigned int             mReferenceLinesVAO;
   unsigned int             mReferenceLinesVBO;

   unsigned int             mTrackLinesVAO;
   unsigned int             mTrackLinesVBO;

   unsigned int             mKeyframePointsVAO;
   unsigned int             mKeyframePointsVBO;

   unsigned int             mSlopeLinesVAO;
   unsigned int             mSlopeLinesVBO;

   std::shared_ptr<Shader>  mTrackShader;

   std::vector<ScalarTrack> mTracks;
   std::vector<glm::vec3>   mReferenceLines;
   std::vector<glm::vec3>   mTrackLines;
   std::vector<glm::vec3>   mKeyframePoints;
   std::vector<glm::vec3>   mSlopeLines;
};

#endif
