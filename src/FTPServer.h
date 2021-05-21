/**
 * @file FTPServer.h
 * @author Pablo Pérez González (alu0101318318@ull.edu.es)
 * @brief Programa servidor del protocolo FTP con sockets
 * @version 0.1
 * @date 2021-05-21
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#if !defined FTPServer_H
#define FTPServer_H

#include <list>

#include "ClientConnection.h"

class FTPServer {
 public:
  explicit FTPServer(int port = 21);
  void run();
  void stop();

 private:
  int port;
  int msock;
  std::list<ClientConnection*> connection_list;
};

#endif
