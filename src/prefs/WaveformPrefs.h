/**********************************************************************

Audacity: A Digital Audio Editor

WaveformPrefs.h

Paul Licameli

**********************************************************************/


#ifndef __AUDACITY_WAVEFORM_PREFS__
#define __AUDACITY_WAVEFORM_PREFS__

#include "PrefsPanel.h"
#include "WaveformSettings.h"

class ShuttleGui;
class WaveTrack;
class wxCheckBox;
class wxChoice;

class wxArrayStringEx;

#define WAVEFORM_PREFS_PLUGIN_SYMBOL ComponentInterfaceSymbol{ XO("Waveform") }

class WaveformPrefs final : public PrefsPanel
{
public:
   WaveformPrefs(wxWindow * parent, wxWindowID winid, WaveTrack *wt);
   virtual ~WaveformPrefs();
   ComponentInterfaceSymbol GetSymbol() override;
   wxString GetDescription() override;
   wxString HelpPageName() override;

   bool Commit() override;
   bool ShowsPreviewButton() override;
   bool Validate() override;
   void PopulateOrExchange(ShuttleGui & S) override;

private:
   void Populate();

   void OnControl(wxCommandEvent&);
   void OnScale(wxCommandEvent&);
   void OnDefaults(wxCommandEvent&);
   DECLARE_EVENT_TABLE()

   void EnableDisableRange();

   WaveTrack *const mWt;
   bool mDefaulted;

   wxCheckBox *mDefaultsCheckbox;
   wxChoice *mScaleChoice;
   wxChoice *mRangeChoice;

   wxArrayStringEx mRangeCodes;
   wxArrayStringEx mRangeChoices;

   WaveformSettings mTempSettings;

   bool mPopulating;
};

/// A PrefsPanelFactory that creates one WaveformPrefs panel.
class WaveformPrefsFactory final : public PrefsPanelFactory
{
public:
   explicit WaveformPrefsFactory(WaveTrack *wt = 0);
   PrefsPanel *operator () (wxWindow *parent, wxWindowID winid) const override;

private:
   WaveTrack *const mWt;
};
#endif
