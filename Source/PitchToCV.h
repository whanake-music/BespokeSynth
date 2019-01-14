/*
  ==============================================================================

    PitchToCV.h
    Created: 28 Nov 2017 9:44:14pm
    Author:  Ryan Challinor

  ==============================================================================
*/

#pragma once
#include "IDrawableModule.h"
#include "INoteReceiver.h"
#include "IModulator.h"
#include "Slider.h"

class PatchCableSource;

class PitchToCV : public IDrawableModule, public INoteReceiver, public IModulator, public IFloatSliderListener
{
public:
   PitchToCV();
   virtual ~PitchToCV();
   static IDrawableModule* Create() { return new PitchToCV(); }
   
   string GetTitleLabel() override { return "pitch to cv"; }
   void CreateUIControls() override;
   
   void SetEnabled(bool enabled) override { mEnabled = enabled; }
   
   //INoteReceiver
   void PlayNote(double time, int pitch, int velocity, int voiceIdx = -1, ModulationParameters modulation = ModulationParameters()) override;
   void SendCC(int control, int value, int voiceIdx = -1) override {}
   
   //IModulator
   virtual float Value(int samplesIn = 0) override;
   virtual bool Active() const override { return mEnabled; }
   
   //IPatchable
   void PostRepatch(PatchCableSource* cableSource, bool fromUserClick) override;
   
   void FloatSliderUpdated(FloatSlider* slider, float oldVal) override {}
   
   void SaveLayout(ofxJSONElement& moduleInfo) override;
   void LoadLayout(const ofxJSONElement& moduleInfo) override;
   void SetUpFromSaveData() override;
private:
   //IDrawableModule
   void DrawModule() override;
   void GetModuleDimensions(int& width, int& height) override { width = 106; height=17*2+2; }
   bool Enabled() const override { return mEnabled; }
   
   float mPitch;
   ModulationChain* mPitchBend;
};
