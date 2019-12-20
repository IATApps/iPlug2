#include "IPlugMidiEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "IGraphicsFlexBox.h"

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
      IFlexBox f;
      f.Init(YGFlexDirectionRow, YGJustifyCenter, 20.f, 20.f);
      int a = f.AddChild(10, 30, YGAlignCenter, 0.f);
      int b = f.AddChild(25, 80, YGAlignCenter, 1.f);
      f.CalcLayout(parent);
      bgBounds = f.GetBounds(0);
      imageBounds = f.GetBounds(a);
      textBounds = f.GetBounds(b);
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
