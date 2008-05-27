// BFTPServer.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// \mainpage
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

/*!
  \mainpage
  
  Welcome to the libDSFTP reference documentation! This set of HTML
  pages should tell you everything about using libDSFTP to add a FTP
  server to your own app. In the section you're reading now, I'd like
  to go through a quick step-by-step for the most typical use.

  \section Preliminaries

  Adding an FTP server is very easy. First, you should make sure that
  the following components are added to your build process. That means
  their include files must be accessible, and their libraries/object
  files must be linked with your program. 

    - gba_nds_fat (either chishm's FAT lib or REIN's)
    - dswifi
    - libDSFTP

    Then, you initialize the Wifi and FAT libraries as usual. Please
    refer to their respective examples for info on how to do it, or to
    the DSFTP source. 

    \section creating Creating the server

    Next, you create an instance of BFTPServer somewhere in your
    program, like so: 

    \code
	BFTPServer server;
    \endcode

    If you want to, you can also create an instance of
    BFTPConfigurator to configure the server. This is not mandatory,
    but it allows you to configure the FTP server via config
    file. This would look as follows.

    \code
	BFTPConfigurator configurator(&server);
	configurator.configureFromFile("/data/settings/ftp.conf");
    \endcode

    ...or wherever you want the config file to be.

    If you do \b not use a BFTPConfigurator, you \b must add users to
    the FTP server manually. Otherwise, you won't be able to log
    in. The simplest way to do this is like this:

    \code
	server.addUser("myusername", "mypassword");
    \endcode

    Please refer to BFTPServer for other arguments to this command.

    \section work Letting it do its work

    That's it, your FTP server is ready! Now, whenever your program is
    idle (e.g. from the main loop), you should let the server do its
    work, like this: 

    \code
	server.handle();
    \endcode

    \section wrap Wrapping it up

    And that's all you have to do. All the code inside of a nice
    little mainloop function would look like this: 

    \code
	void mainloop(void)
	{
		BFTPServer server;
		BFTPConfigurator configurator(&server);
		configurator.configureFromFile("/data/settings/ftp.conf");
		while(true)
		{
			server.handle();
			swiWaitForVBlank();
		}
	}
    \endcode
    
    Please refer to the source code of DSFTP and the other pages of
    this documentation for other possible settings and features.

    \section about About libDSFTP

    DSFTP is provided free of charge. (Although donations are happily
    accepted, see <a
    href="http://giesler.biz/~bjoern/en/sw_dsftp.html#donate">this
    link</a>.) The software is provided as-is, without any express or
    implicit warranty. Don't expect too much, and don't blame me if it
    wrecks your flash cart, sets your house on fire and rapes your
    dog. You have been warned.
    
    That said, it's reasonably well-behaved here :-) Please send me
    your experiences with DSFTP via per email (replace _AT_ by
    @). Thank you! 
*/

#ifndef BFTPSERVER_H
#define BFTPSERVER_H

/* system includes */
#include <deque>
#include <netinet/in.h>
#include <stdio.h>
#include <fat.h>
#include <DSGUI/BVirtualFile.h>

/* my includes */
#include "BTCPConnection.h"

class BFTPControlConn;

//! Structure that defines a FTP user
typedef struct {
  //! User name ("anonymous" is special)
  std::string name;

  //! Password (unencrypted) or email address if name is "anonymous"
  std::string pass;

  //! Root directory that the user will be kept in
  std::string root;

  //! User home relative to the root
  std::string home;

  //! Whether the user may write files
  bool writePermission;

  //! Whether the user may boot the system
  bool bootPermission;

  //! Whether the user is logged in (via password)
  bool loggedIn;
} BFTPUser;

/*!
  \brief FTP server class

  This class implements a full-fledged FTP server. All you need to do
  is create one (after Wifi is setup and inited and everything), add a
  user (or configure via file) and periodically call handle().
*/
class BFTPServer: public BTCPConnection {
public:

  /*!
    \brief Create a non-bound FTP server

    This is useful for building servers that will be configured via
    config file, because BFTPConfigurator will call bind() with the
    configured port during the configure process.
  */
  BFTPServer();

  //! Create a FTP server bound to the given port
  BFTPServer(short port);
  ~BFTPServer();

  //! Handle any incoming or outgoing traffic. Call this periodically.
  virtual void handle();

  //! Return the number of currently active connections.
  int numActiveConnections() { return connections.size(); }

  /*!
    \name Server configuration
    \{
  */

  //! Add a new user. At least one must be present, or else no-one can login.
  void addUser(const std::string& name,
	       const std::string& pass,
	       const std::string& home = "/",
	       const std::string& root = "/",
	       bool writePermission = true,
	       bool bootPermission = true);

  //! Return true if a user named "anonymous" exists
  bool anonymousLoginOK();

  //! Set the path to the Message of the Day file (printed upon login)
  void setMOTDFile(const std::string& motdFile);
  //! Get the path to the Message of the Day file
  const std::string& getMOTDFile() { return motdFile; }

  /*!
    \brief Tell the server to masquerade as the given host

    Useful for NATed servers that have their ports forwarded from the
    NAT router. The name must be resolvable. Its address will be
    returned as reply to passive-mode requests (PASV, EPSV). If you
    don't know what this means, don't use it.
  */
  void setMasqueradeAsHost(const std::string& host);
  //! Return the masquerading host, or the empty string if none.
  const std::string& getMasqueradeAsHost() { return masqueradeAsHost; }

  /*!
    \brief Set the inactivity timeout in seconds

    After receiving no commands after this many seconds, the
    connection will be closed.
  */
  void setInactivityTimeout(int timeout);
  //! Get the inactivity timeout.
  int getInactivityTimeout() { return inactivityTimeout; }

  //! Set the start of the port range for passive-mode data connections
  void setPortRangeStart(int portRangeStart);
  //! Get the start of the port range for passive-mode data connections
  int getPortRangeStart() { return portRangeStart; }
  //! Set the end of the port range for passive-mode data connections
  void setPortRangeEnd(int portRangeEnd);
  //! Get the end of the port range for passive-mode data connections
  int getPortRangeEnd() { return portRangeEnd; }

  //! Set transfer block size
  void setTransferBlockSize(size_t size) { transferBlockSize = size; }
  //! Get transfer block size
  size_t getTransferBlockSize() { return transferBlockSize; }
  
  /*!
    \}
  */

  /*!
    \name Delegate handling
    \{
  */

  class Delegate {
  public:
    virtual ~Delegate() {}
    //! Return false if opening the connection is not allowed
    virtual bool controlConnectionWillOpen(BFTPServer *server,
					   long address) { return true; }
    virtual void controlConnectionDidOpen(BFTPServer *server,
					  BFTPControlConn *conn) {}
  };

  void setDelegate(Delegate* deleg);
  Delegate* delegate() { return deleg; }

  /*!
    \}
  */
  
  //! Called after a control connection was closed. Don't call directly.
  void connectionDidClose(BFTPControlConn* conn);

  //! obsolete. will go away.
  void setScreenSaverTimeout(int to) { ssTimeout = to; }
  //! obsolete. will go away.
  int screenSaverTimeout() { return ssTimeout; }

protected:
  virtual bool handleAccept();

private:
  std::string motdFile;
  std::deque<BFTPUser> users;
  std::string masqueradeAsHost;
  int inactivityTimeout;
  int portRangeStart, portRangeEnd;
  size_t transferBlockSize;
  std::deque<BFTPControlConn*> connections;
  std::deque<BFTPControlConn*> connectionsToDelete;
  Delegate *deleg;

  int ssTimeout;

public:
  BFTPUser getUserWithName(const std::string& name);
  int getNumberOfUsers() { return users.size(); }
};

/*!
  \class BFTPControlConn

  FTP control connection, managed by the BFTPServer class. Do not use
  directly.
*/
class BFTPControlConn: public BTCPConnection {
protected:

  /*!
    \class BFTPDataConn
    
    FTP data connection, managed by the BFTPControlConn class. Do not use
    directly.
  */
  class BFTPDataConn: public BTCPConnection {
  public:
    BFTPDataConn(long addr, short port, BFTPControlConn* master);
    BFTPDataConn(const char* hostname, short port, BFTPControlConn* master);
    BFTPDataConn(short port, BFTPControlConn* master);
    ~BFTPDataConn();
    
    virtual bool handleAccept();
    virtual bool handleConnection();
    virtual void close();
    bool jobDone() { return jobdone; }
    int finishCode() { return fcode; }
    const std::string& finishMessage() { return fdescr; }

    virtual void startStoring(BVirtualFile* file);
    virtual void startRetrieving(BVirtualFile* file);
    virtual void startSendingData(const std::string& data);

    virtual bool handleStore();
    virtual bool handleRetrieve();
    virtual bool handleSend();

    virtual void abort();

    typedef enum {
      DCMODE_IDLE,
      DCMODE_STORING,
      DCMODE_RETRIEVING,
      DCMODE_SENDINGDATA
    } DCMode;

  private:
    void finish(int code, const std::string& descr);
    
    BFTPControlConn* master;

    std::string data;
    BVirtualFile* file;
    DCMode mode;

    char buf[8192];
    unsigned int buflen, bufindex, total;
    bool finished, remoteclosed, jobdone;
    int fcode; std::string fdescr;
  };

public:
  BFTPControlConn(BFTPServer* master, int conn,
		  const struct sockaddr_in& lastaddr);
  virtual ~BFTPControlConn();

  std::string userInfo();
  const BFTPUser& currentUser();
  
  virtual void close();
  virtual void handle();

  int createDataConnection();
  void destroyDataConnection();
  void updateActivityTime();
  
  /*!
    \name Delegate handling
    \{
  */

  class Delegate {
  public:
    virtual ~Delegate() {}
    virtual void controlConnectionWillClose(BFTPControlConn *conn) {}
    virtual void controlConnectionDidClose(BFTPControlConn *conn) {}

    //! Called every time data is going back and forth
    virtual void activityOnControlConnection(BFTPControlConn *conn) {}

    virtual bool handleUnknownCommand(BFTPControlConn *cconn,
				      std::string& cmd, std::string& arg)
    {
      return false;
    }
      
  };

  void setDelegate(Delegate* deleg) { this->deleg = deleg; }
  Delegate* delegate() { return deleg; }

  /*!
    \}
  */
  
  bool sendReply(int code, const char* format, ...);
  bool sendReply(int code, const std::string& reply);

  //! Make fname in the user's root into a global absolute filename
  std::string makeAbsoluteFilename(const std::string& fname);

  BFTPServer* getMaster() { return master; }

protected:
  virtual bool handleInput(const unsigned char* buf, int buflen);

  bool setLoginName(const std::string& name);
  bool setLoginPassword(const std::string& pass);
  bool setCurrentUserByName(const std::string& name);

  bool changeWorkingDir(const std::string& newwd);
  bool listDir(BFTPDataConn* dc, const std::string& args,
	       bool namesOnly = false);
  bool storeFile(BFTPDataConn* dc, const std::string& args);
  bool retrieveFile(BFTPDataConn* dc, const std::string& args);

  void closeIfInactive();

  BFTPDataConn* dc;
  BFTPServer* master;
  Delegate *deleg;

  void dcHandleAccept(BFTPDataConn* dc);
  void dcReportFinish(BFTPDataConn* dc,
		      BFTPDataConn::DCMode mode,
		      bool remoteclosed);

  std::string cwd, fullcwd;
  std::string lastcmd, lastarg;
  std::string renameFrom;

  char buf[4096];

  typedef enum {
    TYPE_ASCII,
    TYPE_IMAGE
  } TransferType;
  TransferType ttype;

  BFTPUser user;

  time_t lastActivityTime;
};


#endif /* BFTPSERVER_H */
