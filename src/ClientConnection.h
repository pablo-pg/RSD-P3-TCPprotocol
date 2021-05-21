/**
 * @file ClientConnection.cpp
 * @author Pablo Pérez González (alu0101318318@ull.edu.es)
 * @brief Programa cliente del protocolo FTP con sockets
 * @version 0.1
 * @date 2021-05-21
 *
 * @copyright Copyright (c) 2021
 *
 */

#if !defined ClientConnection_H
#define ClientConnection_H

#include <pthread.h>

#include <cstdint>
#include <cstdio>
#include <experimental/filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
namespace fs = std::experimental::filesystem;

const int MAX_BUFF = 1000;

class ClientConnection {
 public:
  explicit ClientConnection(int s);
  ~ClientConnection();

  void WaitForRequests();
  void stop();

 private:
  bool ok;  // This variable is a flag that avois that the
            // server listens if initialization errors occured.

  FILE *fd;  // C file descriptor. We use it to buffer the
             // control connection of the socket and it allows to
             // manage it as a C file using fprintf, fscanf, etc.

  char command[MAX_BUFF];  // Buffer for saving the command.
  char arg[MAX_BUFF];      // Buffer for saving the arguments.

  int data_socket;     // Data socket descriptor;
  int control_socket;  // Control socket descriptor;
  bool parar;
};

#endif
