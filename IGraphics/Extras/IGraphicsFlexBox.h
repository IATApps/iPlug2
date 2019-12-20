#pragma once

#include "Yoga.h"
#include "IGraphicsStructs.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

class IFlexBox
{
public:
  IFlexBox();
  
  ~IFlexBox();
  
  /** /todo */
  void Init(YGFlexDirection flexDirection = YGFlexDirectionRow, YGJustify justify = YGJustifySpaceBetween, float padding = 20.f, float margin = 20.f);
  
  /** /todo */
  void CalcLayout(const IRECT& r, YGDirection direction = YGDirectionLTR);
  
  /** /todo */
  int AddChild(float width, float height, YGAlign alignSelf = YGAlignCenter, float flexGrow = 0.f, float flexShrink = 0.f, float margin = 20.f);
  
  /** /todo */
  IRECT GetBounds(int nodeIndex) const;
  
private:
  int mNodeCounter = 0;
  YGConfigRef mConfigRef;
  YGNodeRef mRootNodeRef;
};

END_IPLUG_NAMESPACE
END_IGRAPHICS_NAMESPACE
