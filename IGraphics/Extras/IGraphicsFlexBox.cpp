#include "IGraphicsFlexBox.h"

using namespace iplug;
using namespace igraphics;

IFlexBox::IFlexBox()
{
  mConfigRef = YGConfigNew();
  mRootNodeRef = YGNodeNewWithConfig(mConfigRef);
}

IFlexBox::~IFlexBox()
{
  YGNodeFreeRecursive(mRootNodeRef);
  YGConfigFree(mConfigRef);
}

void IFlexBox::Init(YGFlexDirection flexDirection, YGJustify justify, float padding, float margin)
{
  YGNodeStyleSetFlexDirection(mRootNodeRef, flexDirection);
  YGNodeStyleSetJustifyContent(mRootNodeRef, justify);
  YGNodeStyleSetPadding(mRootNodeRef, YGEdgeAll, padding);
  YGNodeStyleSetMargin(mRootNodeRef, YGEdgeAll, margin);
}

void IFlexBox::CalcLayout(const IRECT& r, YGDirection direction)
{
  YGNodeCalculateLayout(mRootNodeRef, r.W(), r.H(), direction);
}

int IFlexBox::AddChild(float width, float height, YGAlign alignSelf, float margin, float flexGrow, float flexShrink)
{
  int index = mNodeCounter;
  YGNodeRef child = YGNodeNew();
  
  if(width > 0)
    YGNodeStyleSetWidth(child, width);
  
  if(height > 0)
    YGNodeStyleSetHeight(child, height);
  
  YGNodeStyleSetAlignSelf(child, alignSelf);
  YGNodeStyleSetMargin(child, YGEdgeEnd, margin);
  YGNodeStyleSetFlexGrow(child, flexGrow);
  YGNodeStyleSetFlexShrink(child, flexShrink);
  YGNodeInsertChild(mRootNodeRef, child, index);
  
  mNodeCounter++;
  
  return index;
}

IRECT IFlexBox::GetBounds(int nodeIndex) const
{
  if(nodeIndex == 0)
    return IRECT(YGNodeLayoutGetLeft(mRootNodeRef),
                 YGNodeLayoutGetTop(mRootNodeRef),
                 YGNodeLayoutGetLeft(mRootNodeRef) + YGNodeLayoutGetWidth(mRootNodeRef),
                 YGNodeLayoutGetTop(mRootNodeRef)  + YGNodeLayoutGetHeight(mRootNodeRef));
  
  else
  {
    YGNodeRef child = YGNodeGetChild(mRootNodeRef, nodeIndex);
    return IRECT(YGNodeLayoutGetLeft(mRootNodeRef) + YGNodeLayoutGetLeft(child),
                 YGNodeLayoutGetTop(mRootNodeRef)  + YGNodeLayoutGetTop(child),
                 YGNodeLayoutGetLeft(mRootNodeRef) + YGNodeLayoutGetLeft(child) + YGNodeLayoutGetWidth(child),
                 YGNodeLayoutGetTop(mRootNodeRef)  + YGNodeLayoutGetTop(child)  + YGNodeLayoutGetHeight(child));
  }
};

// include Yoga src
#include "YGLayout.cpp"
#include "YGEnums.cpp"
#include "YGMarker.cpp"
#include "YGNodePrint.cpp"
#include "YGValue.cpp"
#include "YGConfig.cpp"
#include "YGNode.cpp"
#include "YGStyle.cpp"
#include "Yoga.cpp"
#include "Utils.cpp"
#include "log.cpp"
