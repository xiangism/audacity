/**********************************************************************

  Audacity: A Digital Audio Editor

  DirManager.h

  Dominic Mazzoni

**********************************************************************/

#ifndef _DIRMANAGER_
#define _DIRMANAGER_

#include "audacity/Types.h"
#include "xml/XMLTagHandler.h"
#include "wxFileNameWrapper.h"
#include "RngSupport.h"

#include <functional>
#include <memory>
#include <unordered_map>

#include "ClientData.h"

class wxFileNameWrapper;
class AudacityProject;
class BlockArray;
class BlockFile;
class ProgressDialog;

using DirHash = std::unordered_map<int, int>;

class BlockFile;
using BlockFilePtr = std::shared_ptr<BlockFile>;

using BlockHash = std::unordered_map< wxString, std::weak_ptr<BlockFile> >;

wxMemorySize GetFreeMemory();

enum {
   kCleanTopDirToo = 1,        // The top directory can be excluded from clean.
   kCleanDirsOnlyIfEmpty = 2,  // Otherwise non empty dirs may be removed.
   kCleanFiles = 4,            // Remove files
   kCleanDirs = 8              // Remove dirs.
};


class PROFILE_DLL_API DirManager final
   : public XMLTagHandler
   , public ClientData::Base
   , public std::enable_shared_from_this< DirManager >
{
 public:

   static DirManager &Get( AudacityProject &project );
   static const DirManager &Get( const AudacityProject &project );
   static DirManager &Reset( AudacityProject &project );
   static void Destroy( AudacityProject &project );

   static int RecursivelyEnumerate(const FilePath &dirPath,
                                     FilePaths& filePathArray,  // output: all files in dirPath tree
                                     wxString dirspec,
                                     wxString filespec,
                                     bool bFiles, bool bDirs,
                                     int progress_count = 0,
                                     int progress_bias = 0,
                                     ProgressDialog* progress = nullptr);

   static int RecursivelyEnumerateWithProgress(const FilePath &dirPath,
                                                FilePaths& filePathArray, // output: all files in dirPath tree
                                                wxString dirspec,
                                                wxString filespec,
                                                bool bFiles, bool bDirs,
                                                int progress_count,
                                                const wxChar* message);

   static int RecursivelyCountSubdirs( const FilePath &dirPath );

   static int RecursivelyRemoveEmptyDirs(const FilePath &dirPath,
                                          int nDirCount = 0,
                                          ProgressDialog* pProgress = nullptr);

   static void RecursivelyRemove(const FilePaths& filePathArray, int count, int bias,
                                 int flags, const wxChar* message = nullptr);

   // Type of a function that builds a block file, using attributes from XML
   using BlockFileDeserializer =
      std::function< BlockFilePtr( DirManager&, const wxChar ** ) >;
   // Typically a statically declared object,
   // registers a function for an XML tag
   struct RegisteredBlockFileDeserializer {
      RegisteredBlockFileDeserializer(
         const wxString &tag, BlockFileDeserializer function );
   };

 private:
   // MM: Construct DirManager
   // Don't call this directly but use Create() instead
   DirManager();

 public:

   static std::shared_ptr< DirManager > Create();

   virtual ~DirManager();

   size_t NumBlockFiles() const { return mBlockFileHash.size(); }

   static void SetTempDir(const wxString &_temp) { globaltemp = _temp; }

   class ProjectSetter
   {
   public:
      ProjectSetter(
         DirManager &dirManager,
         FilePath& newProjPath,  // assigns it if empty
         const FilePath& newProjName, const bool bCreate, bool moving);
      ~ProjectSetter();

      bool Ok();
      void Commit();

   private:
      struct Impl;
      std::unique_ptr<Impl> mpImpl;
   };

   // Returns true on success.
   // If SetProject is told NOT to create the directory
   // but it doesn't already exist, SetProject fails and returns false.
   // This function simply creates a ProjectSetter and commits it if successful.
   // Using ProjectSetter directly allows separation of those steps.
   bool SetProject(
      FilePath& newProjPath, // assigns it if empty
      const FilePath& newProjName, const bool bCreate);

   FilePath GetProjectDataDir();
   FilePath GetProjectName();

   wxLongLong GetFreeDiskSpace();

   using BlockFileFactory = std::function< BlockFilePtr( wxFileNameWrapper ) >;
   BlockFilePtr NewBlockFile( const BlockFileFactory &factory );

   /// Returns true if the blockfile pointed to by b is contained by the DirManager
   bool ContainsBlockFile(const BlockFile *b) const;
   /// Check for existing using filename using complete filename
   bool ContainsBlockFile(const wxString &filepath) const;

   // Adds one to the reference count of the block file,
   // UNLESS it is "locked", then it makes a NEW copy of
   // the BlockFile.
   // May throw an exception in case of disk space exhaustion, otherwise
   // returns non-null.
   BlockFilePtr CopyBlockFile(const BlockFilePtr &b);

   BlockFile *LoadBlockFile(const wxChar **attrs, sampleFormat format);
   void SaveBlockFile(BlockFile *f, int depth, FILE *fp);

#if LEGACY_PROJECT_FILE_SUPPORT
   BlockFile *LoadBlockFile(wxTextFile * in, sampleFormat format);
   void SaveBlockFile(BlockFile * f, wxTextFile * out);
#endif

   std::pair<bool, FilePath>
      LinkOrCopyToNewProjectDirectory(BlockFile *f, bool &link);

   bool EnsureSafeFilename(const wxFileName &fName);

   using LoadingTarget = std::function< BlockFilePtr &() >;
   void SetLoadingTarget( LoadingTarget loadingTarget )
   {
      mLoadingTarget = loadingTarget;
   }
   sampleFormat GetLoadingFormat() const { return mLoadingFormat; }
   void SetLoadingFormat(sampleFormat format) { mLoadingFormat = format; }
   size_t GetLoadingBlockLength() const { return mLoadingBlockLen; }
   void SetLoadingBlockLength(size_t len) { mLoadingBlockLen = len; }

   // Note: following affects only the loading of block files when opening a project
   void SetLoadingMaxSamples(size_t max) { mMaxSamples = max; }

   bool HandleXMLTag(const wxChar *tag, const wxChar **attrs) override;
   XMLTagHandler *HandleXMLChild(const wxChar * WXUNUSED(tag)) override
      { return NULL; }
   bool AssignFile(wxFileNameWrapper &filename, const wxString &value, bool check);

   // Clean the temp dir. Note that now where we have auto recovery the temp
   // dir is not cleaned at start up anymore. But it is cleaned when the
   // program is exited normally.
   static void CleanTempDir();
   static void CleanDir(
      const FilePath &path, 
      const wxString &dirSpec, 
      const wxString &fileSpec, 
      const wxString &msg,
      int flags = 0);

   void FindMissingAliasFiles(
         BlockHash& missingAliasFilesAUFHash,     // output: (.auf) AliasBlockFiles whose aliased files are missing
         BlockHash& missingAliasFilesPathHash);   // output: full paths of missing aliased files
   void FindMissingAUFs(
         BlockHash& missingAUFHash);               // output: missing (.auf) AliasBlockFiles
   void FindMissingAUs(
         BlockHash& missingAUHash);                // missing data (.au) blockfiles
   // Find .au and .auf files that are not in the project.
   void FindOrphanBlockFiles(
         const FilePaths &filePathArray,       // input: all files in project directory
         FilePaths &orphanFilePathArray);      // output: orphan files


   // Remove all orphaned blockfiles without user interaction. This is
   // generally safe, because orphaned blockfiles are not referenced by the
   // project and thus worthless anyway.
   void RemoveOrphanBlockfiles();

   // Get directory where data files are in. Note that projects are normally
   // not interested in this information, but it is important for the
   // auto-save functionality
   FilePath GetDataFilesDir() const;

   // This should only be used by the auto save functionality
   void SetLocalTempDir(const wxString &path);

   // Do not DELETE any temporary files on exit. This is only called if
   // auto recovery is cancelled and should be retried later
   static void SetDontDeleteTempFiles() { dontDeleteTempFiles = true; }

   // Write all write-cached block files to disc, if any
   void WriteCacheToDisk();

   // (Try to) fill cache of blockfiles, if caching is enabled (otherwise do
   // nothing)
   // A no-fail operation that does not throw
   void FillBlockfilesCache();

 private:

   wxFileNameWrapper MakeBlockFileName();
   wxFileNameWrapper MakeBlockFilePath(const wxString &value);

   BlockHash mBlockFileHash; // repository for blockfiles

   // Hashes for management of the sub-directory tree of _data
   struct BalanceInfo
   {
      DirHash   dirTopPool;    // available toplevel dirs
      DirHash   dirTopFull;    // full toplevel dirs
      DirHash   dirMidPool;    // available two-level dirs
      DirHash   dirMidFull;    // full two-level dirs
   } mBalanceInfo;

   // Accessor for the balance info, may need to do a delayed update for
   // deletion in case other threads DELETE block files
   BalanceInfo &GetBalanceInfo();

   void BalanceInfoDel(const wxString&);
   void BalanceInfoAdd(const wxString&);
   void BalanceFileAdd(int);
   int BalanceMidAdd(int, int);

   FilePath projName;
   FilePath projPath;
   FilePath projFull;

   wxString lastProject;

   FilePaths aliasList;

   LoadingTarget mLoadingTarget;
   sampleFormat mLoadingFormat;
   size_t mLoadingBlockLen;

   size_t mMaxSamples; // max samples per block

   unsigned long mLastBlockFileDestructionCount { 0 };

   static wxString globaltemp;
   wxString mytemp;
   static int numDirManagers;
   static bool dontDeleteTempFiles;

   Nyq::NyqEngine<> generator;
   std::uniform_int_distribution<> distribution1M{ 0, 1 << 20 };
   std::uniform_int_distribution<> distribution255{ 0, 255 };
   std::uniform_int_distribution<> distribution4095{ 0, 4095 };
   std::uniform_int_distribution<> distribution_rand_max{ 0, RAND_MAX };
};

#endif
