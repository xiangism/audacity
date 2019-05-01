/**********************************************************************

  Audacity: A Digital Audio Editor

  MousePrefs.cpp

  James Crook

********************************************************************//*!

\class MousePrefs
\brief A PrefsPanel that presents an interface for user to view the
   default bindings of mouse buttons to commands.

  April/2003: These are default bindings and are not yet configurable.
  They are provided to give information about what the bindings are.

  Configuration when available will be mostly used by power users
  who are unlikely to change the default bindings, but will add
  bindings (e.g. for cut, play, and their own nyquist filters)
  using currently unused combinations.

  Unlike key-bindings which are parameterless, mouse bindings
  provide parameters:

    - a single point for a click, and
    - a stream of points or a start and end point for a drag.

  If we allow a nyquist filter to be bound to the mouse, instead of
  being applied to the current selection it would be applied to the
  start and end points of the drag.

*//********************************************************************/

#include "../Audacity.h"
#include "MousePrefs.h"

#include "../Experimental.h"

#include <wx/defs.h>
#include <wx/intl.h>
#include <wx/listctrl.h>

#include "../Prefs.h"
#include "../ShuttleGui.h"
#include "../Internat.h"

// The numbers of the columns of the mList.
enum
{
   ToolColumn,
   ActionColumn,
   ButtonsColumn,
   CommentColumn
};

#if defined(__WXMAC__)
#define CTRL _("Command")
#else
#define CTRL _("Ctrl")
#endif

/// Constructor
MousePrefs::MousePrefs(wxWindow * parent, wxWindowID winid)
:  PrefsPanel(parent, winid, _("Mouse"))
{
   Populate();
}

MousePrefs::~MousePrefs() = default;

ComponentInterfaceSymbol MousePrefs::GetSymbol()
{
   return MOUSE_PREFS_PLUGIN_SYMBOL;
}

wxString MousePrefs::GetDescription()
{
   return _("Preferences for Mouse");
}

wxString MousePrefs::HelpPageName()
{
   return "Mouse_Preferences";
}

/// Creates the dialog and its contents.
void MousePrefs::Populate()
{
   //------------------------- Main section --------------------
   // Now construct the GUI itself.
   // Use 'eIsCreatingFromPrefs' so that the GUI is
   // initialised with values from gPrefs.
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);
   // ----------------------- End of main section --------------
   CreateList();
   if (mList->GetItemCount() > 0) {
      // set first item to be selected (and the focus when the
      // list first becomes the focus)
      mList->SetItemState(0, wxLIST_STATE_FOCUSED | wxLIST_STATE_SELECTED,
         wxLIST_STATE_FOCUSED | wxLIST_STATE_SELECTED);
   }
}

/// Places controls on the panel and also exchanges data with them.
void MousePrefs::PopulateOrExchange(ShuttleGui & S)
{
   S.SetBorder(2);

   S.StartStatic(_("Mouse Bindings (default values, not configurable)"), 1);
   {
      mList = S.AddListControlReportMode();
   }
   S.EndStatic();
}

/// Creates the contents of mList
void MousePrefs::CreateList()
{
   //A dummy first column, which is then deleted, is a workaround - under Windows the first column
   //can't be right aligned.
   mList->InsertColumn(0,             wxT(""),              wxLIST_FORMAT_LEFT);
   mList->InsertColumn(ToolColumn + 1,    _("Tool"),            wxLIST_FORMAT_RIGHT);
   mList->InsertColumn(ActionColumn + 1,  _("Command Action"),  wxLIST_FORMAT_RIGHT);
   mList->InsertColumn(ButtonsColumn + 1, _("Buttons"),         wxLIST_FORMAT_LEFT);
   mList->InsertColumn(CommentColumn + 1, _("Comments"),        wxLIST_FORMAT_LEFT);
   mList->DeleteColumn(0);

   AddItem(_("Left-Click"),        _("Select"),   _("Set Selection Point"));
   AddItem(_("Left-Drag"),         _("Select"),   _("Set Selection Range"));
   AddItem(_("Shift-Left-Click"),  _("Select"),   _("Extend Selection Range"));
   AddItem(_("Left-Double-Click"), _("Select"),   _("Select Clip or Entire Track"));
#ifdef EXPERIMENTAL_SCRUBBING_SCROLL_WHEEL
   AddItem(_("Wheel-Rotate"),      _("Select"),   _("Change scrub speed"));
#endif

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
   // JKC: Prompt is disabled for now.  It's a toggle rather than a drag modifier.
   // more like Snap-to than anything else.
   // Spectral selection
   // AddItem(_("ESC"),              _("Select"),    _("Toggle center snapping in spectrogram"));
#endif

   AddItem(_("Left-Click"),       _("Zoom"),      _("Zoom in on Point"));
   AddItem(_("Left-Drag"),        _("Zoom"),      _("Zoom in on a Range"), _("same as right-drag"));
   AddItem(_("Right-Click"),      _("Zoom"),      _("Zoom out one step"));
   AddItem(_("Right-Drag"),       _("Zoom"),      _("Zoom in on a Range"), _("same as left-drag"));
   AddItem(_("Shift-Drag"),       _("Zoom"),      _("Zoom out on a Range"));
   AddItem(_("Middle-Click"),     _("Zoom"),      _("Zoom default"));

   AddItem(_("Left-Drag"),        _("Time-Shift"),_("Move clip left/right or between tracks"));
   AddItem(_("Shift-Left-Drag"),  _("Time-Shift"),_("Move all clips in track left/right"));
   AddItem(CTRL + _("-Left-Drag"),_("Time-Shift"),_("Move clip up/down between tracks"));

   AddItem(_("Left-Drag"),
   /* i18n-hint: The envelope is a curve that controls the audio loudness.*/
      _("Envelope"),
      _("Change Amplification Envelope"));

   AddItem(_("Left-Click"),       _("Pencil"),    _("Change Sample"));
   AddItem(_("Alt-Left-Click"),   _("Pencil"),    _("Smooth at Sample"));
   AddItem(_("Left-Drag"),        _("Pencil"),    _("Change Several Samples"));
   AddItem(CTRL + _("-Left-Drag"),_("Pencil"),    _("Change ONE Sample only"));

   AddItem(_("Left-Click"),       _("Multi"),     _("Set Selection Point"), _("same as select tool"));
   AddItem(_("Left-Drag"),        _("Multi"),     _("Set Selection Range"), _("same as select tool"));
   AddItem(_("Right-Click"),      _("Multi"),     _("Zoom out one step"),   _("same as zoom tool"));
   AddItem(_("Right-Drag"),       _("Multi"),     _("Zoom in on a Range"),  _("same as zoom tool"));

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
   // JKC: Prompt is disabled for now.  It's a toggle rather than a drag modifier.
   // more like Snap-to than anything else.
   // Spectral selection
   // AddItem(_("ESC"),              _("Select"),    _("Toggle center snapping in spectrogram"), _("same as select tool"));
#endif

   AddItem(_("Wheel-Rotate"),                _("Any"),   _("Scroll tracks up or down"));
   AddItem(_("Shift-Wheel-Rotate"),          _("Any"),   _("Scroll waveform"));
   AddItem(CTRL + _("-Wheel-Rotate"),        _("Any"),   _("Zoom waveform in or out"));
   AddItem(CTRL + _("-Shift-Wheel-Rotate"),  _("Any"),   _("Vertical Scale Waveform (dB) range"));

   mList->SetColumnWidth(ToolColumn, wxLIST_AUTOSIZE);
   mList->SetColumnWidth(ActionColumn, wxLIST_AUTOSIZE);
   mList->SetColumnWidth(ButtonsColumn, wxLIST_AUTOSIZE);

   // Not sure if this extra column is a good idea or not.
   // Anyway, 5 pixels wide is wide enough that some people who are curious will drag it
   // wider to see what's there (the comments show that the duplication of functions
   // is for a reason, and not just random).
   mList->SetColumnWidth(CommentColumn, 5);
}

/// Adds an item to mList
void MousePrefs::AddItem(wxString const & buttons, wxString const & tool,
                         wxString const & action, wxString const & comment)
{
   int i = mList->GetItemCount();
   mList->InsertItem(i, tool);
   mList->SetItem(i, ActionColumn, action);
   mList->SetItem(i, ButtonsColumn, buttons);

   // Add a space before the text to work around a minor bug in the
   // list control when showing narrow columns.
   mList->SetItem(i, CommentColumn, wxT(" ") + comment);
}


/// Update the preferences stored on disk.
/// Currently does nothing as Mouse Preferences don't change.
bool MousePrefs::Commit()
{
// Not yet required...
//   ShuttleGui S(this, eIsSavingToPrefs);
//   PopulateOrExchange(S);
   return true;
}

PrefsPanel *MousePrefsFactory::operator () (wxWindow *parent, wxWindowID winid) const
{
   wxASSERT(parent); // to justify safenew
   return safenew MousePrefs(parent, winid);
}
