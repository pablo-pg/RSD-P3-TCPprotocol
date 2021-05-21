/**
 * @file ftp_server.cpp
 * @author Pablo Pérez González (alu0101318318@ull.edu.es)
 * @brief Programa que simula un servidor TCP para el protocolo FTP
 * @version 0.1
 * @date 2021-05-21
 *
 * @copyright Copyright (c) 2021
 *
 */

#include <signal.h>

#include <iostream>

#include "FTPServer.h"

FTPServer *server;

extern "C" void sighandler(int signal, siginfo_t *info, void *ptr) {
  std::cout << "Dispara sigaction" << std::endl;
  server->stop();
  exit(-1);
}

void exit_handler() { server->stop(); }

int main(int argc, char **argv) {
  struct sigaction action;
  action.sa_sigaction = sighandler;
  action.sa_flags = SA_SIGINFO;
  sigaction(SIGINT, &action, NULL);
  server = new FTPServer(2121);
  atexit(exit_handler);
  server->run();
}
