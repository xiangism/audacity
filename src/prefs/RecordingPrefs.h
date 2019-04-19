/**********************************************************************

  Audacity: A Digital Audio Editor

  RecordingPrefs.h

  Joshua Haberman
  James Crook

**********************************************************************/

#ifndef __AUDACITY_RECORDING_PREFS__
#define __AUDACITY_RECORDING_PREFS__

#include <wx/defs.h>

#include "PrefsPanel.h"

class wxTextCtrl;
class ShuttleGui;

#define RECORDING_PREFS_PLUGIN_SYMBOL ComponentInterfaceSymbol{ XO("Recording") }

class RecordingPrefs final : public PrefsPanel
{
 public:
   RecordingPrefs(wxWindow * parent, wxWindowID winid);
   virtual ~RecordingPrefs();
   virtual ComponentInterfaceSymbol GetSymbol();
   virtual wxString GetDescription();

   bool Commit() override;
   wxString HelpPageName() override;
   void PopulateOrExchange(ShuttleGui & S) override;

 private:
   void Populate();
   void OnToggleCustomName(wxCommandEvent & /* Evt */);

   wxTextCtrl *mToggleCustomName;
   bool mUseCustomTrackName;
   bool mOldNameChoice;

   DECLARE_EVENT_TABLE()
};

/// A PrefsPanelFactory that creates one RecordingPrefs panel.
class RecordingPrefsFactory final : public PrefsPanelFactory
{
public:
   PrefsPanel *operator () (wxWindow *parent, wxWindowID winid) const override;
};
#endif
