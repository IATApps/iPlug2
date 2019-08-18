/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc IPlugFaust
 */

#include <memory>

#include "faust/gui/UI.h"
#include "faust/gui/MidiUI.h"
#include "assocarray.h"

#include "IPlugAPIBase.h"

#include "Oversampler.h"

#ifndef DEFAULT_FAUST_LIBRARY_PATH
  #if defined OS_MAC || defined OS_LINUX
    #define DEFAULT_FAUST_LIBRARY_PATH "/usr/local/share/faust/"
  #else
   #define DEFAULT_FAUST_LIBRARY_PATH "C:\\Program Files\\Faust\\share\\faust"
  #endif
#endif

BEGIN_IPLUG_NAMESPACE

/** This abstract interface is used by the IPlug FAUST architecture file and the IPlug libfaust JIT compiling class FaustGen
 * In order to provide a consistent interface to FAUST DSP whether using the JIT compiler or a compiled C++ class */
class IPlugFaust : public UI, public Meta
{
public:

  IPlugFaust(const char* name, int nVoices = 1, int rate = 1)
  : mNVoices(nVoices)
  {
    if(rate > 1)
      mOverSampler = std::make_unique<OverSampler<sample>>(OverSampler<sample>::RateToFactor(rate), true, 2 /* TODO: flexible channel count */);
    
    mName.Set(name);
  }

  virtual ~IPlugFaust()
  {
    mParams.Empty(true);
  }

  IPlugFaust(const IPlugFaust&) = delete;
  IPlugFaust& operator=(const IPlugFaust&) = delete;
    
  virtual void Init() = 0;

  // NO-OP in the base class
  virtual void SetMaxChannelCount(int maxNInputs, int maxNOutputs) {}
  
  /** In FaustGen this is implemented, so that the SVG files generated by a specific instance can be located. The path to the SVG file for process.svg will be returned.
   * There is a NO-OP implementation here so that when not using the JIT compiler, the same class can be used interchangeably
   * @param path The absolute path to process.svg for this instance. */
  virtual void GetDrawPath(WDL_String& path) {}

  /** Call this method from FaustGen in order to execute a shell command and compile the C++ code against the IPlugFaust_arch architecture file
   * There is a NO-OP implementation here so that when not using the JIT compiler, the same class can be used interchangeably
   * @return \c true on success */
  static bool CompileCPP() { return true; }

  static void SetAutoRecompile(bool enable) {}
  
  void FreeDSP()
  {
    mDSP = nullptr;
  }
  
  void SetOverSamplingRate(int rate)
  {
    if(mOverSampler)
      mOverSampler->SetOverSampling(OverSampler<sample>::RateToFactor(rate));
  }

  // Unique methods
  void SetSampleRate(double sampleRate)
  {
    int multiplier = 1;
    
    if(mOverSampler)
      multiplier = mOverSampler->GetRate();
    
    if (mDSP)
      mDSP->init(((int) sampleRate) * multiplier);
  }

  void ProcessMidiMsg(const IMidiMsg& msg)
  {
//    TODO:
  }

  virtual void ProcessBlock(sample** inputs, sample** outputs, int nFrames)
  {
    if (mDSP)
    {
      assert(mDSP->getSampleRate() != 0); // did you forget to call SetSampleRate?
      
      if(mOverSampler)
        mOverSampler->ProcessBlock(inputs, outputs, nFrames, 2 /* TODO: flexible channel count */,
                                   [&](sample** inputs, sample** outputs, int nFrames) //TODO:: badness capture = allocated
                                   {
                                     mDSP->compute(nFrames, inputs, outputs);
                                   });
      else
        mDSP->compute(nFrames, inputs, outputs);
    }
//    else silence?
  }

  void SetParameterValueNormalised(int paramIdx, double normalizedValue)
  {
    if(paramIdx > kNoParameter && paramIdx >= NParams())
    {
      DBGMSG("IPlugFaust-%s:: No parameter %i\n", mName.Get(), paramIdx);
    }
    else
    {
      mParams.Get(paramIdx)->SetNormalized(normalizedValue);
    
      if(mZones.GetSize() == NParams())
        *(mZones.Get(paramIdx)) = mParams.Get(paramIdx)->Value();
      else
        DBGMSG("IPlugFaust-%s:: Missing zone for parameter %s\n", mName.Get(), mParams.Get(paramIdx)->GetNameForHost());
    }
  }
  
  void SetParameterValue(int paramIdx, double nonNormalizedValue)
  {
    if(NParams()) {
      
    assert(paramIdx < NParams()); // Seems like we don't have enough parameters!
    
    mParams.Get(paramIdx)->Set(nonNormalizedValue);

    if(mZones.GetSize() == NParams())
      *(mZones.Get(paramIdx)) = nonNormalizedValue;
    else
      DBGMSG("IPlugFaust-%s:: Missing zone for parameter %s\n", mName.Get(), mParams.Get(paramIdx)->GetNameForHost());
    }
    else
      DBGMSG("SetParameterValue called with no FAUST params\n");
  }

  void SetParameterValue(const char* labelToLookup, double nonNormalizedValue)
  {
    FAUSTFLOAT* dest = nullptr;
    dest = mMap.Get(labelToLookup, nullptr);
//    mParams.Get(paramIdx)->Set(nonNormalizedValue); // TODO: we are not updating the IPlug parameter

    if(dest)
      *dest = nonNormalizedValue;
    else
      DBGMSG("IPlugFaust-%s:: No parameter named %s\n", mName.Get(), labelToLookup);
  }

  int CreateIPlugParameters(IPlugAPIBase* pPlug, int startIdx = 0, int endIdx = -1, bool setToDefault = true)
  {
    assert(pPlug != nullptr);
   
    if(NParams() == 0)
      return -1;
    
    mPlug = pPlug;
    
    int plugParamIdx = mIPlugParamStartIdx = startIdx;
    
    if(endIdx == -1)
      endIdx = pPlug->NParams();
    
    for (auto p = 0; p < endIdx; p++)
    {
      assert(plugParamIdx + p < pPlug->NParams()); // plugin needs to have enough params!

      IParam* pParam = pPlug->GetParam(plugParamIdx + p);
      const double currentValueNormalised = pParam->GetNormalized();
      pParam->Init(*mParams.Get(p));
      if(setToDefault)
        pParam->SetToDefault();
      else
        pParam->SetNormalized(currentValueNormalised);
    }

    return plugParamIdx;
  }

  int NParams()
  {
    return mParams.GetSize();
  }

  // Meta
  void declare(const char *key, const char *value) override
  {
    // TODO:
  }

  // UI

  // TODO:
  void openTabBox(const char *label) override {}
  void openHorizontalBox(const char *label) override {}
  void openVerticalBox(const char *label) override {}
  void closeBox() override {}
  
  void addButton(const char *label, FAUSTFLOAT *zone) override
  {
    AddOrUpdateParam(IParam::kTypeBool, label, zone);
  }

  void addCheckButton(const char *label, FAUSTFLOAT *zone) override
  {
    AddOrUpdateParam(IParam::kTypeBool, label, zone);
  }

  void addVerticalSlider(const char *label, FAUSTFLOAT *zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) override
  {
    AddOrUpdateParam(IParam::kTypeDouble, label, zone, init, min, max, step);
  }

  void addHorizontalSlider(const char *label, FAUSTFLOAT *zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) override
  {
    AddOrUpdateParam(IParam::kTypeDouble, label, zone, init, min, max, step);
  }

  void addNumEntry(const char *label, FAUSTFLOAT *zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) override
  {
    AddOrUpdateParam(IParam::kTypeEnum, label, zone, init, min, max, step);
  }

  // TODO:
  void addHorizontalBargraph(const char *label, FAUSTFLOAT *zone, FAUSTFLOAT min, FAUSTFLOAT max) override {}
  void addVerticalBargraph(const char *label, FAUSTFLOAT *zone, FAUSTFLOAT min, FAUSTFLOAT max) override {}
  void addSoundfile(const char *label, const char *filename, Soundfile **sf_zone) override {}

protected:
  void AddOrUpdateParam(IParam::EParamType type, const char *label, FAUSTFLOAT *zone, FAUSTFLOAT init = 0., FAUSTFLOAT min = 0., FAUSTFLOAT max = 0., FAUSTFLOAT step = 1.)
  {
    IParam* pParam = nullptr;
    
    const int idx = FindExistingParameterWithName(label);
    
    if(idx > -1)
      pParam = mParams.Get(idx);
    else
      pParam = new IParam();
    
    switch (type)
    {
      case IParam::EParamType::kTypeBool:
        pParam->InitBool(label, 0);
        break;
      case IParam::EParamType::kTypeInt:
        pParam->InitInt(label, static_cast<int>(init), static_cast<int>(min), static_cast<int>(max));
        break;
      case IParam::EParamType::kTypeEnum:
        pParam->InitEnum(label, static_cast<int>(init), static_cast<int>(max - min));
        //TODO: metadata
        break;
      case IParam::EParamType::kTypeDouble:
        pParam->InitDouble(label, init, min, max, step);
        break;
      default:
        break;
    }
    
    if(idx == -1)
      mParams.Add(pParam);
    
    mZones.Add(zone);
  }
  
  void BuildParameterMap()
  {
    for(auto p = 0; p < NParams(); p++)
    {
      mMap.Insert(mParams.Get(p)->GetNameForHost(), mZones.Get(p)); // insert will overwrite keys with the same name
    }
    
    if(mIPlugParamStartIdx > -1 && mPlug != nullptr) // if we've allready linked parameters
    {
      CreateIPlugParameters(mPlug, mIPlugParamStartIdx);
    }
    
    for(auto p = 0; p < NParams(); p++)
    {
      DBGMSG("%i %s\n", p, mParams.Get(p)->GetNameForHost());
    }
  }

  int FindExistingParameterWithName(const char* name) // TODO: this needs to check meta data too - incase of grouping
  {
    for(auto p = 0; p < NParams(); p++)
    {
      if(strcmp(name, mParams.Get(p)->GetNameForHost()) == 0)
      {
        return p;
      }
    }
    
    return -1;
  }
  
  std::unique_ptr<OverSampler<sample>> mOverSampler;
  WDL_String mName;
  int mNVoices;
  std::unique_ptr<::dsp> mDSP;
  std::unique_ptr<MidiUI> mMidiUI;
  WDL_PtrList<IParam> mParams;
  WDL_PtrList<FAUSTFLOAT> mZones;
  WDL_StringKeyedArray<FAUSTFLOAT*> mMap; // map is used for setting FAUST parameters by name, also used to reconnect existing parameters
  int mIPlugParamStartIdx = -1; // if this is negative, it means there is no linking
  IPlugAPIBase* mPlug = nullptr;
  bool mInitialized = false;
};

END_IPLUG_NAMESPACE
