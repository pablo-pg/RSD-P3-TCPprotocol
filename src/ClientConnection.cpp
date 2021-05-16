//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//
//                     2º de grado de Ingeniería Informática
//
//              This class processes an FTP transaction.
//
//****************************************************************************

#include "ClientConnection.h"

#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <langinfo.h>
#include <locale.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pwd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <iostream>

#include "./common.h"

ClientConnection::ClientConnection(int s) {
  int sock = (int)(s);

  char buffer[MAX_BUFF];

  control_socket = s;
  // Check the Linux man pages to know what fdopen does.
  fd = fdopen(s, "a+");
  if (fd == NULL) {
    std::cout << "Connection closed" << std::endl;

    fclose(fd);
    close(control_socket);
    ok = false;
    return;
  }

  ok = true;
  data_socket = -1;
  parar = false;
}

ClientConnection::~ClientConnection() {
  fclose(fd);
  close(control_socket);
}

int connect_TCP(uint32_t address, uint16_t port) {
  // Implement your code to define a socket here
  struct sockaddr_in sin;
  int s;

  memset(&sin, 0, sizeof(sin));
  sin.sin_port = htons(port);
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = address;

  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    errexit("No se pudo crear el socket: %s\n", strerror(errno));
  }
  if (connect(s, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
    errexit("No se pudo conectar con %s: %s\n", address, strerror(errno));
  }
  return s;  // You must return the socket descriptor.
}

void ClientConnection::stop() {
  close(data_socket);
  close(control_socket);
  parar = true;
}

#define COMMAND(cmd) strcmp(command, cmd) == 0

// This method processes the requests.
// Here you should implement the actions related to the FTP commands.
// See the example for the USER command.
// If you think that you have to add other commands feel free to do so. You
// are allowed to add auxiliary methods if necessary.

void ClientConnection::WaitForRequests() {
  if (!ok) {
    return;
  }
  fprintf(fd, "220 Service ready\n");
  while (!parar) {
    fscanf(fd, "%s", command);
    if (COMMAND("USER")) {      /// USER
      fscanf(fd, "%s", arg);
      fprintf(fd, "331 User name ok, need password\n");
    } else if (COMMAND("PWD")) {
    } else if (COMMAND("PASS")) {
      fscanf(fd, "%s", arg);
      if (strcmp(arg, "1234") == 0) {
        fprintf(fd, "230 User logged in\n");
      } else {
        fprintf(fd, "530 Not logged in.\n");
        parar = true;
      }

    } else if (COMMAND("PORT")) {   /// PORT (args = HOST-PORT)
      fscanf(fd, "%s", arg);
      std::string buf;
      std::stringstream ss_arg(arg);
      int values[6];
      for (int  i = 0; i < 6; ++i) {
        std::getline(ss_arg, buf, ',');
        values[i] = std::stoi(buf);
      }
      std::stringstream ss_port;
      ss_port << std::hex << values[4] << values[5];
      // std::cout << "Puerto (hex): " << ss_port.str()  << std::endl;
      uint16_t port;
      ss_port >> std::hex >> port;
      // std::cout << "Puerto (dec): " << port << std::endl;

      std::stringstream ss_ip;
      ss_ip << values[0] << "." << values[1] << "." << values[2] << "."
            << values[3];
      // std::cout << "IP (str) = " << ss_ip.str() << std::endl;
      uint32_t ip;
      inet_pton(AF_INET, ss_ip.str().c_str(), &ip);
      std::cout << "IP: " << ip << std::endl;
      std::cout << "PORT: " << port << std::endl;

      data_socket = connect_TCP(ip, port);
      FILE* fdata;
      fdata = fdopen(data_socket, "a+");  // a+ escribir por el final
        if (fdata == nullptr) {
          std::cout << "Connection closed" << std::endl;
          fclose(fdata);
          close(data_socket);
          fprintf(fd, "425 Connection failed\n");
          ok = false;
        }
      fprintf(fd, "200 PORT OK.\n");
    } else if (COMMAND("PASV")) {
      // To be implemented by students
    } else if (COMMAND("STOR")) {
      // To be implemented by students
    } else if (COMMAND("RETR")) {
      // To be implemented by students
    } else if (COMMAND("LIST")) {
      // To be implemented by students
    } else if (COMMAND("SYST")) {
      fprintf(fd, "215 UNIX Type: L8.\n");
    } else if (COMMAND("TYPE")) {
      fscanf(fd, "%s", arg);
      fprintf(fd, "200 OK\n");
    } else if (COMMAND("QUIT")) {
      fprintf(fd,
              "221 Service closing control connection. Logged out if "
              "appropriate.\n");
      close(data_socket);
      parar = true;
      break;
    } else {
      fprintf(fd, "502 Command not implemented.\n");
      fflush(fd);
      printf("Comando : %s %s\n", command, arg);
      printf("Error interno del servidor\n");
    }
  }

  fclose(fd);

  return;
}
