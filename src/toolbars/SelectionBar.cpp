/**********************************************************************

  Audacity: A Digital Audio Editor

  SelectionBar.cpp

  Copyright 2005 Dominic Mazzoni

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

*******************************************************************//**

\class SelectionBar
\brief (not quite a Toolbar) at foot of screen for setting and viewing the
selection range.

*//****************************************************************//**

\class SelectionBarListener
\brief A parent class of SelectionBar, used to forward events to do
with changes in the SelectionBar.

*//*******************************************************************/


#include "../Audacity.h"
#include "SelectionBar.h"

#include "SelectionBarListener.h"
#include "ToolManager.h"

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#include <wx/setup.h> // for wxUSE_* macros

#ifndef WX_PRECOMP
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/combobox.h>
#include <wx/intl.h>
#include <wx/radiobut.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/valtext.h>
#include <wx/stattext.h>
#endif
#include <wx/statline.h>


#include "../widgets/AButton.h"
#include "../AudioIOBase.h"
#include "../AColor.h"
#include "../KeyboardCapture.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../ProjectAudioIO.h"
#include "../ProjectSettings.h"
#include "../Snap.h"
#include "../ViewInfo.h"
#include "../AllThemeResources.h"

#if wxUSE_ACCESSIBILITY
#include "../widgets/WindowAccessible.h"
#endif

IMPLEMENT_CLASS(SelectionBar, ToolBar);

const static wxChar *numbers[] =
{
   wxT("0"), wxT("1"), wxT("2"), wxT("3"), wxT("4"),
   wxT("5"), wxT("6"), wxT("7"), wxT("8"), wxT("9")
};

enum {
   SelectionBarFirstID = 2700,
   RateID,
   SnapToID,
   OnMenuID,

   ChoiceID,

   StartTimeID,
   LengthTimeID,
   CenterTimeID,
   EndTimeID,

   AudioTimeID,
};

BEGIN_EVENT_TABLE(SelectionBar, ToolBar)
   EVT_SIZE(SelectionBar::OnSize)
   EVT_TEXT(StartTimeID, SelectionBar::OnChangedTime)
   EVT_TEXT(LengthTimeID, SelectionBar::OnChangedTime)
   EVT_TEXT(CenterTimeID, SelectionBar::OnChangedTime)
   EVT_TEXT(EndTimeID, SelectionBar::OnChangedTime)
   EVT_CHOICE(SnapToID, SelectionBar::OnSnapTo)
   EVT_CHOICE(ChoiceID, SelectionBar::OnChoice )
   EVT_IDLE( SelectionBar::OnIdle )
   EVT_COMBOBOX(RateID, SelectionBar::OnRate)
   EVT_TEXT(RateID, SelectionBar::OnRate)

   EVT_COMMAND(wxID_ANY, EVT_TIMETEXTCTRL_UPDATED, SelectionBar::OnUpdate)
   EVT_COMMAND(wxID_ANY, EVT_CAPTURE_KEY, SelectionBar::OnCaptureKey)
END_EVENT_TABLE()

SelectionBar::SelectionBar( AudacityProject &project )
: ToolBar(project, SelectionBarID, _("Selection"), wxT("Selection")),
  mListener(NULL), mRate(0.0),
  mStart(0.0), mEnd(0.0), mLength(0.0), mCenter(0.0), mAudio(0.0),
  mDrive1( StartTimeID), mDrive2( EndTimeID ),
  mSelectionMode(0),
  mStartTime(NULL), mCenterTime(NULL), mLengthTime(NULL), mEndTime(NULL),
  mAudioTime(NULL),
  mChoice(NULL)
{
   // Make sure we have a valid rate as the NumericTextCtrl()s
   // created in Populate()
   // depend on it.  Otherwise, division-by-zero floating point exceptions
   // will occur.
   // Refer to bug #462 for a scenario where the division-by-zero causes
   // Audacity to fail.
   // We expect mRate to be set from the project later.
   mRate = (double) gPrefs->Read(wxT("/SamplingRate/DefaultProjectSampleRate"),
      AudioIOBase::GetOptimalSupportedSampleRate());

   // Selection mode of 0 means showing 'start' and 'end' only.
   mSelectionMode = gPrefs->ReadLong(wxT("/SelectionToolbarMode"),  0);
}

SelectionBar::~SelectionBar() = default;

SelectionBar &SelectionBar::Get( AudacityProject &project )
{
   auto &toolManager = ToolManager::Get( project );
   return *static_cast<SelectionBar*>( toolManager.GetToolBar(SelectionBarID) );
}

const SelectionBar &SelectionBar::Get( const AudacityProject &project )
{
   return Get( const_cast<AudacityProject&>( project )) ;
}

void SelectionBar::Create(wxWindow * parent)
{
   ToolBar::Create(parent);
   UpdatePrefs();
}

auStaticText * SelectionBar::AddTitle( const wxString & Title, wxSizer * pSizer ){
   auStaticText * pTitle = safenew auStaticText(this, Title );
   pTitle->SetBackgroundColour( theTheme.Colour( clrMedium ));
   pTitle->SetForegroundColour( theTheme.Colour( clrTrackPanelText ) );
   pSizer->Add( pTitle,0, wxALIGN_CENTER_VERTICAL | wxRIGHT,  (Title.length() == 1 ) ? 0:5);
   return pTitle;
}


NumericTextCtrl * SelectionBar::AddTime( const wxString Name, int id, wxSizer * pSizer ){
   auto formatName = mListener ? mListener->AS_GetSelectionFormat()
      : NumericFormatSymbol{};
   auto pCtrl = safenew NumericTextCtrl(
      this, id, NumericConverter::TIME, formatName, 0.0, mRate);
   pCtrl->SetName(Name);
   pSizer->Add(pCtrl, 0, wxALIGN_TOP | wxRIGHT, 5);
   return pCtrl;
}

void SelectionBar::AddVLine(  wxSizer * pSizer ){
   pSizer->Add(safenew wxStaticLine(this, -1, wxDefaultPosition,
                                   wxSize(1, toolbarSingle-10),
                                   wxLI_VERTICAL),
                  0,  wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
}

void SelectionBar::Populate()
{
   SetBackgroundColour( theTheme.Colour( clrMedium  ) );

   mStartTime = mEndTime = mLengthTime = mCenterTime = mAudioTime = nullptr;

   // This will be inherited by all children:
   SetFont(wxFont(
#ifdef __WXMAC__
                  12
#else
                  9
#endif
                  ,
                  wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

   wxFlexGridSizer *mainSizer;

   /* we don't actually need a control yet, but we want to use its methods
    * to do some look-ups, so we'll have to create one. We can't make the
    * look-ups static because they depend on translations which are done at
    * runtime */

   // Outer sizer has space top and left.
   // Inner sizers have space on right only.
   // This choice makes for a nice border and internal spacing and places clear responsibility
   // on each sizer as to what spacings it creates.
   Add((mainSizer = safenew wxFlexGridSizer(SIZER_COLS, 1, 1)), 0, wxALIGN_TOP | wxLEFT | wxTOP, 5);

   // Top row (mostly labels)
   wxColour clrText =  theTheme.Colour( clrTrackPanelText );
   wxColour clrText2 = *wxBLUE;
   AddTitle( _("Project Rate (Hz)"), mainSizer );
   AddVLine( mainSizer );
   AddTitle( _("Snap-To"), mainSizer );
   AddVLine( mainSizer );
   AddTitle( _("Audio Position"), mainSizer );
   AddVLine( mainSizer );

   {
      const wxString choices[4] = {
         _("Start and End of Selection"),
         _("Start and Length of Selection"),
         _("Length and End of Selection"),
         _("Length and Center of Selection"),
      };
      mChoice = safenew wxChoice
         (this, ChoiceID, wxDefaultPosition, wxDefaultSize, 4, choices,
          0, wxDefaultValidator, _("Show"));
      mChoice->SetSelection(0);
#if wxUSE_ACCESSIBILITY
      // so that name can be set on a standard control
      mChoice->SetAccessible(safenew WindowAccessible(mChoice));
#endif
#ifdef __WXGTK__
      // Combo boxes are taller on Linux, and if we don't do the following, the selection toolbar will
      // be three units high.
      wxSize sz = mChoice->GetBestSize();
      sz.SetHeight( sz.y-4);
      mChoice->SetMinSize( sz );
#endif
      mainSizer->Add(mChoice, 0, wxALIGN_TOP | wxEXPAND | wxRIGHT, 6);
   }

   // Botton row, (mostly time controls)
   mRateBox = safenew wxComboBox(this, RateID,
                             wxT(""),
                             wxDefaultPosition, wxSize(80, -1));
#if wxUSE_ACCESSIBILITY
   // so that name can be set on a standard control
   mRateBox->SetAccessible(safenew WindowAccessible(mRateBox));
#endif
   mRateBox->SetName(_("Project Rate (Hz)"));
   //mRateBox->SetForegroundColour( clrText2 );
   wxTextValidator vld(wxFILTER_INCLUDE_CHAR_LIST);
   vld.SetIncludes(wxArrayString(10, numbers));
   mRateBox->SetValidator(vld);
   mRateBox->SetValue(wxString::Format(wxT("%d"), (int)mRate));
   UpdateRates(); // Must be done _after_ setting value on mRateBox!

   // We need to capture the SetFocus and KillFocus events to set up
   // for keyboard capture.  On Windows and GTK it's easy since the
   // combobox is presented as one control to hook into.
   mRateText = mRateBox;

#if defined(__WXMAC__)
   // The Mac uses a standard wxTextCtrl for the edit portion and that's
   // the control that gets the focus events.  So we have to find the
   // textctrl.
   wxWindowList kids = mRateBox->GetChildren();
   for (unsigned int i = 0; i < kids.size(); i++) {
      wxClassInfo *ci = kids[i]->GetClassInfo();
      if (ci->IsKindOf(CLASSINFO(wxTextCtrl))) {
         mRateText = kids[i];
         break;
      }
   }
#endif

   mRateText->Bind(wxEVT_SET_FOCUS,
                      &SelectionBar::OnFocus,
                      this);
   mRateText->Bind(wxEVT_KILL_FOCUS,
                      &SelectionBar::OnFocus,
                      this);

#ifdef __WXGTK__
   // Combo boxes are taller on Linux, and if we don't do the following, the selection toolbar will
   // be three units high.
   wxSize sz = mRateBox->GetBestSize();
   sz.SetHeight( sz.y-4);
   mRateBox->SetMinSize( sz );
#endif

   mainSizer->Add(mRateBox, 0, wxALIGN_TOP | wxRIGHT, 5);
   AddVLine( mainSizer );

   mSnapTo = safenew wxChoice(this, SnapToID,
                          wxDefaultPosition, wxDefaultSize,
                          SnapManager::GetSnapLabels());

#ifdef __WXGTK__
   // Combo boxes are taller on Linux, and if we don't do the following, the selection toolbar will
   // be three units high.
   sz = mSnapTo->GetBestSize();
   sz.SetHeight( sz.y-4);
   mSnapTo->SetMinSize( sz );
#endif

   mainSizer->Add(mSnapTo,
                  0, wxALIGN_TOP | wxRIGHT, 5);
#if wxUSE_ACCESSIBILITY
   // so that name can be set on a standard control
   mSnapTo->SetAccessible(safenew WindowAccessible(mSnapTo));
#endif
   mSnapTo->SetName(_("Snap To"));
   //mSnapTo->SetForegroundColour( clrText2 );
   mSnapTo->SetSelection(mListener ? mListener->AS_GetSnapTo() : SNAP_OFF);

   mSnapTo->Bind(wxEVT_SET_FOCUS,
                    &SelectionBar::OnFocus,
                    this);
   mSnapTo->Bind(wxEVT_KILL_FOCUS,
                    &SelectionBar::OnFocus,
                    this);

   AddVLine( mainSizer );

   mAudioTime = AddTime(_("Audio Position"), AudioTimeID, mainSizer );
   // This vertical line is NOT just for decoration!
   // It works around a wxWidgets-on-Windows RadioButton bug, where tabbing
   // into the radiobutton group jumps to selecting the first item in the 
   // group even if some other item had been selected.
   // It is an important bug to work around for sceen reader users, who use TAB 
   // a lot in navigation.
   // More about the bug here:
   // https://forums.wxwidgets.org/viewtopic.php?t=41120
   AddVLine( mainSizer );

   {
      auto hSizer = std::make_unique<wxBoxSizer>(wxHORIZONTAL);

      mStartTime  = AddTime(_("Start"), StartTimeID, hSizer.get() );
      mLengthTime = AddTime(_("Length"), LengthTimeID, hSizer.get() );
      mCenterTime = AddTime(_("Center"), CenterTimeID, hSizer.get() );
      mEndTime    = AddTime(_("End"), EndTimeID, hSizer.get() );
      mainSizer->Add(hSizer.release(), 0, wxALIGN_TOP | wxRIGHT, 0);
   }

   mChoice->MoveBeforeInTabOrder( mStartTime );
   // This shows/hides controls.
   // Do this before layout so that we are sized right.
   SetSelectionMode(mSelectionMode);
   mainSizer->Layout();
   RegenerateTooltips();
   Layout();

   SetMinSize( GetSizer()->GetMinSize() );
}

void SelectionBar::UpdatePrefs()
{
   // The project rate is no longer driven from here.
   // When preferences change, the Project learns about it too.
   // If necessary we can drive the SelectionBar mRate via the Project
   // calling our SetRate().
   // As of 13-Sep-2018, changes to the sample rate pref will only affect 
   // creation of new projects, not the smaple rate in existing ones.

   wxCommandEvent e;
   e.SetInt(mStartTime->GetFormatIndex());
   OnUpdate(e);

   // Set label to pull in language change
   SetLabel(_("Selection"));

   RegenerateTooltips();
   // Give base class a chance
   ToolBar::UpdatePrefs();
}

void SelectionBar::SetListener(SelectionBarListener *l)
{
   mListener = l;
   SetRate(mListener->AS_GetRate());
   SetSnapTo(mListener->AS_GetSnapTo());
   SetSelectionFormat(mListener->AS_GetSelectionFormat());
};

void SelectionBar::RegenerateTooltips()
{
#if wxUSE_TOOLTIPS
   auto formatName =
      mListener
         ? mListener->AS_GetSelectionFormat()
         : NumericFormatSymbol{};
   mSnapTo->SetToolTip(
      wxString::Format(
         _("Snap Clicks/Selections to %s"), formatName.Translation() ));
#endif
}

void SelectionBar::OnSize(wxSizeEvent &evt)
{
   Refresh( true );

   evt.Skip();
}

// When a control value is changed, this function is called.
// It determines the values for the other controls.
void SelectionBar::ModifySelection(int newDriver, bool done)
{
   // If the user moved to a different control, then update which
   // two controls drive the others.
   if( newDriver != mDrive2 )
      SetDrivers( mDrive2, newDriver);

   // Only update a value if user typed something in.
   // The reason is the controls may be less accurate than 
   // the values.
   if( newDriver == StartTimeID )
      mStart = mStartTime->GetValue();
   if( newDriver == EndTimeID )
      mEnd = mEndTime->GetValue();
   if( newDriver == LengthTimeID )
      mLength = mLengthTime->GetValue();
   if( newDriver == CenterTimeID )
      mCenter = mCenterTime->GetValue();

   // There are four controls, and two constraints, which are:
   //    mid = (start+end)/2
   //    length = (end-start)
   // Therefore we can select any two controls as 'drivers' of
   // the other two.
   // Here we compute 'i' which combines the identity of the two 
   // driving controls, to use it as an index.
   // The order of the two drivers generally does not matter much,
   // except that we have want:
   //    start < end
   // and preserve that by adjusting the least dominant driving
   // control.
   int i = mDrive1 + 4 * mDrive2;
   switch(i){
   case StartTimeID + 4 * EndTimeID:
      if( mEnd < mStart )
         mStart = mEnd;
   case StartTimeID * 4 + EndTimeID:
      if( mStart > mEnd )
         mEnd = mStart;
      mLength = mEnd - mStart;
      mCenter = (mStart+mEnd)/2.0;
      break;
   case StartTimeID + 4 * LengthTimeID:
   case StartTimeID * 4 + LengthTimeID:
      if( mLength < 0 )
         mLength = 0;
      mEnd = mStart+mLength;
      mCenter = (mStart+mEnd)/2.0;
      break;
   case StartTimeID + 4 * CenterTimeID:
      if( mCenter < mStart )
         mCenter = mStart;
   case StartTimeID * 4 + CenterTimeID:
      if( mStart > mCenter )
         mStart = mCenter;
      mEnd = mCenter * 2 - mStart;
      mLength = mStart - mEnd;
      break;
   case EndTimeID + 4 * LengthTimeID:
   case EndTimeID * 4 + LengthTimeID:
      if( mLength < 0 )
         mLength = 0;
      mStart = mEnd - mLength;
      mCenter = (mStart+mEnd)/2.0;
      break;
   case EndTimeID + 4 * CenterTimeID:
      if( mCenter > mEnd )
         mCenter = mEnd;
   case EndTimeID * 4 + CenterTimeID:
      if( mEnd < mCenter )
         mEnd = mCenter;
      mStart = mCenter * 2.0 - mEnd;
      mLength = mEnd - mStart;
      break;
   case LengthTimeID + 4 * CenterTimeID:
   case LengthTimeID * 4 + CenterTimeID:
      if( mLength < 0 )
         mLength = 0;
      mStart = mCenter - mLength/2.0;
      mEnd = mCenter + mLength/2.0;
      break;
   default:
      // The above should cover all legal combinations of two distinct controls.
      wxFAIL_MSG( "Illegal sequence of selection changes");
   }

   // Places the start-end mrkers on the track panel.
   mListener->AS_ModifySelection(mStart, mEnd, done);
}

void SelectionBar::OnChangedTime(wxCommandEvent & event)
{
   ModifySelection(event.GetId(), event.GetInt() != 0);
}

// Called when one of the format drop downs is changed.
void SelectionBar::OnUpdate(wxCommandEvent &evt)
{
   int index = evt.GetInt();
   wxWindow *w = FindFocus();
   NumericTextCtrl ** Ctrls[5] = { &mStartTime, &mEndTime, &mLengthTime, &mCenterTime, &mAudioTime };
   int i;
   int iFocus = -1;
   for(i=0;i<5;i++)
      if( w == *Ctrls[i] )
         iFocus = i;

   evt.Skip(false);

   // Save format name before recreating the controls so they resize properly
   {
      auto format = mStartTime->GetBuiltinName(index);
      if (mListener)
         mListener->AS_SetSelectionFormat(format);
   }

   RegenerateTooltips();

   // ToolBar::ReCreateButtons() will get rid of our sizers and controls
   // so reset pointers first.
   for( i=0;i<5;i++)
      *Ctrls[i]=NULL;

   mRateBox = NULL;
   mRateText = NULL;

   ToolBar::ReCreateButtons();

   ValuesToControls();

   auto format = mStartTime->GetBuiltinFormat(index);
   for( i=0;i<5;i++)
      (*Ctrls[i])->SetFormatString( format );

   if( iFocus >=0 )
      (*Ctrls[iFocus])->SetFocus();
   Updated();
}

// The two drivers are the numbers of the two controls which drive the other ones.
// The user gets to see which controls are drivers and which driven.
void SelectionBar::SetDrivers( int driver1, int driver2 )
{
   mDrive1 = driver1;
   mDrive2 = driver2;

   NumericTextCtrl ** Ctrls[4] = { &mStartTime, &mCenterTime, &mLengthTime, &mEndTime};
   wxString Text[4] = { _("Start"), _("Center"), _("Length"),  _("End")  };

   for(int i=0;i<4;i++){
      int id = i + StartTimeID;
      int fixed = (( id == mDrive2 )?mDrive1:mDrive2)-StartTimeID;

      wxString Temp = Text[i];
      // i18n-hint: %s is replaced e.g by 'Length', to indicate that it will be calculated from other parameters.
      wxString Format = ((id != mDrive1) && (id != mDrive2)) ? _("%s - driven") : wxString{ "%s" };
      wxString Title= wxString::Format( Format, Temp );
      // i18n-hint: %s1 is replaced e.g by 'Length', %s2 e.g by 'Center'.
      wxString VoiceOverText = wxString::Format(_("Selection %s. %s won't change."), Temp, Text[fixed]);
      if( *Ctrls[i] ){
         (*Ctrls[i])->SetName( Temp );
      }
   }
}

void SelectionBar::OnChoice(wxCommandEvent & WXUNUSED(event))
{
   int mode = mChoice->GetSelection();
   SetSelectionMode( mode );
   SelectionModeUpdated();
}

void SelectionBar::OnIdle( wxIdleEvent &evt )
{
   evt.Skip();
   auto &project = mProject;
   const auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;

   double audioTime;

   auto &projectAudioIO = ProjectAudioIO::Get( project );
   if ( projectAudioIO.IsAudioActive() ){
      auto gAudioIO = AudioIOBase::Get();
      audioTime = gAudioIO->GetStreamTime();
   }
   else {
      const auto &playRegion = ViewInfo::Get( project ).playRegion;
      audioTime = playRegion.GetStart();
   }

   SetTimes(selectedRegion.t0(), selectedRegion.t1(), audioTime);
}

void SelectionBar::SelectionModeUpdated()
{
   // We just changed the mode.  Remember it.
   gPrefs->Write(wxT("/SelectionToolbarMode"), mSelectionMode);
   gPrefs->Flush();

   wxSize sz = GetMinSize();
   sz.SetWidth( 10 );
   SetMinSize( sz );
   Fit();
   Layout();
   Updated();
}

// We used to have 8 modes which showed different combinations of the
// length, start, end, center controls.
// Mode 7 for example showed all four at the same time.
void SelectionBar::SetSelectionMode(int mode)
{
   // Only modes 0 to 3 are now supported,
   // so fix up a mode that could have come from the config.
   const int maxMode = 3;

   if( mode > maxMode )
      mode = 0;
   if( mode < 0 )
      mode = 0;
   mSelectionMode = mode;
   mChoice->SetSelection( mode ); 

   // First decide which two controls drive the others...
   // For example the last option is with all controls shown, and in that mode we 
   // initially have start and end driving.
   int Drive2[] = { StartTimeID, StartTimeID,  LengthTimeID, LengthTimeID, 
                    StartTimeID, StartTimeID,  StartTimeID,  StartTimeID};
   int Drive1[] = { EndTimeID,   LengthTimeID, EndTimeID,    CenterTimeID, 
                    EndTimeID,   LengthTimeID, EndTimeID,    EndTimeID};

   SetDrivers( Drive1[mode], Drive2[mode] );
   // Then show/hide the relevant controls.
   ShowHideControls( mode );
}

// Our mode determines which controls are visible.
void SelectionBar::ShowHideControls(int mode)
{
   // The bits in these say which controls are visible.
   int masks[8]= { 
      9, 5, 12, 6, // 2 items shown
      13, 7, 11,// 3 items shown
      15};
   int mask = masks[mode];

   NumericTextCtrl ** Ctrls[4]  = { &mStartTime,  &mCenterTime,  &mLengthTime,  &mEndTime};
   for(int i=0;i<4;i++){
      if( *Ctrls[i]) 
         (*Ctrls[i])->Show( (mask & (1<<i))!=0 );
   }
}

void SelectionBar::ValuesToControls()
{
   NumericTextCtrl ** Ctrls[5] = { &mStartTime, &mEndTime, &mLengthTime, &mCenterTime, &mAudioTime };
   double Values[5] = {mStart, mEnd, mLength, mCenter, mAudio };
   int i;
   for(i=0;i<5;i++)
      if( *Ctrls[i] )
         (*Ctrls[i])->SetValue( Values[i] );
}

// A time has been set.  Update the control values.
void SelectionBar::SetTimes(double start, double end, double audio)
{
   if ( start != mStart || end != mEnd || audio != mAudio
      || mLastSelectionMode != mSelectionMode
   ) {
      mStart = start;
      mEnd = end;
      mLength = end-start;
      mCenter = (end+start)/2.0;
      mAudio = audio;
      mLastSelectionMode = mSelectionMode;

      ValuesToControls();
   }
}

void SelectionBar::SetSnapTo(int snap)
{
   mSnapTo->SetSelection(snap);
}

void SelectionBar::SetSelectionFormat(const NumericFormatSymbol & format)
{
   bool changed =
      mStartTime->SetFormatString(mStartTime->GetBuiltinFormat(format));

   // Test first whether changed, to avoid infinite recursion from OnUpdate
   if ( changed ) {
      wxCommandEvent e;
      e.SetInt(mStartTime->GetFormatIndex());
      OnUpdate(e);
   }
}

void SelectionBar::SetRate(double rate)
{
   if (rate != mRate) {
      // if the rate is actually being changed
      mRate = rate;   // update the stored rate
      mRateBox->SetValue(wxString::Format(wxT("%d"), (int)rate));

      // update the TimeTextCtrls if they exist
      NumericTextCtrl ** Ctrls[5] = { &mStartTime, &mEndTime, &mLengthTime, &mCenterTime, &mAudioTime };
      int i;
      for(i=0;i<5;i++)
         if( *Ctrls[i] )
            (*Ctrls[i])->SetSampleRate( rate );
   }
}

void SelectionBar::OnRate(wxCommandEvent & WXUNUSED(event))
{
   if (mRateBox->GetValue().ToDouble(&mRate) && // is a numeric value
         (mRate != 0.0))
   {
      NumericTextCtrl ** Ctrls[5] = { &mStartTime, &mEndTime, &mLengthTime, &mCenterTime, &mAudioTime };
      int i;
      for(i=0;i<5;i++)
         if( *Ctrls[i] )
            (*Ctrls[i])->SetSampleRate( mRate );
      if (mListener) mListener->AS_SetRate(mRate);
   }
}

void SelectionBar::UpdateRates()
{
   wxString oldValue = mRateBox->GetValue();
   mRateBox->Clear();
   for (int i = 0; i < AudioIOBase::NumStandardRates; i++) {
      mRateBox->Append(
         wxString::Format(wxT("%d"), AudioIOBase::StandardRates[i]));
   }
   mRateBox->SetValue(oldValue);
}

void SelectionBar::OnFocus(wxFocusEvent &event)
{
   KeyboardCapture::OnFocus( *this, event );
}

void SelectionBar::OnCaptureKey(wxCommandEvent &event)
{
   wxKeyEvent *kevent = (wxKeyEvent *)event.GetEventObject();
   wxWindow *w = FindFocus();
   int keyCode = kevent->GetKeyCode();

   // Convert numeric keypad entries.
   if ((keyCode >= WXK_NUMPAD0) && (keyCode <= WXK_NUMPAD9)) {
      keyCode -= WXK_NUMPAD0 - '0';
   }

   if (keyCode >= '0' && keyCode <= '9') {
      return;
   }

   // UP/DOWN/LEFT/RIGHT for mRateText
   if (w == mRateText) {
      switch (keyCode)
      {
         case WXK_LEFT:
         case WXK_RIGHT:
         case WXK_UP:
         case WXK_DOWN:
         case WXK_DELETE:
         case WXK_BACK:
            return;
      }
   }

   event.Skip();
}

void SelectionBar::OnSnapTo(wxCommandEvent & WXUNUSED(event))
{
   mListener->AS_SetSnapTo(mSnapTo->GetSelection());
}

static RegisteredToolbarFactory factory{ SelectionBarID,
   []( AudacityProject &project ){
      return ToolBar::Holder{ safenew SelectionBar{ project } }; }
};
