/**********************************************************************

  Audacity: A Digital Audio Editor

  MidiIOPrefs.cpp

  Joshua Haberman
  Dominic Mazzoni
  James Crook

*******************************************************************//**

\class MidiIOPrefs
\brief A PrefsPanel used to select recording and playback devices and
other settings.

  Presents interface for user to select the recording device and
  playback device, from the list of choices that PortMidi
  makes available.

  Also lets user decide whether or not to record in stereo, and
  whether or not to play other tracks while recording one (duplex).

*//********************************************************************/

#include "../Audacity.h"
#include "MidiIOPrefs.h"

#include "../Experimental.h"

#ifdef EXPERIMENTAL_MIDI_OUT

#include <wx/defs.h>

#include <wx/choice.h>
#include <wx/intl.h>

#include "../../lib-src/portmidi/pm_common/portmidi.h"

#include "../AudioIO.h"
#include "../Internat.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../ShuttleGui.h"
#include "../widgets/ErrorDialog.h"

enum {
   HostID = 10000,
   PlayID,
   RecordID,
   ChannelsID
};

BEGIN_EVENT_TABLE(MidiIOPrefs, PrefsPanel)
   EVT_CHOICE(HostID, MidiIOPrefs::OnHost)
//   EVT_CHOICE(RecordID, MidiIOPrefs::OnDevice)
END_EVENT_TABLE()

MidiIOPrefs::MidiIOPrefs(wxWindow * parent, wxWindowID winid)
/* i18n-hint: untranslatable acronym for "Musical Instrument Device Interface" */
:  PrefsPanel(parent, winid, _("MIDI Devices"))
{
   Populate();
}

MidiIOPrefs::~MidiIOPrefs()
{
}

void MidiIOPrefs::Populate()
{
   // First any pre-processing for constructing the GUI.
   GetNamesAndLabels();

   // Get current setting for devices
   mPlayDevice = gPrefs->Read(wxT("/MidiIO/PlaybackDevice"), wxT(""));
#ifdef EXPERIMENTAL_MIDI_IN
   mRecordDevice = gPrefs->Read(wxT("/MidiIO/RecordingDevice"), wxT(""));
#endif
//   mRecordChannels = gPrefs->Read(wxT("/MidiIO/RecordChannels"), 2L);

   //------------------------- Main section --------------------
   // Now construct the GUI itself.
   // Use 'eIsCreatingFromPrefs' so that the GUI is
   // initialised with values from gPrefs.
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);
   // ----------------------- End of main section --------------

   wxCommandEvent e;
   OnHost(e);
}

/// Gets the lists of names and lists of labels which are
/// used in the choice controls.
/// The names are what the user sees in the wxChoice.
/// The corresponding labels are what gets stored.
void MidiIOPrefs::GetNamesAndLabels() {
   // Gather list of hosts.  Only added hosts that have devices attached.
   Pm_Terminate(); // close and open to refresh device lists
   Pm_Initialize();
   int nDevices = Pm_CountDevices();
   for (int i = 0; i < nDevices; i++) {
      const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
      if (info->output || info->input) { //should always happen
         wxString name = wxSafeConvertMB2WX(info->interf);
         if ( ! make_iterator_range( mHostNames ).contains( name ) ) {
            mHostNames.push_back(name);
            mHostLabels.push_back(name);
         }
      }
   }
}

void MidiIOPrefs::PopulateOrExchange( ShuttleGui & S ) {

   S.SetBorder(2);
   S.StartScroller();

   S.StartStatic(_("Interface"));
   {
      S.StartMultiColumn(2);
      {
         S.Id(HostID);
         /* i18n-hint: (noun) */
         mHost = S.TieChoice(_("&Host:"),
                             wxT("/MidiIO/Host"),
                             wxT(""),
                             mHostNames,
                             mHostLabels);

         S.AddPrompt(_("Using: PortMidi"));
      }
      S.EndMultiColumn();
   }
   S.EndStatic();

   S.StartStatic(_("Playback"));
   {
      S.StartMultiColumn(2);
      {
         S.Id(PlayID);
         mPlay = S.AddChoice(_("&Device:"),
                             {} );
         mLatency = S.TieNumericTextBox(_("MIDI Synth L&atency (ms):"),
                                        wxT("/MidiIO/SynthLatency"),
                                        DEFAULT_SYNTH_LATENCY, 3);
      }
      S.EndMultiColumn();
   }
   S.EndStatic();
#ifdef EXPERIMENTAL_MIDI_IN
   S.StartStatic(_("Recording"));
   {
      S.StartMultiColumn(2);
      {
         S.Id(RecordID);
         mRecord = S.AddChoice(_("De&vice:"),
                               {} );

         S.Id(ChannelsID);
         /*
         mChannels = S.AddChoice(_("&Channels:"),
                                 wxEmptyString,
                                 {} );
         */
      }
      S.EndMultiColumn();
   }
   S.EndStatic();
#endif
   S.EndScroller();

}

void MidiIOPrefs::OnHost(wxCommandEvent & WXUNUSED(e))
{
   wxString itemAtIndex;
   int index = mHost->GetCurrentSelection();
   if (index >= 0 && index < (int)mHostNames.size())
      itemAtIndex = mHostNames[index];
   int nDevices = Pm_CountDevices();

   if (nDevices == 0) {
      mHost->Clear();
      mHost->Append(_("No MIDI interfaces"), (void *) NULL);
      mHost->SetSelection(0);
   }

   mPlay->Clear();
#ifdef EXPERIMENTAL_MIDI_IN
   mRecord->Clear();
#endif

   wxArrayStringEx playnames;
   wxArrayStringEx recordnames;

   for (int i = 0; i < nDevices; i++) {
      const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
      wxString interf = wxSafeConvertMB2WX(info->interf);
      if (itemAtIndex == interf) {
         wxString name = wxSafeConvertMB2WX(info->name);
         wxString device = wxString::Format(wxT("%s: %s"),
                                            interf,
                                            name);
         if (info->output) {
            playnames.push_back(name);
            index = mPlay->Append(name, (void *) info);
            if (device == mPlayDevice) {
               mPlay->SetSelection(index);
            }
         }
#ifdef EXPERIMENTAL_MIDI_IN
         if (info->input) {
            recordnames.push_back(name);
            index = mRecord->Append(name, (void *) info);
            if (device == mRecordDevice) {
               mRecord->SetSelection(index);
            }
         }
#endif
      }
   }

   if (mPlay->GetCount() == 0) {
      playnames.push_back(_("No devices found"));
      mPlay->Append(playnames[0], (void *) NULL);
   }
#ifdef EXPERIMENTAL_MIDI_IN
   if (mRecord->GetCount() == 0) {
      recordnames.push_back(_("No devices found"));
      mRecord->Append(recordnames[0], (void *) NULL);
   }
#endif
   if (mPlay->GetCount() && mPlay->GetSelection() == wxNOT_FOUND) {
      mPlay->SetSelection(0);
   }
#ifdef EXPERIMENTAL_MIDI_IN
   if (mRecord->GetCount() && mRecord->GetSelection() == wxNOT_FOUND) {
      mRecord->SetSelection(0);
   }
#endif
   ShuttleGui S(this, eIsCreating);
   S.SetSizeHints(mPlay, playnames);
#ifdef EXPERIMENTAL_MIDI_IN
   S.SetSizeHints(mRecord, recordnames);
#endif
//   OnDevice(e);
}

bool MidiIOPrefs::Commit()
{
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   const PmDeviceInfo *info;

   info = (const PmDeviceInfo *) mPlay->GetClientData(mPlay->GetSelection());
   if (info) {
      gPrefs->Write(wxT("/MidiIO/PlaybackDevice"),
                    wxString::Format(wxT("%s: %s"),
                                     wxString(wxSafeConvertMB2WX(info->interf)),
                                     wxString(wxSafeConvertMB2WX(info->name))));
   }
#ifdef EXPERIMENTAL_MIDI_IN
   info = (const PmDeviceInfo *) mRecord->GetClientData(mRecord->GetSelection());
   if (info) {
      gPrefs->Write(wxT("/MidiIO/RecordingDevice"),
                    wxString::Format(wxT("%s: %s"),
                                     wxString(wxSafeConvertMB2WX(info->interf)),
                                     wxString(wxSafeConvertMB2WX(info->name))));
   }
#endif
   return gPrefs->Flush();
}

bool MidiIOPrefs::Validate()
{
   long latency;
   if (!mLatency->GetValue().ToLong(&latency)) {
      AudacityMessageBox(_("The MIDI Synthesizer Latency must be an integer"));
      return false;
   }
   return true;
}

wxString MidiIOPrefs::HelpPageName()
{
   return "MIDI_Devices_Preferences";
}

PrefsPanel *MidiIOPrefsFactory::operator () (wxWindow *parent, wxWindowID winid) const
{
   wxASSERT(parent); // to justify safenew
   return safenew MidiIOPrefs(parent, winid);
}

#endif
