// BLogger.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BLOGGER_H
#define BLOGGER_H

/*!
  \file BLogger.h

  Loggers and how to manage them.
*/

/* system includes */
#include <stdio.h>
#include <string>
#include <deque>
#include <DSGUI/BVirtualFile.h>

/* my includes */
/* (none) */

//! Criticality level for log messages
typedef enum {
  LOG_FATAL = 0, // Fatal error
  LOG_CRIT = 1,  // Critical error
  LOG_ERROR = 2, // Regular error
  LOG_WARN = 3,  // Warning
  LOG_INFO = 4,  // Informational
  LOG_BLAH = 5   // Everything else
} LogLevel;

//! Macro for logging, printf-style
#define LOG(level, txt...) BLoggerManager::get()->log(level, txt)

/*!
  \class BLogger

  Superclass for all Logger subclasses. To add your own Logger (e.g. a
  graphical one), write a subclass of this and override the log()
  method.
*/
class BLogger {
public:
  virtual ~BLogger();


  /*!
    \brief Add new log entry.

    This method shouldn't be called directly (although it can);
    rather, add the logger to BLoggerManager's set of loggers, and
    BLoggerManager will call this.
  */
  virtual void log(LogLevel level, const std::string& text) = 0;
};


/*!
  \class BFileLogger

  Logger that writes all log entries to a disk file.
*/
class BFileLogger: public BLogger {
public:
  /*!
    \brief Construct a new BFileLogger.

    An exception will be thrown if the file named by filename is not
    accessible. If it is, it will be opened in append mode.
  */
  BFileLogger(const std::string& filename);
  virtual ~BFileLogger();

  //! Add new log entry.
  virtual void log(LogLevel level, const std::string& text);
private:
  BVirtualFile* file;
  std::string filename;
};

/*!
  \class BStdoutLogger

  Logger that writes all log entries to the console (via printf).
*/
class BStdoutLogger: public BLogger {
public:
  virtual ~BStdoutLogger();
  virtual void log(LogLevel level, const std::string& text);
};

/*!
  \class BLoggerManager

  Singleton class that manages the system's logging system. All log
  output should be done using this class's single instance (preferably
  using the LOG() macro.

  BLoggerManager maintains a maximum log level; all log output above
  that level (i.e. less critical) will be suppressed.

  BLoggerManager maintains a list of loggers that can be added to; one
  default logger of class BStdoutLogger is always there. Log messages
  that go to BLoggerManager and that are not suppressed will go to all
  loggers in the list.

  BLoggerManager can be set to wake up the screen saver (using a
  custom call back method) upon receiving new, non-suppressed
  entries.
*/
class BLoggerManager {
public:
  //! Return the single instance of BLoggerManager.
  static BLoggerManager* get();
  ~BLoggerManager();

  //! Log a message, printf() style.
  void log(LogLevel level, const char* format, ...);

  /*!
    \brief Set the maximum log level

    All log output above this level (i.e. less critical) will be
    suppressed.
  */
  void setLogLevel(LogLevel level);

  //! Add a new logger to the system's list.
  void addLogger(BLogger* logger);

  //! Set whether to wake up the screen saver on log entry
  void setWakeOnLog(bool wakeOnLog);

  //! Return whether the screen saver will be woken up on log entry
  bool wakesOnLog() { return wakeOnLog; }

  /*!
    \brief Set the screensaver callback function

    If wakesOnLog() is true, this method will be called upon receiving
    new, non-suppressed log entries.
  */
  void setScreenSaverResetFunction(void (*fn)(void*), void* arg);
  
private:
  BLoggerManager();
  static BLoggerManager* singleton;
  std::deque<BLogger*> loggers;
  BStdoutLogger* defaultLogger;
  LogLevel level;
  bool wakeOnLog;
  void (*ssResetFn)(void*); void *ssResetArg;
};

#endif /* BLOGGER_H */
