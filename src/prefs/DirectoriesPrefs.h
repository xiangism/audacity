/**********************************************************************

  Audacity: A Digital Audio Editor

  DirectoriesPrefs.h

  Joshua Haberman

**********************************************************************/

#ifndef __AUDACITY_DIRECTORIES_PREFS__
#define __AUDACITY_DIRECTORIES_PREFS__

#include "PrefsPanel.h"

class ShuttleGui;

class wxStaticText;
class wxTextCtrl;

#define DIRECTORIES_PREFS_PLUGIN_SYMBOL ComponentInterfaceSymbol{ XO("Directories") }

class DirectoriesPrefs final : public PrefsPanel
{
 public:
   DirectoriesPrefs(wxWindow * parent, wxWindowID winid);
   ~DirectoriesPrefs();
   virtual ComponentInterfaceSymbol GetSymbol();
   virtual wxString GetDescription();

   bool Commit() override;
   bool Validate() override;
   wxString HelpPageName() override;
   void PopulateOrExchange(ShuttleGui & S) override;

 private:
   void Populate();
   void UpdateFreeSpace(wxCommandEvent & e);
   void OnChooseTempDir(wxCommandEvent & e);

   wxStaticText *mFreeSpace;
   wxTextCtrl *mTempDir;

   DECLARE_EVENT_TABLE()
};

/// A PrefsPanelFactory that creates one DirectoriesPrefs panel.
class DirectoriesPrefsFactory final : public PrefsPanelFactory
{
public:
   PrefsPanel *operator () (wxWindow *parent, wxWindowID winid) const override;
};
#endif
