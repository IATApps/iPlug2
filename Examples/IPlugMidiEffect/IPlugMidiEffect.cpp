#include "IPlugMidiEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "Yoga.h"

IPlugMidiEffect::IPlugMidiEffect(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPrograms))
{
  GetParam(kParamGain)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");
  
#if IPLUG_DSP
  SetTailSize(4410000);
#endif
  
#if IPLUG_EDITOR // All UI methods and member variables should be within an IPLUG_EDITOR guard, should you want distributed UI
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, 1.);
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    IRECT bounds = pGraphics->GetBounds();
    
    auto calculateFlexBox = [](const IRECT& parent, IRECT& bgBounds, IRECT& imageBounds, IRECT& textBounds) {
      YGConfigRef config = YGConfigNew();
      YGNodeRef root = YGNodeNewWithConfig(config);
      YGNodeStyleSetFlexDirection(root, YGFlexDirectionRow);
      YGNodeStyleSetPadding(root, YGEdgeAll, 20);
      YGNodeStyleSetMargin(root, YGEdgeAll, 20);
      
      YGNodeRef image = YGNodeNew();
      YGNodeStyleSetWidth(image, 80);
      YGNodeStyleSetHeight(image, 80);
      YGNodeStyleSetAlignSelf(image, YGAlignCenter);
      YGNodeStyleSetMargin(image, YGEdgeEnd, 20);
      
      YGNodeRef text = YGNodeNew();
      YGNodeStyleSetHeight(text, 25);
      YGNodeStyleSetAlignSelf(text, YGAlignCenter);
      YGNodeStyleSetFlexGrow(text, 1);
      
      YGNodeInsertChild(root, image, 0);
      YGNodeInsertChild(root, text, 1);
      
      YGNodeCalculateLayout(root, parent.W(), parent.H(), YGDirectionLTR);
      
      auto ToIRECT = [](YGNodeRef root, YGNodeRef input) {
        if(input == root) return IRECT(YGNodeLayoutGetLeft(root), YGNodeLayoutGetTop(root), YGNodeLayoutGetLeft(root) + YGNodeLayoutGetWidth(input), YGNodeLayoutGetTop(root) + YGNodeLayoutGetHeight(root));
        else return IRECT(YGNodeLayoutGetLeft(root) + YGNodeLayoutGetLeft(input), YGNodeLayoutGetTop(root) + YGNodeLayoutGetTop(input), YGNodeLayoutGetLeft(root) + YGNodeLayoutGetLeft(input) + YGNodeLayoutGetWidth(input), YGNodeLayoutGetTop(root) + YGNodeLayoutGetTop(input) + YGNodeLayoutGetHeight(input));
      };
      
      bgBounds = ToIRECT(root, root);
      imageBounds = ToIRECT(root, image);
      textBounds = ToIRECT(root, text);
      YGNodeFreeRecursive(root);
      YGConfigFree(config);
    };

    auto actionFunc = [&](IControl* pCaller) {
      static bool onoff = false;
      onoff = !onoff;
      IMidiMsg msg;
      constexpr int pitches[3] = {60, 65, 67};
      
      for (int i = 0; i<3; i++)
      {
        if(onoff)
          msg.MakeNoteOnMsg(pitches[i], 60, 0);
        else
          msg.MakeNoteOffMsg(pitches[i], 0);
      
        SendMidiMsgFromUI(msg);
      }
      
      SplashClickActionFunc(pCaller);
    };
    
    IRECT white, blue, red;
    calculateFlexBox(bounds, white, blue, red);
    
    if(pGraphics->NControls())
    {
      pGraphics->GetBackgroundControl()->SetTargetAndDrawRECTs(bounds);
      pGraphics->GetControlWithTag(0)->SetTargetAndDrawRECTs(white);
      pGraphics->GetControlWithTag(1)->SetTargetAndDrawRECTs(blue);
      pGraphics->GetControlWithTag(2)->SetTargetAndDrawRECTs(red);
      
      return;
    }
    else
    {
      pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
      pGraphics->AttachPanelBackground(COLOR_GRAY);
      pGraphics->AttachCornerResizer(EUIResizerMode::Size, true);
      
      pGraphics->AttachControl(new IPanelControl(white, COLOR_WHITE), 0);
      pGraphics->AttachControl(new IPanelControl(blue, COLOR_BLUE), 1);
      pGraphics->AttachControl(new IPanelControl(red, COLOR_RED), 2);


//      pGraphics->AttachControl(new IVButtonControl(pGraphics->GetBounds().GetPadded(-10), actionFunc, "Trigger Chord"));
    }
  };
#endif
}

#if IPLUG_DSP
void IPlugMidiEffect::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kParamGain)->Value() / 100.;
  const int nChans = NOutChansConnected();

  for (auto s = 0; s < nFrames; s++) {
    for (auto c = 0; c < nChans; c++) {
      outputs[c][s] = outputs[c][s] * gain;
    }
  }
}

void IPlugMidiEffect::ProcessMidiMsg(const IMidiMsg& msg)
{
  TRACE;
  
  int status = msg.StatusMsg();
  
  switch (status)
  {
    case IMidiMsg::kNoteOn:
    case IMidiMsg::kNoteOff:
    case IMidiMsg::kPolyAftertouch:
    case IMidiMsg::kControlChange:
    case IMidiMsg::kProgramChange:
    case IMidiMsg::kChannelAftertouch:
    case IMidiMsg::kPitchWheel:
    {
      goto handle;
    }
    default:
      return;
  }
  
handle:
  SendMidiMsg(msg);
}
#endif
