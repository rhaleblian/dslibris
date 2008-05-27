// BFTPConfigurator.h (this is -*- C++ -*-)
// 
// \author: Bjoern Giesler <bjoern@giesler.de>
// 
// 
// 
// $Author: giesler $
// $Locker$
// $Revision$
// $Date: 2002-08-19 10:41:28 +0200 (Mon, 19 Aug 2002) $

#ifndef BFTPCONFIGURATOR_H
#define BFTPCONFIGURATOR_H

/*!
  \file BFTPConfigurator.h

  Contains a class that configures an FTP server by reading config
  statements from a file.
*/

/* system includes */
#include <string>

/* my includes */
#include "BFTPServer.h"

/*!
  \brief Configurator for FTP servers

  This class will read statements from a config file and use them to
  configure a FTP server instance.
*/
class BFTPConfigurator {
public:
  //! Create a new configurator that will configure the given server.
  BFTPConfigurator(BFTPServer* server);
  /*!
    \brief Configure the server by reading statements from a file.

    This will return false only if the file could not be
    opened. Error messages will be output via printf(), but do not lead
    to abortion.
  */
  bool configureFromFile(const std::string& filename);

private:
  BFTPServer* server;
};

#endif /* BFTPCONFIGURATOR_H */
