/**********************************************************************

  Audacity: A Digital Audio Editor

  Prefs.cpp

  Dominic Mazzoni

*******************************************************************//*!

\file Prefs.cpp
\brief Utility functions for working with our wxConf (gPrefs)


  Audacity uses wxWidgets' wxConfig class to handle preferences.
  See Prefs.h for more information on how it works...

\verbatim
  Note: The info below is very outdated and incomplete

  Preference field specification:
   /
      Version					- Audacity Version that created these prefs
      DefaultOpenPath			- Default directory for NEW file selector
   /FileFormats
      CopyOrEditUncompressedData - Copy data from uncompressed files or
         [ "copy", "edit"]   - edit in place?
      ExportFormat_SF1		   - Format to export PCM data in
                             (this number is a libsndfile1.0 format)
   /SamplingRate
      DefaultProjectSampleRate- New projects will have this rate
         [ 8000, 11025, 16000, 22050, 44100, 48000 ]
   /AudioIO
      PlaybackDevice			- device to use for playback
      RecordingDevice			- device to use for recording
         (these are device names understood by PortAudio)
   /Display
      WaveformColor			- 0xRRGGBB  --since it will be stored in
      ShadowColor				- 			  decimal, it will be somewhat
      SpectrumLowColor		- 			  non-intuitive to edit, but
      SpectrumHighColor		- 			  much easier to parse.
   /Locale
      Language				- two-letter language code for translations

   (*): wxGTK
   (+): wxWin
   ($): wxMac
\endverbatim

*//*******************************************************************/


#include "Audacity.h"
#include "Prefs.h"

#include <wx/defs.h>
#include <wx/app.h>
#include <wx/intl.h>
#include <wx/fileconf.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

#include "FileNames.h"
#include "MemoryX.h"
#include "Languages.h"

std::unique_ptr<AudacityPrefs> ugPrefs {};

AUDACITY_DLL_API AudacityPrefs *gPrefs = NULL;
int gMenusDirty = 0;

wxDEFINE_EVENT(EVT_PREFS_UPDATE, wxCommandEvent);

PrefsListener::PrefsListener()
{
   wxTheApp->Bind(EVT_PREFS_UPDATE, &PrefsListener::OnEvent, this);
}

PrefsListener::~PrefsListener()
{
   // Explicit unbinding is needed because this is not a wxEvtHandler
   wxTheApp->Unbind(EVT_PREFS_UPDATE, &PrefsListener::OnEvent, this);
}

void PrefsListener::UpdateSelectedPrefs( int )
{
}

void PrefsListener::OnEvent( wxCommandEvent &evt )
{
   evt.Skip();
   auto id = evt.GetId();
   if (id <= 0)
      UpdatePrefs();
   else
      UpdateSelectedPrefs( id );
}

#if 0
// Copy one entry from one wxConfig object to another
static void CopyEntry(wxString path, wxConfigBase *src, wxConfigBase *dst, wxString entry)
{
   switch(src->GetEntryType(entry)) {
   case wxConfigBase::Type_Unknown:
   case wxConfigBase::Type_String: {
      wxString value = src->Read(entry, wxT(""));
      dst->Write(path + entry, value);
      break;
   }
   case wxConfigBase::Type_Boolean: {
      bool value = false;
      src->Read(entry, &value, value);
      dst->Write(path + entry, value);
      break;
   }
   case wxConfigBase::Type_Integer: {
      long value = false;
      src->Read(entry, &value, value);
      dst->Write(path + entry, value);
      break;
   }
   case wxConfigBase::Type_Float: {
      double value = false;
      src->Read(entry, &value, value);
      dst->Write(path + entry, value);
      break;
   }
   }
}


// Recursive routine to copy all groups and entries from one wxConfig object to another
static void CopyEntriesRecursive(wxString path, wxConfigBase *src, wxConfigBase *dst)
{
   wxString entryName;
   long entryIndex;
   bool entryKeepGoing;

   entryKeepGoing = src->GetFirstEntry(entryName, entryIndex);
   while (entryKeepGoing) {
      CopyEntry(path, src, dst, entryName);
      entryKeepGoing = src->GetNextEntry(entryName, entryIndex);
   }

   wxString groupName;
   long groupIndex;
   bool groupKeepGoing;

   groupKeepGoing = src->GetFirstGroup(groupName, groupIndex);
   while (groupKeepGoing) {
      wxString subPath = path+groupName+wxT("/");
      src->SetPath(subPath);
      CopyEntriesRecursive(subPath, src, dst);
      src->SetPath(path);
      groupKeepGoing = src->GetNextGroup(groupName, groupIndex);
   }
}
#endif

AudacityPrefs::AudacityPrefs(const wxString& appName,
               const wxString& vendorName,
               const wxString& localFilename,
               const wxString& globalFilename,
               long style,
               const wxMBConv& conv) :
   wxFileConfig(appName,
               vendorName,
               localFilename,
               globalFilename,
               style,
               conv)
{
}



// Bug 825 is essentially that SyncLock requires EditClipsCanMove.
// SyncLock needs rethinking, but meanwhile this function 
// fixes the issues of Bug 825 by allowing clips to move when in 
// SyncLock.
bool AudacityPrefs::GetEditClipsCanMove()
{
   bool mIsSyncLocked;
   gPrefs->Read(wxT("/GUI/SyncLockTracks"), &mIsSyncLocked, false);
   if( mIsSyncLocked )
      return true;
   bool editClipsCanMove;
   Read(wxT("/GUI/EditClipCanMove"), &editClipsCanMove, true);
   return editClipsCanMove;
}

void InitPreferences()
{
   wxString appName = wxTheApp->GetAppName();

   wxFileName configFileName(FileNames::DataDir(), wxT("audacity.cfg"));

   ugPrefs = std::make_unique<AudacityPrefs>
      (appName, wxEmptyString,
       configFileName.GetFullPath(),
       wxEmptyString, wxCONFIG_USE_LOCAL_FILE);
   gPrefs = ugPrefs.get();

   wxConfigBase::Set(gPrefs);
}

void FinishPreferences()
{
   if (gPrefs) {
      wxConfigBase::Set(NULL);
      ugPrefs.reset();
      gPrefs = NULL;
   }
}

//////////
wxString ChoiceSetting::Read() const
{
   const auto &defaultValue = Default().Internal();
   wxString value;
   if ( !gPrefs->Read(mKey, &value, defaultValue) )
      if (!mMigrated) {
         const_cast<ChoiceSetting*>(this)->Migrate( value );
         mMigrated = true;
      }

   // Remap to default if the string is not known -- this avoids surprises
   // in case we try to interpret config files from future versions
   auto index = Find( value );
   if ( index >= mnSymbols )
      value = defaultValue;
   return value;
}

size_t ChoiceSetting::Find( const wxString &value ) const
{
   return size_t(
      std::find( begin(), end(), EnumValueSymbol{ value, {} } )
         - mSymbols );
}

void ChoiceSetting::Migrate( wxString &value )
{
   (void)value;// Compiler food
}

bool ChoiceSetting::Write( const wxString &value )
{
   auto index = Find( value );
   if (index >= mnSymbols)
      return false;

   auto result = gPrefs->Write( mKey, value );
   mMigrated = true;
   return result;
}

int EnumSetting::ReadInt() const
{
   if (!mIntValues)
      return 0;

   auto index = Find( Read() );
   wxASSERT( index < mnSymbols );
   return mIntValues[ index ];
}

size_t EnumSetting::FindInt( int code ) const
{
   if (!mIntValues)
      return mnSymbols;

   return size_t(
      std::find( mIntValues, mIntValues + mnSymbols, code )
         - mIntValues );
}

void EnumSetting::Migrate( wxString &value )
{
   int intValue = 0;
   if ( !mOldKey.empty() &&
        gPrefs->Read(mOldKey, &intValue, 0) ) {
      // Make the migration, only once and persistently.
      // Do not DELETE the old key -- let that be read if user downgrades
      // Audacity.  But further changes will be stored only to the NEW key
      // and won't be seen then.
      auto index = FindInt( intValue );
      if ( index >= mnSymbols )
         index = mDefaultSymbol;
      value = mSymbols[index].Internal();
      Write(value);
      gPrefs->Flush();
   }
}

bool EnumSetting::WriteInt( int code ) // you flush gPrefs afterward
{
   auto index = FindInt( code );
   if ( index >= mnSymbols )
      return false;
   return Write( mSymbols[index].Internal() );
}
