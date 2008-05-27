// BTCPConnection.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BTCPCONNECTION_H
#define BTCPCONNECTION_H

/*!
  \file BTCPConnection.h

  Contains the interface for the TCP connection superclass
*/

/* system includes */
#include <string>
#include <deque>
#include <netinet/in.h>

/* my includes */
/* (none) */

/*!
  \class BTCPConnection

  Superclass for TCP connections
*/
class BTCPConnection {
public:
  //! TCP client constructor -- connect to addr, port
  BTCPConnection(long addr, short port);
  
  //! TCP client constructor -- connect to host, port
  BTCPConnection(const std::string& host, short port);
  
  //! TCP server constructor -- listen on port
  BTCPConnection(short port);
  
  //! TCP server constructor -- don't listen at all (call bind() later)
  BTCPConnection();
  
  virtual ~BTCPConnection();

  //! Bind to the given port (if not already bound)
  void bind(short port);
  //! Return the port the connection is bound to
  short portNumber() { return portnum; }

  //! Handle incoming connections and connection attempts. Call periodically.
  virtual void handle();
  //! Close connection.
  virtual void close();

  //! Return true if connected
  bool connected() { return conn != -1; }

  //! Send the buffer over the connection. Return the number of bytes sent.
  int writeRaw(const unsigned char* buf, int buflen);
  //! Send the buffer over the connection, making sure everything gets sent.
  bool write(const unsigned char* buf, int buflen = -1);
  //! Send the buffer over the connection, making sure everything gets sent.
  bool write(const char* buf, int buflen = -1)
  {
    return write((const unsigned char*)buf, buflen);
  }
  //! Read from the connection.
  int read(char* buf, int buflen);

  //! Make the connection blocking. Subsequent calls to writeRaw() may block.
  bool makeBlocking();
  //! Make the connection nonblocking. Calls to writeRaw() may return 0.
  bool makeNonblocking();

protected:
  //! Called if the remote closes the connection
  virtual void handleClose();
  //! Called by handle() if connected. Read and write in here
  virtual bool handleConnection();
  //! Called by the default handleConnection() if input was received
  virtual bool handleInput(const unsigned char* buf, int bufleno);
  //! Called by handle() if not connected. Accept in here
  virtual bool handleAccept();

  //! Return the size of the read block (buf)
  unsigned int readBlockSize();
  //! Set the size of the read block
  void setReadBlockSize(unsigned int rbs);

  //! The port the connection is listening on, or -1 if it's not
  short portnum;
  //! The server socket
  int sock;
  //! The connection socket
  int conn;
  //! If this is true, accept()ed connections are nonblocking by default
  bool doNonblocking;
  //! True if this connection is a server
  bool isServer;

  //! Size of the read block (buf)
  unsigned int rbs;
  //! The read block. Use this for data transfer
  unsigned char* buf;

  //! The address of the last connection partner
  struct sockaddr_in lastaddr;

private:
  typedef struct {
    int socket;
    unsigned int timeout;
  } TimedSocketCloser;
  static std::deque<TimedSocketCloser> socketsToClose;
  static void reapSockets();
};

#endif /* BTCPCONNECTION_H */
