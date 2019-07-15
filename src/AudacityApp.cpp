/**********************************************************************

  Audacity: A Digital Audio Editor

  AudacityApp.cpp

  Dominic Mazzoni

******************************************************************//**

\class AudacityApp
\brief AudacityApp is the 'main' class for Audacity

It handles initialization and termination by subclassing wxApp.

*//*******************************************************************/

#include "Audacity.h" // This should always be included first; for USE_* macros and __UNIX__
#include "AudacityApp.h"

#include "Experimental.h"

#if 0
// This may be used to debug memory leaks.
// See: Visual Leak Dectector @ http://vld.codeplex.com/
#include <vld.h>
#endif

#include <wx/setup.h> // for wxUSE_* macros
#include <wx/wxcrtvararg.h>
#include <wx/defs.h>
#include <wx/app.h>
#include <wx/bitmap.h>
#include <wx/docview.h>
#include <wx/event.h>
#include <wx/ipc.h>
#include <wx/window.h>
#include <wx/intl.h>
#include <wx/menu.h>
#include <wx/snglinst.h>
#include <wx/splash.h>
#include <wx/stdpaths.h>
#include <wx/sysopt.h>
#include <wx/fontmap.h>

#include <wx/fs_zip.h>
#include <wx/image.h>

#include <wx/dir.h>
#include <wx/file.h>
#include <wx/filename.h>

#ifdef __WXGTK__
#include <unistd.h>
#endif

// chmod, lstat, geteuid
#ifdef __UNIX__
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#endif

#if defined(__WXMSW__)
#include <wx/msw/registry.h> // for wxRegKey
#endif

#include "AudacityLogger.h"
#include "AboutDialog.h"
#include "AColor.h"
#include "AudioIO.h"
#include "Benchmark.h"
#include "Clipboard.h"
#include "CrashReport.h"
#include "DirManager.h"
#include "commands/CommandHandler.h"
#include "commands/AppCommandEvent.h"
#include "widgets/ASlider.h"
#include "FFmpeg.h"
//#include "LangChoice.h"
#include "Languages.h"
#include "Menus.h"
#include "MissingAliasFileDialog.h"
#include "PluginManager.h"
#include "Project.h"
#include "ProjectAudioIO.h"
#include "ProjectAudioManager.h"
#include "ProjectFileManager.h"
#include "ProjectHistory.h"
#include "ProjectManager.h"
#include "ProjectSettings.h"
#include "ProjectWindow.h"
#include "Screenshot.h"
#include "Sequence.h"
#include "WaveTrack.h"
#include "prefs/PrefsDialog.h"
#include "Theme.h"
#include "PlatformCompatibility.h"
#include "FileNames.h"
#include "AutoRecovery.h"
#include "AutoRecoveryDialog.h"
#include "SplashDialog.h"
#include "FFT.h"
#include "BlockFile.h"
#include "ondemand/ODManager.h"
#include "widgets/AudacityMessageBox.h"
#include "prefs/DirectoriesPrefs.h"
#include "prefs/GUIPrefs.h"
#include "tracks/ui/Scrubbing.h"
#include "widgets/FileHistory.h"

#ifdef EXPERIMENTAL_EASY_CHANGE_KEY_BINDINGS
#include "prefs/KeyConfigPrefs.h"
#endif

//temporarily commented out till it is added to all projects
//#include "Profiler.h"

#include "ModuleManager.h"

#include "import/Import.h"

#if defined(EXPERIMENTAL_CRASH_REPORT)
#include <wx/debugrpt.h>
#include <wx/evtloop.h>
#include <wx/textdlg.h>
#endif

#ifdef EXPERIMENTAL_SCOREALIGN
#include "effects/ScoreAlignDialog.h"
#endif

#if 0
#ifdef _DEBUG
    #ifdef _MSC_VER
        #undef THIS_FILE
        static char*THIS_FILE= __FILE__;
        #define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
    #endif
#endif
#endif

// Windows specific linker control...only needed once so
// this is a good place (unless we want to add another file).
#if defined(__WXMSW__)
//#if wxCHECK_VERSION(3, 0, 2) && !wxCHECK_VERSION(3, 1, 0)
#include <wx/init.h>
//#endif
// These lines ensure that Audacity gets WindowsXP themes.
// Without them we get the old-style Windows98/2000 look under XP.
#  if !defined(__WXWINCE__)
#     pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#  endif

// These lines allows conditional inclusion of the various libraries
// that Audacity can use.

#  if defined(USE_LIBFLAC)
#     pragma comment(lib, "libflac++")
#     pragma comment(lib, "libflac")
#  endif

#  if defined(USE_LIBID3TAG)
#     pragma comment(lib, "libid3tag")
#  endif

#  if defined(USE_LIBMAD)
#     pragma comment(lib, "libmad")
#  endif

#  if defined(USE_LIBTWOLAME)
#     pragma comment(lib, "twolame")
#  endif

#  if defined(USE_LIBVORBIS)
#     pragma comment(lib, "libogg")
#     pragma comment(lib, "libvorbis")
#  endif

#  if defined(USE_LV2)
#     pragma comment(lib, "lv2")
#  endif

#  if defined(USE_MIDI)
#     pragma comment(lib, "portsmf")
#     endif

#  if defined(EXPERIMENTAL_MIDI_OUT)
#     pragma comment(lib, "portmidi")
#  endif

#  if defined(EXPERIMENTAL_SCOREALIGN)
#     pragma comment(lib, "libscorealign")
#  endif

#  if defined(USE_NYQUIST)
#     pragma comment(lib, "libnyquist")
#  endif

#  if defined(USE_PORTMIXER)
#     pragma comment(lib, "portmixer")
#  endif

#  if defined(USE_SBSMS)
#     pragma comment(lib, "sbsms")
#  endif

#  if defined(USE_SOUNDTOUCH)
#     pragma comment(lib, "soundtouch")
#  endif

#  if defined(USE_VAMP)
#     pragma comment(lib, "libvamp")
#  endif

#if AUDACITY_NEED_WX_LIBS
#  if defined(_DEBUG)
#     define D "d"
#  else
#     define D ""
#  endif
#  if wxCHECK_VERSION(3, 1, 0)
#     define V "31"
#  elif wxCHECK_VERSION(3, 0, 0)
#     define V "30"
#  else
#     define V "28"
#  endif

#  if defined(EXPERIMENTAL_CRASH_REPORT)
#     pragma comment(lib, "wxmsw" V "u" D "_qa")
#  endif
#  pragma comment(lib, "wxbase" V "u" D)
#  pragma comment(lib, "wxbase" V "u" D "_net")
#  pragma comment(lib, "wxmsw"  V "u" D "_core")
#  pragma comment(lib, "wxmsw"  V "u" D "_html")
#  pragma comment(lib, "wxpng"        D)
#  pragma comment(lib, "wxzlib"       D)
#  pragma comment(lib, "wxjpeg"       D)
#  pragma comment(lib, "wxtiff"       D)

#  undef V
#  undef D
#endif // AUDACITY_NO_DLLEXPORT
#endif //(__WXMSW__)

// DA: Logo for Splash Screen
#ifdef EXPERIMENTAL_DA
#include "../images/DarkAudacityLogoWithName.xpm"
#else
#include "../images/AudacityLogoWithName.xpm"
#endif


////////////////////////////////////////////////////////////
/// Custom events
////////////////////////////////////////////////////////////

#if 0
#ifdef __WXGTK__
static void wxOnAssert(const wxChar *fileName, int lineNumber, const wxChar *msg)
{
   if (msg)
      wxPrintf("ASSERTION FAILED: %s\n%s: %d\n", (const char *)wxString(msg).mb_str(), (const char *)wxString(fileName).mb_str(), lineNumber);
   else
      wxPrintf("ASSERTION FAILED!\n%s: %d\n", (const char *)wxString(fileName).mb_str(), lineNumber);

   // Force core dump
   int *i = 0;
   if (*i)
      exit(1);

   exit(0);
}
#endif
#endif

namespace {

void PopulatePreferences()
{
   bool resetPrefs = false;
   wxString langCode = gPrefs->Read(wxT("/Locale/Language"), wxEmptyString);
   bool writeLang = false;

   const wxFileName fn(
      FileNames::ResourcesDir(), 
      wxT("FirstTime.ini"));
   if (fn.FileExists())   // it will exist if the (win) installer put it there
   {
      const wxString fullPath{fn.GetFullPath()};

      wxFileConfig ini(wxEmptyString,
                       wxEmptyString,
                       fullPath,
                       wxEmptyString,
                       wxCONFIG_USE_LOCAL_FILE);

      wxString lang;
      if (ini.Read(wxT("/FromInno/Language"), &lang))
      {
         // Only change "langCode" if the language was actually specified in the ini file.
         langCode = lang;
         writeLang = true;

         // Inno Setup doesn't allow special characters in the Name values, so "0" is used
         // to represent the "@" character.
         langCode.Replace(wxT("0"), wxT("@"));
      }

      ini.Read(wxT("/FromInno/ResetPrefs"), &resetPrefs, false);

      bool gone = wxRemoveFile(fullPath);  // remove FirstTime.ini
      if (!gone)
      {
         AudacityMessageBox(wxString::Format(_("Failed to remove %s"), fullPath), _("Failed!"));
      }
   }

   langCode = GUIPrefs::InitLang( langCode );

   // User requested that the preferences be completely reset
   if (resetPrefs)
   {
      // pop up a dialogue
      wxString prompt = _("Reset Preferences?\n\nThis is a one-time question, after an 'install' where you asked to have the Preferences reset.");
      int action = AudacityMessageBox(prompt, _("Reset Audacity Preferences"),
                                wxYES_NO, NULL);
      if (action == wxYES)   // reset
      {
         gPrefs->DeleteAll();
         writeLang = true;
      }
   }

   // Save the specified language
   if (writeLang)
   {
      gPrefs->Write(wxT("/Locale/Language"), langCode);
   }

   // In AUdacity 2.1.0 support for the legacy 1.2.x preferences (depreciated since Audacity
   // 1.3.1) is dropped. As a result we can drop the import flag
   // first time this version of Audacity is run we try to migrate
   // old preferences.
   bool newPrefsInitialized = false;
   gPrefs->Read(wxT("/NewPrefsInitialized"), &newPrefsInitialized, false);
   if (newPrefsInitialized) {
      gPrefs->DeleteEntry(wxT("/NewPrefsInitialized"), true);  // take group as well if empty
   }

   // record the Prefs version for future checking (this has not been used for a very
   // long time).
   gPrefs->Write(wxT("/PrefsVersion"), wxString(wxT(AUDACITY_PREFS_VERSION_STRING)));

   // Check if some prefs updates need to happen based on audacity version.
   // Unfortunately we can't use the PrefsVersion prefs key because that resets things.
   // In the future we may want to integrate that better.
   // these are done on a case-by-case basis for now so they must be backwards compatible
   // (meaning the changes won't mess audacity up if the user goes back to an earlier version)
   int vMajor = gPrefs->Read(wxT("/Version/Major"), (long) 0);
   int vMinor = gPrefs->Read(wxT("/Version/Minor"), (long) 0);
   int vMicro = gPrefs->Read(wxT("/Version/Micro"), (long) 0);

   gPrefs->SetVersionKeysInit(vMajor, vMinor, vMicro);   // make a note of these initial values
                                                            // for use by ToolManager::ReadConfig()

   // These integer version keys were introduced april 4 2011 for 1.3.13
   // The device toolbar needs to be enabled due to removal of source selection features in
   // the mixer toolbar.
   if ((vMajor < 1) ||
       (vMajor == 1 && vMinor < 3) ||
       (vMajor == 1 && vMinor == 3 && vMicro < 13)) {


      // Do a full reset of the Device Toolbar to get it on the screen.
      if (gPrefs->Exists(wxT("/GUI/ToolBars/Device")))
         gPrefs->DeleteGroup(wxT("/GUI/ToolBars/Device"));

      // We keep the mixer toolbar prefs (shown/not shown)
      // the width of the mixer toolbar may have shrunk, the prefs will keep the larger value
      // if the user had a device that had more than one source.
      if (gPrefs->Exists(wxT("/GUI/ToolBars/Mixer"))) {
         // Use the default width
         gPrefs->Write(wxT("/GUI/ToolBars/Mixer/W"), -1);
      }
   }

   // In 2.1.0, the Meter toolbar was split and lengthened, but strange arrangements happen
   // if upgrading due to the extra length.  So, if a user is upgrading, use the pre-2.1.0
   // lengths, but still use the NEW split versions.
   if (gPrefs->Exists(wxT("/GUI/ToolBars/Meter")) &&
      !gPrefs->Exists(wxT("/GUI/ToolBars/CombinedMeter"))) {

      // Read in all of the existing values
      long dock, order, show, x, y, w, h;
      gPrefs->Read(wxT("/GUI/ToolBars/Meter/Dock"), &dock, -1);
      gPrefs->Read(wxT("/GUI/ToolBars/Meter/Order"), &order, -1);
      gPrefs->Read(wxT("/GUI/ToolBars/Meter/Show"), &show, -1);
      gPrefs->Read(wxT("/GUI/ToolBars/Meter/X"), &x, -1);
      gPrefs->Read(wxT("/GUI/ToolBars/Meter/Y"), &y, -1);
      gPrefs->Read(wxT("/GUI/ToolBars/Meter/W"), &w, -1);
      gPrefs->Read(wxT("/GUI/ToolBars/Meter/H"), &h, -1);

      // "Order" must be adjusted since we're inserting two NEW toolbars
      if (dock > 0) {
         wxString oldPath = gPrefs->GetPath();
         gPrefs->SetPath(wxT("/GUI/ToolBars"));

         wxString bar;
         long ndx = 0;
         bool cont = gPrefs->GetFirstGroup(bar, ndx);
         while (cont) {
            long o;
            if (gPrefs->Read(bar + wxT("/Order"), &o) && o >= order) {
               gPrefs->Write(bar + wxT("/Order"), o + 2);
            }
            cont = gPrefs->GetNextGroup(bar, ndx);
         }
         gPrefs->SetPath(oldPath);

         // And override the height
         h = 27;
      }

      // Write the split meter bar values
      gPrefs->Write(wxT("/GUI/ToolBars/RecordMeter/Dock"), dock);
      gPrefs->Write(wxT("/GUI/ToolBars/RecordMeter/Order"), order);
      gPrefs->Write(wxT("/GUI/ToolBars/RecordMeter/Show"), show);
      gPrefs->Write(wxT("/GUI/ToolBars/RecordMeter/X"), -1);
      gPrefs->Write(wxT("/GUI/ToolBars/RecordMeter/Y"), -1);
      gPrefs->Write(wxT("/GUI/ToolBars/RecordMeter/W"), w);
      gPrefs->Write(wxT("/GUI/ToolBars/RecordMeter/H"), h);
      gPrefs->Write(wxT("/GUI/ToolBars/PlayMeter/Dock"), dock);
      gPrefs->Write(wxT("/GUI/ToolBars/PlayMeter/Order"), order + 1);
      gPrefs->Write(wxT("/GUI/ToolBars/PlayMeter/Show"), show);
      gPrefs->Write(wxT("/GUI/ToolBars/PlayMeter/X"), -1);
      gPrefs->Write(wxT("/GUI/ToolBars/PlayMeter/Y"), -1);
      gPrefs->Write(wxT("/GUI/ToolBars/PlayMeter/W"), w);
      gPrefs->Write(wxT("/GUI/ToolBars/PlayMeter/H"), h);

      // And hide the old combined meter bar
      gPrefs->Write(wxT("/GUI/ToolBars/Meter/Dock"), -1);
   }

   // Upgrading pre 2.2.0 configs we assume extended set of defaults.
   if ((0<vMajor && vMajor < 2) ||
       (vMajor == 2 && vMinor < 2))
   {
      gPrefs->Write(wxT("/GUI/Shortcuts/FullDefaults"),1);
   }

   // write out the version numbers to the prefs file for future checking
   gPrefs->Write(wxT("/Version/Major"), AUDACITY_VERSION);
   gPrefs->Write(wxT("/Version/Minor"), AUDACITY_RELEASE);
   gPrefs->Write(wxT("/Version/Micro"), AUDACITY_REVISION);

   gPrefs->Flush();
}

}

static bool gInited = false;
static bool gIsQuitting = false;

static void QuitAudacity(bool bForce)
{
   // guard against recursion
   if (gIsQuitting)
      return;

   gIsQuitting = true;

   wxTheApp->SetExitOnFrameDelete(true);

   // Try to close each open window.  If the user hits Cancel
   // in a Save Changes dialog, don't continue.
   // BG: unless force is true

   // BG: Are there any projects open?
   //-   if (!AllProjects{}.empty())
/*start+*/
   if (AllProjects{}.empty())
   {
#ifdef __WXMAC__
      Clipboard::Get().Clear();
#endif
   }
   else
/*end+*/
   {
      if (AllProjects{}.size())
         // PRL:  Always did at least once before close might be vetoed
         // though I don't know why that is important
         ProjectManager::SaveWindowSize();
      bool closedAll = AllProjects::Close( bForce );
      if ( !closedAll )
      {
         gIsQuitting = false;
         return;
      }
   }

   ModuleManager::Get().Dispatch(AppQuiting);

#ifdef EXPERIMENTAL_SCOREALIGN
   CloseScoreAlignDialog();
#endif
   CloseScreenshotTools();

   //release ODManager Threads
   ODManager::Quit();

   //print out profile if we have one by deleting it
   //temporarily commented out till it is added to all projects
   //DELETE Profiler::Instance();

   //remove our logger
   std::unique_ptr<wxLog>{ wxLog::SetActiveTarget(NULL) }; // DELETE

   if (bForce)
   {
      wxExit();
   }
}

static void QuitAudacity()
{
   QuitAudacity(false);
}

#if defined(__WXGTK__) && defined(HAVE_GTK)

///////////////////////////////////////////////////////////////////////////////
// Provide the ability to receive notification from the session manager
// when the user is logging out or shutting down.
//
// Most of this was taken from nsNativeAppSupportUnix.cpp from Mozilla.
///////////////////////////////////////////////////////////////////////////////

// TODO: May need updating.  Is this code too obsolete (relying on Gnome2 so's) to be
// worth keeping anymore?
// CB suggests we use libSM directly ref:
// http://www.x.org/archive/X11R7.7/doc/libSM/SMlib.html#The_Save_Yourself_Callback

#include <dlfcn.h>
/* There is a conflict between the type names used in Glib >= 2.21 and those in
 * wxGTK (http://trac.wxwidgets.org/ticket/10883)
 * Happily we can avoid the hack, as we only need some of the headers, not
 * the full GTK headers
 */
#include <glib-object.h>

typedef struct _GnomeProgram GnomeProgram;
typedef struct _GnomeModuleInfo GnomeModuleInfo;
typedef struct _GnomeClient GnomeClient;

typedef enum
{
  GNOME_SAVE_GLOBAL,
  GNOME_SAVE_LOCAL,
  GNOME_SAVE_BOTH
} GnomeSaveStyle;

typedef enum
{
  GNOME_INTERACT_NONE,
  GNOME_INTERACT_ERRORS,
  GNOME_INTERACT_ANY
} GnomeInteractStyle;

typedef enum
{
  GNOME_DIALOG_ERROR,
  GNOME_DIALOG_NORMAL
} GnomeDialogType;

typedef GnomeProgram * (*_gnome_program_init_fn)(const char *,
                                                 const char *,
                                                 const GnomeModuleInfo *,
                                                 int,
                                                 char **,
                                                 const char *,
                                                 ...);
typedef const GnomeModuleInfo * (*_libgnomeui_module_info_get_fn)();
typedef GnomeClient * (*_gnome_master_client_fn)(void);
typedef void (*GnomeInteractFunction)(GnomeClient *,
                                      gint,
                                      GnomeDialogType,
                                      gpointer);
typedef void (*_gnome_client_request_interaction_fn)(GnomeClient *,
                                                     GnomeDialogType,
                                                     GnomeInteractFunction,
                                                     gpointer);
typedef void (*_gnome_interaction_key_return_fn)(gint, gboolean);

static _gnome_client_request_interaction_fn gnome_client_request_interaction;
static _gnome_interaction_key_return_fn gnome_interaction_key_return;

static void interact_cb(GnomeClient * /* client */,
                        gint key,
                        GnomeDialogType /* type */,
                        gpointer /* data */)
{
   wxCloseEvent e(wxEVT_QUERY_END_SESSION, wxID_ANY);
   e.SetEventObject(&wxGetApp());
   e.SetCanVeto(true);

   wxGetApp().ProcessEvent(e);

   gnome_interaction_key_return(key, e.GetVeto());
}

static gboolean save_yourself_cb(GnomeClient *client,
                                 gint /* phase */,
                                 GnomeSaveStyle /* style */,
                                 gboolean shutdown,
                                 GnomeInteractStyle interact,
                                 gboolean /* fast */,
                                 gpointer /* user_data */)
{
   if (!shutdown || interact != GNOME_INTERACT_ANY) {
      return TRUE;
   }

   if (AllProjects{}.empty()) {
      return TRUE;
   }

   gnome_client_request_interaction(client,
                                    GNOME_DIALOG_NORMAL,
                                    interact_cb,
                                    NULL);

   return TRUE;
}

class GnomeShutdown
{
 public:
   GnomeShutdown()
   {
      mArgv[0].reset(strdup("Audacity"));

      mGnomeui = dlopen("libgnomeui-2.so.0", RTLD_NOW);
      if (!mGnomeui) {
         return;
      }

      mGnome = dlopen("libgnome-2.so.0", RTLD_NOW);
      if (!mGnome) {
         return;
      }

      _gnome_program_init_fn gnome_program_init = (_gnome_program_init_fn)
         dlsym(mGnome, "gnome_program_init");
      _libgnomeui_module_info_get_fn libgnomeui_module_info_get = (_libgnomeui_module_info_get_fn)
         dlsym(mGnomeui, "libgnomeui_module_info_get");
      _gnome_master_client_fn gnome_master_client = (_gnome_master_client_fn)
         dlsym(mGnomeui, "gnome_master_client");

      gnome_client_request_interaction = (_gnome_client_request_interaction_fn)
         dlsym(mGnomeui, "gnome_client_request_interaction");
      gnome_interaction_key_return = (_gnome_interaction_key_return_fn)
         dlsym(mGnomeui, "gnome_interaction_key_return");


      if (!gnome_program_init || !libgnomeui_module_info_get) {
         return;
      }

      gnome_program_init(mArgv[0].get(),
                         "1.0",
                         libgnomeui_module_info_get(),
                         1,
                         reinterpret_cast<char**>(mArgv),
                         NULL);

      mClient = gnome_master_client();
      if (mClient == NULL) {
         return;
      }

      g_signal_connect(mClient, "save-yourself", G_CALLBACK(save_yourself_cb), NULL);
   }

   virtual ~GnomeShutdown()
   {
      // Do not dlclose() the libraries here lest you want segfaults...
   }

 private:

   MallocString<> mArgv[1];
   void *mGnomeui;
   void *mGnome;
   GnomeClient *mClient;
};

// This variable exists to call the constructor and
// connect a signal for the 'save-yourself' message.
GnomeShutdown GnomeShutdownInstance;

#endif

// Where drag/drop or "Open With" filenames get stored until
// the timer routine gets around to picking them up.
static wxArrayString ofqueue;

//
// DDE support for opening multiple files with one instance
// of Audacity.
//

#define IPC_APPL wxT("audacity")
#define IPC_TOPIC wxT("System")

class IPCConn final : public wxConnection
{
public:
   IPCConn()
   : wxConnection()
   {
   };

   ~IPCConn()
   {
   };

   bool OnExec(const wxString & WXUNUSED(topic),
               const wxString & data)
   {
      // Add the filename to the queue.  It will be opened by
      // the OnTimer() event when it is safe to do so.
      ofqueue.push_back(data);

      return true;
   }
};

class IPCServ final : public wxServer
{
public:
   IPCServ(const wxString & appl)
   : wxServer()
   {
      Create(appl);
   };

   ~IPCServ()
   {
   };

   wxConnectionBase *OnAcceptConnection(const wxString & topic) override
   {
      if (topic != IPC_TOPIC) {
         return NULL;
      }

      // Trust wxWidgets framework to DELETE it
      return safenew IPCConn();
   };
};

#if defined(__WXMAC__)

IMPLEMENT_APP_NO_MAIN(AudacityApp)
IMPLEMENT_WX_THEME_SUPPORT

int main(int argc, char *argv[])
{
   wxDISABLE_DEBUG_SUPPORT();

   return wxEntry(argc, argv);
}

#elif defined(__WXMSW__) && !wxCHECK_VERSION(3, 1, 0)
// Disable telling Windows that we support HiDPI displays.  It is forced on
// in wxWidget versions between 3.0.0 and 3.1.0.
IMPLEMENT_APP_NO_MAIN(AudacityApp)
IMPLEMENT_WX_THEME_SUPPORT

extern "C" int WINAPI WinMain(HINSTANCE hInstance,
                              HINSTANCE hPrevInstance,
                              wxCmdLineArgType WXUNUSED(lpCmdLine),
                              int nCmdShow)
{
   wxDISABLE_DEBUG_SUPPORT();

   // Disable setting of HiDPI aware mode
   wxMSWDisableSettingHighDPIAware();

   /* NB: We pass NULL in place of lpCmdLine to behave the same as  */
   /*     Borland-specific wWinMain() above. If it becomes needed   */
   /*     to pass lpCmdLine to wxEntry() here, you'll have to fix   */
   /*     wWinMain() above too.                                     */
   return wxEntry(hInstance, hPrevInstance, NULL, nCmdShow);
}

#else
IMPLEMENT_APP(AudacityApp)
#endif

#ifdef __WXMAC__

// in response of an open-document apple event
void AudacityApp::MacOpenFile(const wxString &fileName)
{
   ofqueue.push_back(fileName);
}

// in response of a print-document apple event
void AudacityApp::MacPrintFile(const wxString &fileName)
{
   ofqueue.push_back(fileName);
}

// in response of a open-application apple event
void AudacityApp::MacNewFile()
{
   if (!gInited)
      return;

   // This method should only be used on the Mac platform
   // when no project windows are open.

   if (AllProjects{}.empty())
      (void) ProjectManager::New();
}

#endif //__WXMAC__

// IPC communication
#define ID_IPC_SERVER   6200
#define ID_IPC_SOCKET   6201

// we don't really care about the timer id, but set this value just in case we do in the future
#define kAudacityAppTimerID 0

BEGIN_EVENT_TABLE(AudacityApp, wxApp)
   EVT_QUERY_END_SESSION(AudacityApp::OnQueryEndSession)
   EVT_END_SESSION(AudacityApp::OnEndSession)

   EVT_TIMER(kAudacityAppTimerID, AudacityApp::OnTimer)
#ifdef __WXMAC__
   EVT_MENU(wxID_NEW, AudacityApp::OnMenuNew)
   EVT_MENU(wxID_OPEN, AudacityApp::OnMenuOpen)
   EVT_MENU(wxID_ABOUT, AudacityApp::OnMenuAbout)
   EVT_MENU(wxID_PREFERENCES, AudacityApp::OnMenuPreferences)
#endif

   // Associate the handler with the menu id on all operating systems, even
   // if they don't have an application menu bar like in macOS, so that
   // other parts of the program can send the application a shut-down
   // event
   EVT_MENU(wxID_EXIT, AudacityApp::OnMenuExit)

#ifndef __WXMSW__
   EVT_SOCKET(ID_IPC_SERVER, AudacityApp::OnServerEvent)
   EVT_SOCKET(ID_IPC_SOCKET, AudacityApp::OnSocketEvent)
#endif

   // Recent file event handlers.
   EVT_MENU(FileHistory::ID_RECENT_CLEAR, AudacityApp::OnMRUClear)
   EVT_MENU_RANGE(FileHistory::ID_RECENT_FIRST, FileHistory::ID_RECENT_LAST,
      AudacityApp::OnMRUFile)

   // Handle AppCommandEvents (usually from a script)
   EVT_APP_COMMAND(wxID_ANY, AudacityApp::OnReceiveCommand)

   // Global ESC key handling
   EVT_KEY_DOWN(AudacityApp::OnKeyDown)
END_EVENT_TABLE()

// backend for OnMRUFile
// TODO: Would be nice to make this handle not opening a file with more panache.
//  - Inform the user if DefaultOpenPath not set.
//  - Switch focus to correct instance of project window, if already open.
bool AudacityApp::MRUOpen(const FilePath &fullPathStr) {
   // Most of the checks below are copied from ProjectManager::OpenFiles.
   // - some rationalisation might be possible.

   AudacityProject *proj = GetActiveProject();

   if (!fullPathStr.empty())
   {
      // verify that the file exists
      if (wxFile::Exists(fullPathStr))
      {
         FileNames::UpdateDefaultPath(FileNames::Operation::Open, fullPathStr);

         // Make sure it isn't already open.
         // Test here even though AudacityProject::OpenFile() also now checks, because
         // that method does not return the bad result.
         // That itself may be a FIXME.
         if (ProjectFileManager::IsAlreadyOpen(fullPathStr))
            return false;

         // DMM: If the project is dirty, that means it's been touched at
         // all, and it's not safe to open a NEW project directly in its
         // place.  Only if the project is brand-NEW clean and the user
         // hasn't done any action at all is it safe for Open to take place
         // inside the current project.
         //
         // If you try to Open a NEW project inside the current window when
         // there are no tracks, but there's an Undo history, etc, then
         // bad things can happen, including data files moving to the NEW
         // project directory, etc.
         if (proj && (
            ProjectHistory::Get( *proj ).GetDirty() ||
            !TrackList::Get( *proj ).empty()
         ) )
            proj = nullptr;
         // This project is clean; it's never been touched.  Therefore
         // all relevant member variables are in their initial state,
         // and it's okay to open a NEW project inside this window.
         ( void ) ProjectManager::OpenProject( proj, fullPathStr );
      }
      else {
         // File doesn't exist - remove file from history
         AudacityMessageBox(wxString::Format(_("%s could not be found.\n\nIt has been removed from the list of recent files."),
                      fullPathStr));
         return(false);
      }
   }
   return(true);
}

bool AudacityApp::SafeMRUOpen(const wxString &fullPathStr)
{
   return GuardedCall< bool >( [&]{ return MRUOpen( fullPathStr ); } );
}

void AudacityApp::OnMRUClear(wxCommandEvent& WXUNUSED(event))
{
   FileHistory::Global().Clear();
}

//vvv Basically, anything from Recent Files is treated as a .aup, until proven otherwise,
// then it tries to Import(). Very questionable handling, imo.
// Better, for example, to check the file type early on.
void AudacityApp::OnMRUFile(wxCommandEvent& event) {
   int n = event.GetId() - FileHistory::ID_RECENT_FIRST;
   auto &history = FileHistory::Global();
   const auto &fullPathStr = history.GetHistoryFile(n);

   // Try to open only if not already open.
   // Test IsAlreadyOpen() here even though AudacityProject::MRUOpen() also now checks,
   // because we don't want to RemoveFileFromHistory() just because it already exists,
   // and AudacityApp::OnMacOpenFile() calls MRUOpen() directly.
   // that method does not return the bad result.
   // PRL: Don't call SafeMRUOpen
   // -- if open fails for some exceptional reason of resource exhaustion that
   // the user can correct, leave the file in history.
   if (!ProjectFileManager::IsAlreadyOpen(fullPathStr) && !MRUOpen(fullPathStr))
      history.RemoveFileFromHistory(n);
}

void AudacityApp::OnTimer(wxTimerEvent& WXUNUSED(event))
{
   // Filenames are queued when Audacity receives a few of the
   // AppleEvent messages (via wxWidgets).  So, open any that are
   // in the queue and clean the queue.
   if (gInited) {
      if (ofqueue.size()) {
         // Load each file on the queue
         while (ofqueue.size()) {
            wxString name;
            name.swap(ofqueue[0]);
            ofqueue.erase( ofqueue.begin() );

            // Get the user's attention if no file name was specified
            if (name.empty()) {
               // Get the users attention
               AudacityProject *project = GetActiveProject();
               if (project) {
                  auto &window = GetProjectFrame( *project );
                  window.Maximize();
                  window.Raise();
                  window.RequestUserAttention();
               }
               continue;
            }

            // TODO: Handle failures better.
            // Some failures are OK, e.g. file not found, just would-be-nices to do better,
            // so FAIL_MSG is more a case of an enhancement request than an actual  problem.
            // LL:  In all but one case an appropriate message is already displayed.  The
            //      instance that a message is NOT displayed is when a failure to write
            //      to the config file has occurred.
            // PRL: Catch any exceptions, don't try this file again, continue to
            // other files.
            if (!SafeMRUOpen(name)) {
               wxFAIL_MSG(wxT("MRUOpen failed"));
            }
         }
      }
   }

   // Check if a warning for missing aliased files should be displayed
   if (MissingAliasFilesDialog::ShouldShow()) {
      // find which project owns the blockfile
      // note: there may be more than 1, but just go with the first one.
      //size_t numProjects = AllProjects{}.size();
      auto marked = MissingAliasFilesDialog::Marked();
      auto offendingProject = marked.second;
      wxString missingFileName = marked.first;

      // if there are no projects open, don't show the warning (user has closed it)
      if (offendingProject) {
         auto &window = GetProjectFrame( *offendingProject );
         window.Iconize(false);
         window.Raise();

         wxString errorMessage = wxString::Format(_(
"One or more external audio files could not be found.\n\
It is possible they were moved, deleted, or the drive they \
were on was unmounted.\n\
Silence is being substituted for the affected audio.\n\
The first detected missing file is:\n\
%s\n\
There may be additional missing files.\n\
Choose Help > Diagnostics > Check Dependencies to view a list of \
locations of the missing files."), missingFileName);

         // if an old dialog exists, raise it if it is
         if ( auto dialog = MissingAliasFilesDialog::Find( *offendingProject ) )
            dialog->Raise();
         else {
            MissingAliasFilesDialog::Show(offendingProject.get(), _("Files Missing"),
                                   errorMessage, wxT(""), true);
         }
      }
      // Only show this warning once per event (playback/menu item/etc).
      MissingAliasFilesDialog::SetShouldShow(false);
   }
}

#if defined(__WXMSW__)
#define WL(lang, sublang) (lang), (sublang),
#else
#define WL(lang,sublang)
#endif

#if wxCHECK_VERSION(3, 0, 1)
wxLanguageInfo userLangs[] =
{
   // Bosnian is defined in wxWidgets already
//   { wxLANGUAGE_USER_DEFINED, wxT("bs"), WL(0, SUBLANG_DEFAULT) wxT("Bosnian"), wxLayout_LeftToRight },

   { wxLANGUAGE_USER_DEFINED, wxT("eu"), WL(0, SUBLANG_DEFAULT) wxT("Basque"), wxLayout_LeftToRight },
};
#endif

void AudacityApp::OnFatalException()
{
#if defined(EXPERIMENTAL_CRASH_REPORT)
   CrashReport::Generate(wxDebugReport::Context_Exception);
#endif

   exit(-1);
}


#ifdef _MSC_VER
// If this is compiled with MSVC (Visual Studio)
#pragma warning( push )
#pragma warning( disable : 4702) // unreachable code warning.
#endif //_MSC_VER

bool AudacityApp::OnExceptionInMainLoop()
{
   // This function is invoked from catch blocks in the wxWidgets framework,
   // and throw; without argument re-throws the exception being handled,
   // letting us dispatch according to its type.

   try { throw; }
   catch ( AudacityException &e ) {
      (void)e;// Compiler food
      // Here is the catch-all for our own exceptions

      // Use CallAfter to delay this to the next pass of the event loop,
      // rather than risk doing it inside stack unwinding.
      auto pProject = ::GetActiveProject();
      auto pException = std::current_exception();
      CallAfter( [=]      // Capture pException by value!
      {

         // Restore the state of the project to what it was before the
         // failed operation
         if (pProject) {
            ProjectHistory::Get( *pProject ).RollbackState();

            // Forget pending changes in the TrackList
            TrackList::Get( *pProject ).ClearPendingTracks();

            ProjectWindow::Get( *pProject ).RedrawProject();
         }

         // Give the user an alert
         try { std::rethrow_exception( pException ); }
         catch( AudacityException &e )
            { e.DelayedHandlerAction(); }

      } );

      // Don't quit the program
      return true;
   }
   catch ( ... ) {
      // There was some other type of exception we don't know.
      // Let the inherited function do throw; again and whatever else it does.
      return wxApp::OnExceptionInMainLoop();
   }
   // Shouldn't ever reach this line
   return false;
}
#ifdef _MSC_VER
#pragma warning( pop )
#endif //_MSC_VER

int AudacityApp::FilterEvent(wxEvent & event)
{
   (void)event;// compiler food (stops unused parameter warning)
#if !wxCHECK_VERSION(3, 0, 0) && defined(__WXGTK__)
   // On wxGTK, there's a focus issue where dialogs do not automatically pass focus
   // to the first child.  This means that you can use the keyboard to navigate within
   // the dialog.  Watching for the ACTIVATE event allows us to set the focus ourselves
   // when each dialog opens.
   //
   // See bug #57
   //
   if (event.GetEventType() == wxEVT_ACTIVATE)
   {
      wxActivateEvent & e = (wxActivateEvent &) event;

      if (e.GetEventObject() && e.GetActive() && e.GetEventObject()->IsKindOf(CLASSINFO(wxDialog)))
      {
         ((wxWindow *)e.GetEventObject())->SetFocus();
      }
   }
#endif


#if defined(__WXMAC__) || defined(__WXGTK__)
   if (event.GetEventType() == wxEVT_ACTIVATE)
   {
      wxActivateEvent & e = static_cast<wxActivateEvent &>(event);

      const auto object = e.GetEventObject();
      if (object && e.GetActive() &&
          object->IsKindOf(CLASSINFO(wxWindow)))
      {
         const auto window = ((wxWindow *)e.GetEventObject());
         window->SetFocus();
#if defined(__WXMAC__)
         window->NavigateIn();
#endif
      }
   }
#endif

   return Event_Skip;
}

AudacityApp::AudacityApp()
{
// Do not capture crashes in debug builds
#if !defined(__WXDEBUG__)
#if defined(EXPERIMENTAL_CRASH_REPORT)
#if defined(wxUSE_ON_FATAL_EXCEPTION) && wxUSE_ON_FATAL_EXCEPTION
   wxHandleFatalExceptions();
#endif
#endif
#endif
}

AudacityApp::~AudacityApp()
{
}

// The `main program' equivalent, creating the windows and returning the
// main frame
bool AudacityApp::OnInit()
{
   // JKC: ANSWER-ME: Who actually added the event loop guarantor?
   // Although 'blame' says Leland, I think it came from a donated patch.

   // PRL:  It was added by LL at 54676a72285ba7ee3a69920e91fa390a71ef10c9 :
   // "   Ensure OnInit() has an event loop
   //     And allow events to flow so the splash window updates under GTK"
   // then mistakenly lost in the merge at
   // 37168ebbf67ae869ab71a3b5cbbf1d2a48e824aa
   // then restored at 7687972aa4b2199f0717165235f3ef68ade71e08

   // Ensure we have an event loop during initialization
   wxEventLoopGuarantor eventLoop;

   // cause initialization of wxWidgets' global logger target
   (void) AudacityLogger::Get();

#if defined(__WXMAC__)
   // Disable window animation
   wxSystemOptions::SetOption(wxMAC_WINDOW_PLAIN_TRANSITION, 1);
#endif

   // Don't use AUDACITY_NAME here.
   // We want Audacity with a capital 'A'

// DA: App name
#ifndef EXPERIMENTAL_DA
   wxString appName = wxT("Audacity");
#else
   wxString appName = wxT("DarkAudacity");
#endif

   wxTheApp->SetAppName(appName);
   // Explicitly set since OSX will use it for the "Quit" menu item
   wxTheApp->SetAppDisplayName(appName);
   wxTheApp->SetVendorName(appName);

   ::wxInitAllImageHandlers();

   // AddHandler takes ownership
   wxFileSystem::AddHandler(safenew wxZipFSHandler);

   //
   // Paths: set search path and temp dir path
   //
   FilePaths audacityPathList;

#ifdef __WXGTK__
   /* Search path (for plug-ins, translations etc) is (in this order):
      * The AUDACITY_PATH environment variable
      * The current directory
      * The user's .audacity-files directory in their home directory
      * The "share" and "share/doc" directories in their install path */
   wxString home = wxGetHomeDir();

   wxString envTempDir = wxGetenv(wxT("TMPDIR"));
   if (!envTempDir.empty()) {
      /* On Unix systems, the environment variable TMPDIR may point to
         an unusual path when /tmp and /var/tmp are not desirable. */
      FileNames::SetDefaultTempDir( wxString::Format(
         wxT("%s/audacity-%s"), envTempDir, wxGetUserId() ) );
   } else {
      /* On Unix systems, the default temp dir is in /var/tmp. */
      FileNames::SetDefaultTempDir( wxString::Format(
         wxT("/var/tmp/audacity-%s"), wxGetUserId() ) );
   }

// DA: Path env variable.
#ifndef EXPERIMENTAL_DA
   wxString pathVar = wxGetenv(wxT("AUDACITY_PATH"));
#else
   wxString pathVar = wxGetenv(wxT("DARKAUDACITY_PATH"));
#endif
   if (!pathVar.empty())
      FileNames::AddMultiPathsToPathList(pathVar, audacityPathList);
   FileNames::AddUniquePathToPathList(::wxGetCwd(), audacityPathList);

#ifdef AUDACITY_NAME
   FileNames::AddUniquePathToPathList(wxString::Format(wxT("%s/.%s-files"),
      home, wxT(AUDACITY_NAME)),
      audacityPathList);
   FileNames::AddUniquePathToPathList(wxString::Format(wxT("%s/share/%s"),
      wxT(INSTALL_PREFIX), wxT(AUDACITY_NAME)),
      audacityPathList);
   FileNames::AddUniquePathToPathList(wxString::Format(wxT("%s/share/doc/%s"),
      wxT(INSTALL_PREFIX), wxT(AUDACITY_NAME)),
      audacityPathList);
#else //AUDACITY_NAME
   FileNames::AddUniquePathToPathList(wxString::Format(wxT("%s/.audacity-files"),
      home),
      audacityPathList);
   FileNames::AddUniquePathToPathList(wxString::Format(wxT("%s/share/audacity"),
      wxT(INSTALL_PREFIX)),
      audacityPathList);
   FileNames::AddUniquePathToPathList(wxString::Format(wxT("%s/share/doc/audacity"),
      wxT(INSTALL_PREFIX)),
      audacityPathList);
#endif //AUDACITY_NAME

   FileNames::AddUniquePathToPathList(wxString::Format(wxT("%s/share/locale"),
      wxT(INSTALL_PREFIX)),
      audacityPathList);

   FileNames::AddUniquePathToPathList(wxString::Format(wxT("./locale")),
      audacityPathList);

#endif //__WXGTK__

// JKC Bug 1220: Use path based on home directory on WXMAC
#ifdef __WXMAC__
   wxFileName tmpFile;
   tmpFile.AssignHomeDir();
   wxString tmpDirLoc = tmpFile.GetPath(wxPATH_GET_VOLUME);
#else
   wxFileName tmpFile;
   tmpFile.AssignTempFileName(wxT("nn"));
   wxString tmpDirLoc = tmpFile.GetPath(wxPATH_GET_VOLUME);
   ::wxRemoveFile(tmpFile.GetFullPath());
#endif



   // On Mac and Windows systems, use the directory which contains Audacity.
#ifdef __WXMSW__
   // On Windows, the path to the Audacity program is in argv[0]
   wxString progPath = wxPathOnly(argv[0]);
   FileNames::AddUniquePathToPathList(progPath, audacityPathList);
   FileNames::AddUniquePathToPathList(progPath + wxT("\\Languages"), audacityPathList);

   // See bug #1271 for explanation of location
   tmpDirLoc = FileNames::MkDir(wxStandardPaths::Get().GetUserLocalDataDir());
   FileNames::SetDefaultTempDir( wxString::Format(
      wxT("%s\\SessionData"), tmpDirLoc ) );
#endif //__WXWSW__

#ifdef __WXMAC__
   // On Mac OS X, the path to the Audacity program is in argv[0]
   wxString progPath = wxPathOnly(argv[0]);

   FileNames::AddUniquePathToPathList(progPath, audacityPathList);
   // If Audacity is a "bundle" package, then the root directory is
   // the great-great-grandparent of the directory containing the executable.
   //FileNames::AddUniquePathToPathList(progPath + wxT("/../../../"), audacityPathList);

   // These allow for searching the "bundle"
   FileNames::AddUniquePathToPathList(
      progPath + wxT("/../"), audacityPathList);
   FileNames::AddUniquePathToPathList(
      progPath + wxT("/../Resources"), audacityPathList);

   // JKC Bug 1220: Using an actual temp directory for session data on Mac was
   // wrong because it would get cleared out on a reboot.
   FileNames::SetDefaultTempDir( wxString::Format(
      wxT("%s/Library/Application Support/audacity/SessionData"), tmpDirLoc) );

   //FileNames::SetDefaultTempDir( wxString::Format(
   //   wxT("%s/audacity-%s"),
   //   tmpDirLoc,
   //   wxGetUserId() ) );
#endif //__WXMAC__

   FileNames::SetAudacityPathList( std::move( audacityPathList ) );

   // Define languanges for which we have translations, but that are not yet
   // supported by wxWidgets.
   //
   // TODO:  The whole Language initialization really need to be reworked.
   //        It's all over the place.
#if wxCHECK_VERSION(3, 0, 1)
   for (size_t i = 0, cnt = WXSIZEOF(userLangs); i < cnt; i++)
   {
      wxLocale::AddLanguage(userLangs[i]);
   }
#endif

   // Initialize preferences and language
   wxFileName configFileName(FileNames::DataDir(), wxT("audacity.cfg"));
   InitPreferences( configFileName );
   PopulatePreferences();
   // This test must follow PopulatePreferences, because if an error message
   // must be shown, we need internationalization to have been initialized
   // first, which was done in PopulatePreferences
   if ( !CheckWritablePreferences() ) {
      ::AudacityMessageBox(
         UnwritablePreferencesErrorMessage( configFileName ) );
      return false;
   }

#if defined(__WXMSW__) && !defined(__WXUNIVERSAL__) && !defined(__CYGWIN__)
   this->AssociateFileTypes();
#endif

   theTheme.EnsureInitialised();

   // AColor depends on theTheme.
   AColor::Init();

   // Init DirManager, which initializes the temp directory
   // If this fails, we must exit the program.
   if (!InitTempDir()) {
      FinishPreferences();
      return false;
   }

   //<<<< Try to avoid dialogs before this point.
   // The reason is that InitTempDir starts the single instance checker.
   // If we're waiitng in a dialog before then we can very easily
   // start multiple instances, defeating the single instance checker.

   // Initialize the CommandHandler
   InitCommandHandler();

   // Initialize the PluginManager
   PluginManager::Get().Initialize();

   // Initialize the ModuleManager, including loading found modules
   ModuleManager::Get().Initialize(*mCmdHandler);

   // Parse command line and handle options that might require
   // immediate exit...no need to initialize all of the audio
   // stuff to display the version string.
   std::shared_ptr< wxCmdLineParser > parser{ ParseCommandLine().release() };
   if (!parser)
   {
      // Either user requested help or a parsing error occured
      exit(1);
   }

   if (parser->Found(wxT("v")))
   {
      wxPrintf("Audacity v%s\n", AUDACITY_VERSION_STRING);
      exit(0);
   }

   long lval;
   if (parser->Found(wxT("b"), &lval))
   {
      if (lval < 256 || lval > 100000000)
      {
         wxPrintf(_("Block size must be within 256 to 100000000\n"));
         exit(1);
      }

      Sequence::SetMaxDiskBlockSize(lval);
   }

   wxString fileName;
   if (parser->Found(wxT("d"), &fileName))
   {
      AutoSaveFile asf;
      if (asf.Decode(fileName))
      {
         wxPrintf(_("File decoded successfully\n"));
      }
      else
      {
         wxPrintf( AutoSaveFile::FailureMessage( fileName ) );
      }
      exit(1);
   }

   // BG: Create a temporary window to set as the top window
   wxImage logoimage((const char **)AudacityLogoWithName_xpm);
   logoimage.Rescale(logoimage.GetWidth() / 2, logoimage.GetHeight() / 2);
   if( GetLayoutDirection() == wxLayout_RightToLeft)
      logoimage = logoimage.Mirror();
   wxBitmap logo(logoimage);

   AudacityProject *project;
   {
      // Bug 718: Position splash screen on same screen 
      // as where Audacity project will appear.
      wxRect wndRect;
      bool bMaximized = false;
      bool bIconized = false;
      GetNextWindowPlacement(&wndRect, &bMaximized, &bIconized);

      wxSplashScreen temporarywindow(
         logo,
         wxSPLASH_CENTRE_ON_SCREEN | wxSPLASH_NO_TIMEOUT,
         0,
         NULL,
         wxID_ANY,
         wndRect.GetTopLeft(),
         wxDefaultSize,
         wxSTAY_ON_TOP);

      // Unfortunately with the Windows 10 Creators update, the splash screen 
      // now appears before setting its position.
      // On a dual monitor screen it will appear on one screen and then 
      // possibly jump to the second.
      // We could fix this by writing our own splash screen and using Hide() 
      // until the splash scren was correctly positioned, then Show()

      // Possibly move it on to the second screen...
      temporarywindow.SetPosition( wndRect.GetTopLeft() );
      // Centered on whichever screen it is on.
      temporarywindow.Center();
      temporarywindow.SetTitle(_("Audacity is starting up..."));
      SetTopWindow(&temporarywindow);

      // ANSWER-ME: Why is YieldFor needed at all?
      //wxEventLoopBase::GetActive()->YieldFor(wxEVT_CATEGORY_UI|wxEVT_CATEGORY_USER_INPUT|wxEVT_CATEGORY_UNKNOWN);
      wxEventLoopBase::GetActive()->YieldFor(wxEVT_CATEGORY_UI);

      //JKC: Would like to put module loading here.

      // More initialization

      InitDitherers();
      AudioIO::Init();

#ifdef __WXMAC__

      // On the Mac, users don't expect a program to quit when you close the last window.
      // Create a menubar that will show when all project windows are closed.

      auto fileMenu = std::make_unique<wxMenu>();
      auto urecentMenu = std::make_unique<wxMenu>();
      auto recentMenu = urecentMenu.get();
      fileMenu->Append(wxID_NEW, wxString(_("&New")) + wxT("\tCtrl+N"));
      fileMenu->Append(wxID_OPEN, wxString(_("&Open...")) + wxT("\tCtrl+O"));
      fileMenu->AppendSubMenu(urecentMenu.release(), _("Open &Recent..."));
      fileMenu->Append(wxID_ABOUT, _("&About Audacity..."));
      fileMenu->Append(wxID_PREFERENCES, wxString(_("&Preferences...")) + wxT("\tCtrl+,"));

      {
         auto menuBar = std::make_unique<wxMenuBar>();
         menuBar->Append(fileMenu.release(), _("&File"));

         // PRL:  Are we sure wxWindows will not leak this menuBar?
         // The online documentation is not explicit.
         wxMenuBar::MacSetCommonMenuBar(menuBar.release());
      }

      auto &recentFiles = FileHistory::Global();
      recentFiles.UseMenu(recentMenu);
      recentFiles.AddFilesToMenu(recentMenu);

      SetExitOnFrameDelete(false);

#endif //__WXMAC__
      temporarywindow.Show(false);
   }

   // Workaround Bug 1377 - Crash after Audacity starts and low disk space warning appears
   // The temporary splash window is closed AND cleaned up, before attempting to create
   // a project and possibly creating a modal warning dialog by doing so.
   // Also fixes problem of warning being obscured.
   // Downside is that we have no splash screen for the (brief) time that we spend
   // creating the project.
   // Root cause is problem with wxSplashScreen and other dialogs co-existing, that
   // seemed to arrive with wx3.
   {
      project = ProjectManager::New();
      mCmdHandler->SetProject(project);
      wxWindow * pWnd = MakeHijackPanel();
      if (pWnd)
      {
         auto &window = GetProjectFrame( *project );
         window.Show(false);
         pWnd->SetParent( &window );
         SetTopWindow(pWnd);
         pWnd->Show(true);
      }
   }

   if( ProjectSettings::Get( *project ).GetShowSplashScreen() ){
      // This may do a check-for-updates at every start up.
      // Mainly this is to tell users of ALPHAS who don't know that they have an ALPHA.
      // Disabled for now, after discussion.
      // project->MayCheckForUpdates();
      SplashDialog::DoHelpWelcome(*project);
   }

   #ifdef USE_FFMPEG
   FFmpegStartup();
   #endif

   Importer::Get().Initialize();

   // Bug1561: delay the recovery dialog, to avoid crashes.
   CallAfter( [=] () mutable {
      //
      // Auto-recovery
      //
      bool didRecoverAnything = false;
      if (!ShowAutoRecoveryDialogIfNeeded(&project, &didRecoverAnything))
      {
         // Important: Prevent deleting any temporary files!
         DirManager::SetDontDeleteTempFiles();
         QuitAudacity(true);
      }

      //
      // Remainder of command line parsing, but only if we didn't recover
      //
      if (!didRecoverAnything)
      {
         if (parser->Found(wxT("t")))
         {
            RunBenchmark(NULL);
            QuitAudacity(true);
         }

         // As of wx3, there's no need to process the filename arguments as they
         // will be sent via the MacOpenFile() method.
#if !defined(__WXMAC__)
         for (size_t i = 0, cnt = parser->GetParamCount(); i < cnt; i++)
         {
            // PRL: Catch any exceptions, don't try this file again, continue to
            // other files.
            SafeMRUOpen(parser->GetParam(i));
         }
#endif
      }
   } );

   gInited = true;

   ModuleManager::Get().Dispatch(AppInitialized);

   mTimer.SetOwner(this, kAudacityAppTimerID);
   mTimer.Start(200);

#ifdef EXPERIMENTAL_EASY_CHANGE_KEY_BINDINGS
   CommandManager::SetMenuHook( [](const CommandID &id){
      if (::wxGetMouseState().ShiftDown()) {
         // Only want one page of the preferences
         PrefsDialog::Factories factories;
         factories.push_back(KeyConfigPrefsFactory( id ));
         auto pWindow = FindProjectFrame( GetActiveProject() );
         GlobalPrefsDialog dialog( pWindow, factories );
         dialog.ShowModal();
         MenuCreator::RebuildAllMenuBars();
         return true;
      }
      else
         return false;
   } );
#endif

   return TRUE;
}

void AudacityApp::InitCommandHandler()
{
   mCmdHandler = std::make_unique<CommandHandler>();
   //SetNextHandler(mCmdHandler);
}

// AppCommandEvent callback - just pass the event on to the CommandHandler
void AudacityApp::OnReceiveCommand(AppCommandEvent &event)
{
   wxASSERT(NULL != mCmdHandler);
   mCmdHandler->OnReceiveCommand(event);
}

void AudacityApp::OnKeyDown(wxKeyEvent &event)
{
   if(event.GetKeyCode() == WXK_ESCAPE) {
      // Stop play, including scrub, but not record
      auto project = ::GetActiveProject();
      auto token = ProjectAudioIO::Get( *project ).GetAudioIOToken();
      auto &scrubber = Scrubber::Get( *project );
      auto scrubbing = scrubber.HasMark();
      if (scrubbing)
         scrubber.Cancel();
      auto gAudioIO = AudioIO::Get();
      if((token > 0 &&
               gAudioIO->IsAudioTokenActive(token) &&
               gAudioIO->GetNumCaptureChannels() == 0) ||
         scrubbing)
         // ESC out of other play (but not record)
         ProjectAudioManager::Get( *project ).Stop();
      else
         event.Skip();
   }
   else
      event.Skip();
}

// Ensures directory is created and puts the name into result.
// result is unchanged if unsuccessful.
void SetToExtantDirectory( wxString & result, const wxString & dir ){
   // don't allow path of "".
   if( dir.empty() )
      return;
   if( wxDirExists( dir ) ){
      result = dir;
      return;
   }
   // Use '/' so that this works on Mac and Windows alike.
   wxFileName name( dir + "/junkname.cfg" );
   if( name.Mkdir( wxS_DIR_DEFAULT , wxPATH_MKDIR_FULL ) )
      result = dir;
}

bool AudacityApp::InitTempDir()
{
   // We need to find a temp directory location.

   wxString tempFromPrefs = gPrefs->Read(wxT("/Directories/TempDir"), wxT(""));
   auto tempDefaultLoc = FileNames::DefaultTempDir();

   wxString temp;

   #ifdef __WXGTK__
   if (tempFromPrefs.length() > 0 && tempFromPrefs[0] != wxT('/'))
      tempFromPrefs = wxT("");
   #endif

   // Stop wxWidgets from printing its own error messages

   wxLogNull logNo;

   // Try temp dir that was stored in prefs first
   if( FileNames::IsTempDirectoryNameOK( tempFromPrefs ) )
      SetToExtantDirectory( temp, tempFromPrefs );

   // If that didn't work, try the default location

   if (temp.empty())
      SetToExtantDirectory( temp, tempDefaultLoc );

   // Check temp directory ownership on *nix systems only
   #ifdef __UNIX__
   struct stat tempStatBuf;
   if ( lstat(temp.mb_str(), &tempStatBuf) != 0 ) {
      temp.clear();
   }
   else {
      if ( geteuid() != tempStatBuf.st_uid ) {
         temp.clear();
      }
   }
   #endif

   if (temp.empty()) {
      // Failed
      if( !FileNames::IsTempDirectoryNameOK( tempFromPrefs ) ) {
         AudacityMessageBox(_("Audacity could not find a safe place to store temporary files.\nAudacity needs a place where automatic cleanup programs won't delete the temporary files.\nPlease enter an appropriate directory in the preferences dialog."));
      } else {
         AudacityMessageBox(_("Audacity could not find a place to store temporary files.\nPlease enter an appropriate directory in the preferences dialog."));
      }

      // Only want one page of the preferences
      PrefsDialog::Factories factories;
      factories.push_back(DirectoriesPrefsFactory());
      GlobalPrefsDialog dialog(NULL, factories);
      dialog.ShowModal();

      AudacityMessageBox(_("Audacity is now going to exit. Please launch Audacity again to use the new temporary directory."));
      return false;
   }

   // The permissions don't always seem to be set on
   // some platforms.  Hopefully this fixes it...
   #ifdef __UNIX__
   chmod(OSFILENAME(temp), 0755);
   #endif

   bool bSuccess = gPrefs->Write(wxT("/Directories/TempDir"), temp) && gPrefs->Flush();
   DirManager::SetTempDir(temp);

   // Make sure the temp dir isn't locked by another process.
   if (!CreateSingleInstanceChecker(temp))
      return false;

   return bSuccess;
}

// Return true if there are no other instances of Audacity running,
// false otherwise.
//
// Use "dir" for creating lockfiles (on OS X and Unix).

bool AudacityApp::CreateSingleInstanceChecker(const wxString &dir)
{
   wxString name = wxString::Format(wxT("audacity-lock-%s"), wxGetUserId());
   mChecker.reset();
   auto checker = std::make_unique<wxSingleInstanceChecker>();

#if defined(__UNIX__)
   wxString sockFile(dir + wxT("/.audacity.sock"));
#endif

   wxString runningTwoCopiesStr = _("Running two copies of Audacity simultaneously may cause\ndata loss or cause your system to crash.\n\n");

   if (!checker->Create(name, dir)) {
      // Error initializing the wxSingleInstanceChecker.  We don't know
      // whether there is another instance running or not.

      wxString prompt =
         _("Audacity was not able to lock the temporary files directory.\nThis folder may be in use by another copy of Audacity.\n") +
         runningTwoCopiesStr +
         _("Do you still want to start Audacity?");
      int action = AudacityMessageBox(prompt,
                                _("Error Locking Temporary Folder"),
                                wxYES_NO | wxICON_EXCLAMATION,
                                NULL);
      if (action == wxNO)
         return false;
   }
   else if ( checker->IsAnotherRunning() ) {
      // Parse the command line to ensure correct syntax, but
      // ignore options other than -v, and only use the filenames, if any.
      auto parser = ParseCommandLine();
      if (!parser)
      {
         // Complaints have already been made
         return false;
      }

      if (parser->Found(wxT("v")))
      {
         wxPrintf("Audacity v%s\n", AUDACITY_VERSION_STRING);
         return false;
      }

      // Windows and Linux require absolute file names as command may
      // not come from current working directory.
      FilePaths filenames;
      for (size_t i = 0, cnt = parser->GetParamCount(); i < cnt; i++)
      {
         wxFileName filename(parser->GetParam(i));
         if (filename.MakeAbsolute())
            filenames.push_back(filename.GetLongPath());
      }

#if defined(__WXMSW__)
      // On Windows, we attempt to make a connection
      // to an already active Audacity.  If successful, we send
      // the first command line argument (the audio file name)
      // to that Audacity for processing.
      wxClient client;

      // We try up to 50 times since there's a small window
      // where the server may not have been fully initialized.
      for (int i = 0; i < 50; i++)
      {
         std::unique_ptr<wxConnectionBase> conn{ client.MakeConnection(wxEmptyString, IPC_APPL, IPC_TOPIC) };
         if (conn)
         {
            bool ok = false;
            if (filenames.size() > 0)
            {
               for (size_t i = 0, cnt = filenames.size(); i < cnt; i++)
               {
                  ok = conn->Execute(filenames[i]);
               }
            }
            else
            {
               // Send an empty string to force existing Audacity to front
               ok = conn->Execute(wxEmptyString);
            }

            if (ok)
               return false;
         }

         wxMilliSleep(10);
      }
#else
      // On Unix-like machines, we use a local (file based) socket to
      // send the first command line argument to an already running
      // Audacity.
      wxUNIXaddress addr;
      addr.Filename(sockFile);

      {
         // Setup the socket
         // A wxSocketClient must not be deleted by us, but rather, let the
         // framework do appropriate delayed deletion after Destroy()
         Destroy_ptr<wxSocketClient> sock { safenew wxSocketClient() };
         sock->SetFlags(wxSOCKET_WAITALL);

         // We try up to 50 times since there's a small window
         // where the server may not have been fully initialized.
         for (int i = 0; i < 50; i++)
         {
            // Connect to the existing Audacity
            sock->Connect(addr, true);
            if (sock->IsConnected())
            {
               if (filenames.size() > 0)
               {
                  for (size_t i = 0, cnt = filenames.size(); i < cnt; i++)
                  {
                     const wxString param = filenames[i];
                     sock->WriteMsg((const wxChar *) param, (param.length() + 1) * sizeof(wxChar));
                  }
               }
               else
               {
                  // Send an empty string to force existing Audacity to front
                  sock->WriteMsg(wxEmptyString, sizeof(wxChar));
               }

               return sock->Error();
            }

            wxMilliSleep(100);
         }
      }
#endif
      // There is another copy of Audacity running.  Force quit.

      wxString prompt =
         _("The system has detected that another copy of Audacity is running.\n") +
         runningTwoCopiesStr +
         _("Use the New or Open commands in the currently running Audacity\nprocess to open multiple projects simultaneously.\n");
      AudacityMessageBox(prompt, _("Audacity is already running"),
            wxOK | wxICON_ERROR);
      return false;
   }

#if defined(__WXMSW__)
   // Create the DDE IPC server
   mIPCServ = std::make_unique<IPCServ>(IPC_APPL);
#else
   int mask = umask(077);
   remove(OSFILENAME(sockFile));
   wxUNIXaddress addr;
   addr.Filename(sockFile);
   mIPCServ = std::make_unique<wxSocketServer>(addr, wxSOCKET_NOWAIT);
   umask(mask);

   if (!mIPCServ || !mIPCServ->IsOk())
   {
      // TODO:  Complain here
      return false;
   }

   mIPCServ->SetEventHandler(*this, ID_IPC_SERVER);
   mIPCServ->SetNotify(wxSOCKET_CONNECTION_FLAG);
   mIPCServ->Notify(true);
#endif
   mChecker = std::move(checker);
   return true;
}

#if defined(__UNIX__)
void AudacityApp::OnServerEvent(wxSocketEvent & /* evt */)
{
   wxSocketBase *sock;

   // Accept all pending connection requests
   do
   {
      sock = mIPCServ->Accept(false);
      if (sock)
      {
         // Setup the socket
         sock->SetEventHandler(*this, ID_IPC_SOCKET);
         sock->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG);
         sock->Notify(true);
      }
   } while (sock);
}

void AudacityApp::OnSocketEvent(wxSocketEvent & evt)
{
   wxSocketBase *sock = evt.GetSocket();

   if (evt.GetSocketEvent() == wxSOCKET_LOST)
   {
      sock->Destroy();
      return;
   }

   // Read the length of the filename and bail if we have a short read
   wxChar name[PATH_MAX];
   sock->ReadMsg(&name, sizeof(name));
   if (!sock->Error())
   {
      // Add the filename to the queue.  It will be opened by
      // the OnTimer() event when it is safe to do so.
      ofqueue.push_back(name);
   }
}

#endif

std::unique_ptr<wxCmdLineParser> AudacityApp::ParseCommandLine()
{
   auto parser = std::make_unique<wxCmdLineParser>(argc, argv);
   if (!parser)
   {
      return nullptr;
   }

   /*i18n-hint: This controls the number of bytes that Audacity will
    *           use when writing files to the disk */
   parser->AddOption(wxT("b"), wxT("blocksize"), _("set max disk block size in bytes"),
                     wxCMD_LINE_VAL_NUMBER);

   /*i18n-hint: This decodes an autosave file */
   parser->AddOption(wxT("d"), wxT("decode"), _("decode an autosave file"),
                     wxCMD_LINE_VAL_STRING);

   /*i18n-hint: This displays a list of available options */
   parser->AddSwitch(wxT("h"), wxT("help"), _("this help message"),
                     wxCMD_LINE_OPTION_HELP);

   /*i18n-hint: This runs a set of automatic tests on Audacity itself */
   parser->AddSwitch(wxT("t"), wxT("test"), _("run self diagnostics"));

   /*i18n-hint: This displays the Audacity version */
   parser->AddSwitch(wxT("v"), wxT("version"), _("display Audacity version"));

   /*i18n-hint: This is a list of one or more files that Audacity
    *           should open upon startup */
   parser->AddParam(_("audio or project file name"),
                    wxCMD_LINE_VAL_STRING,
                    wxCMD_LINE_PARAM_MULTIPLE | wxCMD_LINE_PARAM_OPTIONAL);

   // Run the parser
   if (parser->Parse() == 0)
      return parser;

   return{};
}

void AudacityApp::OnQueryEndSession(wxCloseEvent & event)
{
   bool mustVeto = false;

#ifdef __WXMAC__
   mustVeto = wxDialog::OSXHasModalDialogsOpen();
#endif

   if ( mustVeto )
      event.Veto(true);
   else
      OnEndSession(event);
}

void AudacityApp::OnEndSession(wxCloseEvent & event)
{
   bool force = !event.CanVeto();

   // Try to close each open window.  If the user hits Cancel
   // in a Save Changes dialog, don't continue.
   gIsQuitting = true;
   if (AllProjects{}.size())
      // PRL:  Always did at least once before close might be vetoed
      // though I don't know why that is important
      ProjectManager::SaveWindowSize();
   bool closedAll = AllProjects::Close( force );
   if ( !closedAll )
   {
      gIsQuitting = false;
      event.Veto();
   }
}

int AudacityApp::OnExit()
{
   gIsQuitting = true;
   while(Pending())
   {
      Dispatch();
   }

   Importer::Get().Terminate();

   if(gPrefs)
   {
      bool bFalse = false;
      //Should we change the commands.cfg location next startup?
      if(gPrefs->Read(wxT("/QDeleteCmdCfgLocation"), &bFalse))
      {
         gPrefs->DeleteEntry(wxT("/QDeleteCmdCfgLocation"));
         gPrefs->Write(wxT("/DeleteCmdCfgLocation"), true);
         gPrefs->Flush();
      }
   }

   FileHistory::Global().Save(*gPrefs, wxT("RecentFiles"));

   FinishPreferences();

#ifdef USE_FFMPEG
   DropFFmpegLibs();
#endif

   DeinitFFT();

   AudioIO::Deinit();

   // Terminate the PluginManager (must be done before deleting the locale)
   PluginManager::Get().Terminate();

   if (mIPCServ)
   {
#if defined(__UNIX__)
      wxUNIXaddress addr;
      if (mIPCServ->GetLocal(addr))
      {
         remove(OSFILENAME(addr.Filename()));
      }
#endif
   }

   return 0;
}

// The following five methods are currently only used on Mac OS,
// where it's possible to have a menu bar but no windows open.
// It doesn't hurt any other platforms, though.

// ...That is, as long as you check to see if no windows are open
// before executing the stuff.
// To fix this, check to see how many project windows are open,
// and skip the event unless none are open (which should only happen
// on the Mac, at least currently.)

void AudacityApp::OnMenuAbout(wxCommandEvent & /*event*/)
{
   // This function shadows a similar function
   // in Menus.cpp, but should only be used on the Mac platform.
#ifdef __WXMAC__
   // Modeless dialog, consistent with other Mac applications
   // Not more than one at once!
   const auto instance = AboutDialog::ActiveIntance();
   if (instance)
      instance->Raise();
   else
      // This dialog deletes itself when dismissed
      (safenew AboutDialog{ nullptr })->Show(true);
#else
      wxASSERT(false);
#endif
}

void AudacityApp::OnMenuNew(wxCommandEvent & event)
{
   // This function shadows a similar function
   // in Menus.cpp, but should only be used on the Mac platform
   // when no project windows are open. This check assures that
   // this happens, and enable the same code to be present on
   // all platforms.

   if(AllProjects{}.empty())
      (void) ProjectManager::New();
   else
      event.Skip();
}


void AudacityApp::OnMenuOpen(wxCommandEvent & event)
{
   // This function shadows a similar function
   // in Menus.cpp, but should only be used on the Mac platform
   // when no project windows are open. This check assures that
   // this happens, and enable the same code to be present on
   // all platforms.


   if(AllProjects{}.empty())
      ProjectManager::OpenFiles(NULL);
   else
      event.Skip();


}

void AudacityApp::OnMenuPreferences(wxCommandEvent & event)
{
   // This function shadows a similar function
   // in Menus.cpp, but should only be used on the Mac platform
   // when no project windows are open. This check assures that
   // this happens, and enable the same code to be present on
   // all platforms.

   if(AllProjects{}.empty()) {
      GlobalPrefsDialog dialog(NULL /* parent */ );
      dialog.ShowModal();
   }
   else
      event.Skip();

}

void AudacityApp::OnMenuExit(wxCommandEvent & event)
{
   // This function shadows a similar function
   // in Menus.cpp, but should only be used on the Mac platform
   // when no project windows are open. This check assures that
   // this happens, and enable the same code to be present on
   // all platforms.

   // LL:  Removed "if" to allow closing based on final project count.
   // if(AllProjects{}.empty())
      QuitAudacity();

   // LL:  Veto quit if projects are still open.  This can happen
   //      if the user selected Cancel in a Save dialog.
   event.Skip(AllProjects{}.empty());

}

//BG: On Windows, associate the aup file type with Audacity
/* We do this in the Windows installer now,
   to avoid issues where user doesn't have admin privileges, but
   in case that didn't work, allow the user to decide at startup.

   //v Should encapsulate this & allow access from Prefs, too,
   //      if people want to manually change associations.
*/
#if defined(__WXMSW__) && !defined(__WXUNIVERSAL__) && !defined(__CYGWIN__)
void AudacityApp::AssociateFileTypes()
{
   wxRegKey associateFileTypes;
   associateFileTypes.SetName(wxT("HKCR\\.AUP"));
   bool bKeyExists = associateFileTypes.Exists();
   if (!bKeyExists) {
      // Not at HKEY_CLASSES_ROOT. Try HKEY_CURRENT_USER.
      associateFileTypes.SetName(wxT("HKCU\\Software\\Classes\\.AUP"));
      bKeyExists = associateFileTypes.Exists();
   }
   if (!bKeyExists) {
      // File types are not currently associated.
      // Check pref in case user has already decided against it.
      bool bWantAssociateFiles = true;
      if (!gPrefs->Read(wxT("/WantAssociateFiles"), &bWantAssociateFiles) ||
            bWantAssociateFiles) {
         // Either there's no pref or user does want associations
         // and they got stepped on, so ask.
         int wantAssoc =
            AudacityMessageBox(
               _("Audacity project (.AUP) files are not currently \nassociated with Audacity. \n\nAssociate them, so they open on double-click?"),
               _("Audacity Project Files"),
               wxYES_NO | wxICON_QUESTION);
         if (wantAssoc == wxYES) {
            gPrefs->Write(wxT("/WantAssociateFiles"), true);
            gPrefs->Flush();

            wxString root_key;

            root_key = wxT("HKCU\\Software\\Classes\\");
            associateFileTypes.SetName(root_key + wxT(".AUP")); // Start again with HKEY_CLASSES_ROOT.
            if (!associateFileTypes.Create(true)) {
               // Not at HKEY_CLASSES_USER. Try HKEY_CURRENT_ROOT.
               root_key = wxT("HKCR\\");
               associateFileTypes.SetName(root_key + wxT(".AUP"));
               if (!associateFileTypes.Create(true)) {
                  // Actually, can't create keys. Empty root_key to flag failure.
                  root_key.Empty();
               }
            }
            if (root_key.empty()) {
               //v Warn that we can't set keys. Ask whether to set pref for no retry?
            } else {
               associateFileTypes = wxT("Audacity.Project"); // Finally set value for .AUP key

               associateFileTypes.SetName(root_key + wxT("Audacity.Project"));
               if(!associateFileTypes.Exists()) {
                  associateFileTypes.Create(true);
                  associateFileTypes = wxT("Audacity Project File");
               }

               associateFileTypes.SetName(root_key + wxT("Audacity.Project\\shell"));
               if(!associateFileTypes.Exists()) {
                  associateFileTypes.Create(true);
                  associateFileTypes = wxT("");
               }

               associateFileTypes.SetName(root_key + wxT("Audacity.Project\\shell\\open"));
               if(!associateFileTypes.Exists()) {
                  associateFileTypes.Create(true);
               }

               associateFileTypes.SetName(root_key + wxT("Audacity.Project\\shell\\open\\command"));
               wxString tmpRegAudPath;
               if(associateFileTypes.Exists()) {
                  tmpRegAudPath = wxString(associateFileTypes).Lower();
               }
               if (!associateFileTypes.Exists() ||
                     (tmpRegAudPath.Find(wxT("audacity.exe")) >= 0)) {
                  associateFileTypes.Create(true);
                  associateFileTypes = (wxString)argv[0] + (wxString)wxT(" \"%1\"");
               }

#if 0
               // These can be use later to support more startup messages
               // like maybe "Import into existing project" or some such.
               // Leaving here for an example...
               associateFileTypes.SetName(root_key + wxT("Audacity.Project\\shell\\open\\ddeexec"));
               if(!associateFileTypes.Exists()) {
                  associateFileTypes.Create(true);
                  associateFileTypes = wxT("%1");
               }

               associateFileTypes.SetName(root_key + wxT("Audacity.Project\\shell\\open\\ddeexec\\Application"));
               if(!associateFileTypes.Exists()) {
                  associateFileTypes.Create(true);
                  associateFileTypes = IPC_APPL;
               }

               associateFileTypes.SetName(root_key + wxT("Audacity.Project\\shell\\open\\ddeexec\\Topic"));
               if(!associateFileTypes.Exists()) {
                  associateFileTypes.Create(true);
                  associateFileTypes = IPC_TOPIC;
               }
#endif
            }
         } else {
            // User said no. Set a pref so we don't keep asking.
            gPrefs->Write(wxT("/WantAssociateFiles"), false);
            gPrefs->Flush();
         }
      }
   }
}
#endif

