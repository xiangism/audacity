/**********************************************************************

  Audacity: A Digital Audio Editor

  PrefsPanel.h

  Joshua Haberman

*******************************************************************//**

\class PrefsPanel
\brief Base class for a panel in the PrefsDialog.  Classes derived from 
this class include BatchPrefs, DirectoriesPrefs, GUIPrefs, KeyConfigPrefs, 
MousePrefs, QualityPrefs, SpectrumPrefs and ThemePrefs.

  The interface works like this: Each panel in the preferences dialog
  must derive from PrefsPanel. You must override Apply() with code
  to validate fields (returning false if any are bad), updating the
  global preferences object gPrefs, and instructing the applicable parts
  of the program to re-read the preference options.

  To actually add the new panel, edit the PrefsDialog constructor
  to append the panel to its list of panels.

*******************************************************************//**

\class PrefsPanelFactory
\brief Base class for factories such as GUIPrefsFactory that produce a
PrefsPanel.

*//*******************************************************************/

#ifndef __AUDACITY_PREFS_PANEL__
#define __AUDACITY_PREFS_PANEL__

#include "../widgets/wxPanelWrapper.h" // to inherit
#include "../include/audacity/ComponentInterface.h"

/* A few constants for an attempt at semi-uniformity */
#define PREFS_FONT_SIZE     8

#define BUILTIN_PREFS_PANEL_PREFIX wxT("Built-in PrefsPanel: ")

/* these are spacing guidelines: ie. radio buttons should have a 5 pixel
 * border on each side */
#define RADIO_BUTTON_BORDER    5
#define TOP_LEVEL_BORDER       5
#define GENERIC_CONTROL_BORDER 5

class ShuttleGui;

class PrefsPanel /* not final */ : public wxPanelWrapper, ComponentInterface
{
 public:
   PrefsPanel(wxWindow * parent, wxWindowID winid, const wxString &title)
   :  wxPanelWrapper(parent, winid)
   {
      SetLabel(title);     // Provide visual label
      SetName(title);      // Provide audible label
   }

   PrefsPanel(const PrefsPanel&) = delete;
   PrefsPanel& operator=(const PrefsPanel&) = delete;
   PrefsPanel(PrefsPanel&&) = delete;
   PrefsPanel& operator=(PrefsPanel&&) = delete;

   virtual ~PrefsPanel();

   // NEW virtuals
   virtual void Preview() {} // Make tentative changes
   virtual bool Commit() = 0; // used to be called "Apply"


   virtual PluginPath GetPath();
   virtual VendorSymbol GetVendor();
   virtual wxString GetVersion();

   //virtual ComponentInterfaceSymbol GetSymbol();
   //virtual wxString GetDescription();


   // If it returns True, the Preview button is added below the panel
   // Default returns false
   virtual bool ShowsPreviewButton();
   virtual void PopulateOrExchange( ShuttleGui & WXUNUSED(S) ){};

   // If not empty string, the Help button is added below the panel
   // Default returns empty string.
   virtual wxString HelpPageName();

   virtual void Cancel();
};

class PrefsPanelFactory /* not final */
{
public:
   PrefsPanelFactory() = default;
   PrefsPanelFactory(const PrefsPanelFactory&) = delete;
   PrefsPanelFactory& operator=(const PrefsPanelFactory&) = delete;
   PrefsPanelFactory(PrefsPanelFactory&&) = delete;
   PrefsPanelFactory& operator=(PrefsPanelFactory&&) = delete;

   virtual ~PrefsPanelFactory();

   // Precondition: parent != NULL
   virtual PrefsPanel *operator () (wxWindow *parent, wxWindowID winid) const = 0;
};

#endif
