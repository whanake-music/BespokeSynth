//
//  TitleBar.cpp
//  Bespoke
//
//  Created by Ryan Challinor on 11/30/14.
//
//

#include "TitleBar.h"
#include "ModularSynth.h"
#include "SynthGlobals.h"
#include "Profiler.h"
#include "ModuleFactory.h"
#include "ModuleSaveDataPanel.h"
#include "HelpDisplay.h"
#include "VSTPlugin.h"
#include "Prefab.h"

TitleBar* TheTitleBar = nullptr;

SpawnList::SpawnList(TitleBar* owner, int x, int y, string label)
: mLabel(label)
, mSpawnIndex(-1)
, mSpawnList(nullptr)
, mOwner(owner)
, mPos(x,y)
{
}

void SpawnList::SetList(vector<string> spawnables, string overrideModuleType)
{
   mOverrideModuleType = overrideModuleType;
   mSpawnList = new DropdownList(mOwner,mLabel.c_str(),mPos.x,mPos.y,&mSpawnIndex);
   mSpawnList->SetNoHover(true);
   
   mSpawnList->Clear();
   mSpawnList->SetUnknownItemString(mLabel);
   mSpawnables = spawnables;
   for (int i=0; i<mSpawnables.size(); ++i)
   {
      string name = mSpawnables[i].c_str();
      if (mOverrideModuleType == "" && TheSynth->GetModuleFactory()->IsExperimental(name))
         name += " (exp.)";
      mSpawnList->AddLabel(name,i);
   }
}

namespace
{
   ofVec2f moduleGrabOffset(-40,10);
}

void SpawnList::OnSelection(DropdownList* list)
{
   if (list == mSpawnList)
   {
      IDrawableModule* module = Spawn();
      TheSynth->SetMoveModule(module, moduleGrabOffset.x, moduleGrabOffset.y);
      mSpawnIndex = -1;
   }
}

IDrawableModule* SpawnList::Spawn()
{
   string moduleType = mSpawnables[mSpawnIndex];
   if (mOverrideModuleType != "")
      moduleType = mOverrideModuleType;
   
   IDrawableModule* module = TheSynth->SpawnModuleOnTheFly(moduleType, TheSynth->GetMouseX() + moduleGrabOffset.x, TheSynth->GetMouseY() + moduleGrabOffset.y);
   
   if (mOverrideModuleType == "vstplugin")
   {
      VSTPlugin* plugin = dynamic_cast<VSTPlugin*>(module);
      plugin->SetVST(mSpawnables[mSpawnIndex]);
   }
   
   if (mOverrideModuleType == "prefab")
   {
      Prefab* prefab = dynamic_cast<Prefab*>(module);
      prefab->LoadPrefab("prefabs/"+mSpawnables[mSpawnIndex]);
   }
   
   return module;
}

void SpawnList::Draw()
{
   int x,y;
   mSpawnList->GetPosition(x, y, true);
   //DrawText(mLabel,x,y-2);
   mSpawnList->Draw();
}

void SpawnList::SetPosition(int x, int y)
{
   mSpawnList->SetPosition(x, y);
}

void SpawnList::SetPositionRelativeTo(SpawnList* list)
{
   int x,y,w,h;
   list->mSpawnList->GetPosition(x, y, true);
   list->mSpawnList->GetDimensions(w, h);
   mSpawnList->SetPosition(x + w + 5, y);
}

TitleBar::TitleBar()
: mInstrumentModules(this,500,16,"instruments:")
, mNoteModules(this,0,0,"note effects:")
, mSynthModules(this,0,0,"synths:")
, mAudioModules(this,0,0,"audio effects:")
, mModulatorModules(this,0,0,"modulators:")
, mOtherModules(this,0,0,"other:")
, mVstPlugins(this,0,0,"vst plugins:")
, mPrefabs(this,0,0,"prefabs:")
, mSaveLayoutButton(nullptr)
, mResetLayoutButton(nullptr)
, mSaveStateButton(nullptr)
, mLoadStateButton(nullptr)
, mWriteAudioButton(nullptr)
, mQuitButton(nullptr)
, mLoadLayoutDropdown(nullptr)
, mLoadLayoutIndex(-1)
{
   assert(TheTitleBar == nullptr);
   TheTitleBar = this;
   
   mHelpDisplay = dynamic_cast<HelpDisplay*>(HelpDisplay::Create());
}

void TitleBar::CreateUIControls()
{
   IDrawableModule::CreateUIControls();
   mSaveLayoutButton = new ClickButton(this,"save layout",280,19);
   mLoadStateButton = new ClickButton(this,"load state",140,1);
   mSaveStateButton = new ClickButton(this,"save state",205,1);
   mResetLayoutButton = new ClickButton(this,"reset layout",140,19);
   mWriteAudioButton = new ClickButton(this,"write audio",280,1);
   mQuitButton = new ClickButton(this,"quit",400,1);
   mDisplayHelpButton = new ClickButton(this," ? ",380,1);
   mLoadLayoutDropdown = new DropdownList(this, "load layout", 140, 20, &mLoadLayoutIndex);
   
   mLoadLayoutDropdown->SetShowing(false);
   mSaveLayoutButton->SetShowing(false);
   
   mHelpDisplay->CreateUIControls();
   
   ListLayouts();
}

TitleBar::~TitleBar()
{
   assert(TheTitleBar == this);
   TheTitleBar = nullptr;
}

void TitleBar::SetModuleFactory(ModuleFactory* factory)
{
   mInstrumentModules.SetList(factory->GetSpawnableModules(kModuleType_Instrument), "");
   mNoteModules.SetList(factory->GetSpawnableModules(kModuleType_Note), "");
   mSynthModules.SetList(factory->GetSpawnableModules(kModuleType_Synth), "");
   mAudioModules.SetList(factory->GetSpawnableModules(kModuleType_Audio), "");
   mModulatorModules.SetList(factory->GetSpawnableModules(kModuleType_Modulator), "");
   mOtherModules.SetList(factory->GetSpawnableModules(kModuleType_Other), "");
   
   vector<string> vsts;
   VSTLookup::GetAvailableVSTs(vsts);
   mVstPlugins.SetList(vsts, "vstplugin");
   
   File dir(ofToDataPath("prefabs"));
   Array<File> files;
   dir.findChildFiles(files, File::findFiles, false);
   vector<string> prefabs;
   for (auto file : files)
   {
      if (file.getFileExtension() == ".pfb")
         prefabs.push_back(file.getFileName().toStdString());
   }
   mPrefabs.SetList(prefabs, "prefab");
}

void TitleBar::ListLayouts()
{
   mLoadLayoutDropdown->Clear();
   
   DirectoryIterator dir(File(ofToDataPath("layouts")), false);
   int layoutIdx = 0;
   while (dir.next())
   {
      File file = dir.getFile();
      if (file.getFileExtension() == ".json")
      {
         mLoadLayoutDropdown->AddLabel(file.getFileNameWithoutExtension().toRawUTF8(), layoutIdx);
         
         if (file.getRelativePathFrom(File(ofToDataPath(""))).toStdString() == TheSynth->GetLoadedLayout())
            mLoadLayoutIndex = layoutIdx;
         
         ++layoutIdx;
      }
   }
   
   mSaveLayoutButton->PositionTo(mLoadLayoutDropdown, kAnchor_Right);
}

void TitleBar::DrawModule()
{
   if (HiddenByZoom())
      return;
   
   ofSetColor(255,255,255);
   DrawTextBold("bespoke", 2, 28, 36);
   
   string info;
   if (TheSynth->GetMoveModule())
      info += " (moving module \"" + string(TheSynth->GetMoveModule()->Name()) + "\")";
   if (TextEntry::GetActiveTextEntry())
      info += " (entering text)";
   
   DrawTextLeftJustify(info, ofGetWidth()/gDrawScale - 60, 16);
   
   if (ofGetWidth() / gDrawScale >= 620)
   {
      mSaveLayoutButton->Draw();
      mSaveStateButton->Draw();
      mLoadStateButton->Draw();
      mWriteAudioButton->Draw();
      mLoadLayoutDropdown->Draw();
      mResetLayoutButton->Draw();
   }
   
   if (ofGetWidth() / gDrawScale >= 920)
   {
      if (ofGetWidth() / gDrawScale >= 1280)
      {
         mInstrumentModules.SetPosition(400,2);
         mNoteModules.SetPositionRelativeTo(&mInstrumentModules);
         mSynthModules.SetPositionRelativeTo(&mNoteModules);
         mAudioModules.SetPositionRelativeTo(&mSynthModules);
         mModulatorModules.SetPositionRelativeTo(&mAudioModules);
         mOtherModules.SetPositionRelativeTo(&mModulatorModules);
         if (ofGetWidth() / gDrawScale >= 1400)
            mVstPlugins.SetPositionRelativeTo(&mOtherModules);
         else
            mVstPlugins.SetPosition(400,18);
         mPrefabs.SetPositionRelativeTo(&mVstPlugins);
      }
      else
      {
         mInstrumentModules.SetPosition(400,2);
         mNoteModules.SetPositionRelativeTo(&mInstrumentModules);
         mSynthModules.SetPositionRelativeTo(&mNoteModules);
         mAudioModules.SetPosition(400, 18);
         mModulatorModules.SetPositionRelativeTo(&mAudioModules);
         mOtherModules.SetPositionRelativeTo(&mModulatorModules);
         mVstPlugins.SetPositionRelativeTo(&mOtherModules);
         mPrefabs.SetPositionRelativeTo(&mVstPlugins);
      }
      mInstrumentModules.Draw();
      mNoteModules.Draw();
      mSynthModules.Draw();
      mAudioModules.Draw();
      mModulatorModules.Draw();
      mOtherModules.Draw();
      mVstPlugins.Draw();
      mPrefabs.Draw();
   }
   
   float usage = TheSynth->GetGlobalManagers()->mDeviceManager.getCpuUsage();
   string stats;
   stats += "fps:" + ofToString(ofGetFrameRate(),0);
   stats += "  audio cpu:" + ofToString(usage * 100,1);
   if (usage > 1)
      ofSetColor(255,150,150);
   else
      ofSetColor(255,255,255);
   DrawTextLeftJustify(stats, ofGetWidth()/gDrawScale - 5, 33);
   mQuitButton->SetPosition(ofGetWidth()/gDrawScale - 31, 4);
   mQuitButton->Draw();
   mDisplayHelpButton->SetPosition(ofGetWidth()/gDrawScale - 51, 4);
   mDisplayHelpButton->Draw();
}

bool TitleBar::HiddenByZoom() const
{
   return ofGetWidth() / gDrawScale < 620;
}

void TitleBar::GetModuleDimensions(int& x, int& y)
{
   if (HiddenByZoom())
   {
      x = 0;
      y = 0;
      return;
   }
   
   x = ofGetWidth() / gDrawScale + 5;
   y = 36;
}

void TitleBar::CheckboxUpdated(Checkbox* checkbox)
{
}

void TitleBar::DropdownUpdated(DropdownList* list, int oldVal)
{
   if (list == mLoadLayoutDropdown)
   {
      string layout = mLoadLayoutDropdown->GetLabel(mLoadLayoutIndex);
      TheSynth->LoadLayoutFromFile(ofToDataPath("layouts/"+layout+".json"));
      return;
   }
   
   mInstrumentModules.OnSelection(list);
   mNoteModules.OnSelection(list);
   mSynthModules.OnSelection(list);
   mAudioModules.OnSelection(list);
   mModulatorModules.OnSelection(list);
   mOtherModules.OnSelection(list);
   mVstPlugins.OnSelection(list);
   mPrefabs.OnSelection(list);
}

void TitleBar::ButtonClicked(ClickButton* button)
{
   if (button == mSaveLayoutButton)
   {
      if (GetKeyModifiers() == kModifier_Shift)
         TheSynth->SaveLayoutAsPopup();
      else
         TheSynth->SaveLayout();
   }
   if (button == mSaveStateButton)
      TheSynth->SaveStatePopup();
   if (button == mLoadStateButton)
      TheSynth->LoadStatePopup();
   if (button == mWriteAudioButton)
      TheSynth->SaveOutput();
   if (button == mQuitButton)
      TheSynth->Exit();
   if (button == mDisplayHelpButton)
   {
      int x,y,w,h,butW,butH;
      mDisplayHelpButton->GetPosition(x, y);
      mDisplayHelpButton->GetDimensions(butW, butH);
      mHelpDisplay->GetDimensions(w, h);
      mHelpDisplay->SetPosition(x-w+butW,y+butH);
      TheSynth->PushModalFocusItem(mHelpDisplay);
   }
   if (button == mResetLayoutButton)
      TheSynth->LoadLayoutFromFile(ofToDataPath("layouts/blank.json"));
}


