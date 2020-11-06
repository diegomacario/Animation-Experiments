#ifndef REARRANGE_BONES_H
#define REARRANGE_BONES_H

#include <map>
#include "Skeleton.h"
#include "Mesh.h"
#include "Clip.h"

typedef std::map<int, int> JointMap;

JointMap RearrangeSkeleton(Skeleton& skeleton);
void     RearrangeClip(Clip& clip, JointMap& jointMap);
void     RearrangeFastClip(FastClip& fastClip, JointMap& jointMap);
//void     RearrangeMesh(Mesh& mesh, JointMap& jointMap);

#endif
