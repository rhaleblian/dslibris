// BVirtualFile.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BVIRTUALFILE_H
#define BVIRTUALFILE_H

/* system includes */
#include <nds.h>
#include <stdio.h>
#include <vector>
#include <string>

/* my includes */
/* (none) */

//! File abstraction base class
class BVirtualFile {
public:
  typedef enum {
    WHENCE_START,
    WHENCE_CURRENT,
    WHENCE_END
  } Whence;

  virtual ~BVirtualFile() {}
  
  virtual int read(void* buf, unsigned int nbytes) = 0;
  std::string read();
  
  virtual int write(const void* buf, unsigned int nbytes) = 0;
  int write(const std::string& str) {
    return write(str.c_str(), str.size());
  }
  
  virtual long tell() = 0;
  virtual int seek(long offset, Whence whence) = 0;
  virtual int eof() = 0;

  void rewind() { seek(0, WHENCE_START); }
};

//! FAT (i.e. flash cart) file
class BFATFile: public BVirtualFile {
public:
  BFATFile(const char* filename, const char* mode="r");
  BFATFile(FILE* file, bool closeWhenDone);
  ~BFATFile();
  
  virtual int read(void* buf, unsigned int nbytes);
  std::string read();

  virtual int write(const void* buf, unsigned int nbytes);
  int write(const std::string& str) {
    return write(str.c_str(), str.size());
  }
  virtual long tell();
  virtual int seek(long offset, Whence whence);
  virtual int eof();

private:
  FILE* _file;
  bool _close;
};

//! File abstraction for a memory block
class BMemFile: public BVirtualFile {
public:
  BMemFile(u32 address, u32 len, bool freeWhenDone = false);
  ~BMemFile();

  virtual int read(void* buf, unsigned int nbytes);
  std::string read();

  virtual int write(const void* buf, unsigned int nbytes);
  int write(const std::string& str) {
    return write(str.c_str(), str.size());
  }
  virtual long tell();
  virtual int seek(long offset, Whence whence);
  virtual int eof();

private:
  u32 _adr; u32 _len; u32 _cur; bool _free;
};

//! File management master class
class BFileManager {
public:
  static BFileManager* get();

  typedef enum {
    TYPE_DIRECTORY,
    TYPE_FILE,
    TYPE_NOEXIST
  } FileType;

  typedef struct {
    std::string filename;
    FileType filetype;
  } FileAndType;

  std::vector<FileAndType> directoryContents(const std::string& dir);
  std::vector<FileAndType> directoryContents() {
    return directoryContents(_curdir);
  }
  FileType typeOfFile(const std::string& filename);

  //! Return the file size, -1 if error
  int sizeOfFile(const std::string& filename);

  bool changeDirectory(const std::string& dir);
  std::string currentDirectory();

  bool makeDirectory(const std::string& dir);
  bool renameFile(const std::string& fname1, const std::string& fname2);
  bool removeFile(const std::string& fname);

  std::string normalizePath(const std::string& str);
  std::string lastPathComponent(const std::string& str);
  std::string absolutePath(const std::string& str);

  //! Return filename extension in lower case, starting with "."
  std::string filenameExtension(const std::string& str);
  std::string filenameWithoutExtension(const std::string& str);

private:
  BFileManager();
  static BFileManager* _mgr;
  std::string _curdir;
};
#endif /* BVIRTUALFILE_H */
