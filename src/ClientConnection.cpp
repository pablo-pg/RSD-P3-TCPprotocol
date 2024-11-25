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
  int sock = s;

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
    if (COMMAND("USER")) {                       /// USER
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
    } else if (COMMAND("PORT")) {               /// PORT (args = HOST-PORT)
      fscanf(fd, "%s", arg);
      std::string buf;
      std::stringstream ss_arg(arg);
      int values[6];
      for (int i = 0; i < 6; ++i) {
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
    } else if (COMMAND("PASV")) {                   /// PASV
      uint16_t port;
      uint32_t ip;
      struct sockaddr_in pasvaddr;
      socklen_t len;
      int s;
      len = sizeof(pasvaddr);
      s = socket(AF_INET, SOCK_STREAM, 0);
      if (s < 0) {
        errexit("No se pudo crear el socket pasivo %s\n", strerror(errno));
      }
      getsockname(control_socket, (struct sockaddr*)&pasvaddr, &len);
      pasvaddr.sin_port = 0;
      if (bind(s, (struct sockaddr*)&pasvaddr, sizeof(pasvaddr)) < 0) {
        close(s);
        errexit("No se pudo hacer bind con el puerto (pasivo): %s\n",
                strerror(errno));
        s = -1;
      }
      if (listen(s, 5) < 0) {
        errexit("Fallo en el listen: %s:(\n", strerror(errno));
      }
      getsockname(s, (struct sockaddr*)&pasvaddr, &len);
      ip = ntohl(pasvaddr.sin_addr.s_addr);
      port = ntohs(pasvaddr.sin_port);
      std::cout << "Local bind: " << inet_ntoa(pasvaddr.sin_addr) << "-" << port
                << std::endl;
      data_socket = s;
      passive_ = 1;
      fprintf(fd, "227 Entering Passive Mode (%u,%u,%u,%u,%u,%u)\n",
              (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff,
              (port >> 8) & 0xff, port & 0xff);
    } else if (COMMAND("STOR")) {                   /// STOR (args = FILE)
      fscanf(fd, "%s", arg);
      if (passive_) {
        struct sockaddr_in fsin;
        socklen_t alen;
        alen = sizeof(fsin);
        data_socket = accept(data_socket, (struct sockaddr*)&fsin, &alen);
      }
      char buf[MAX_BUFF];
      int stor_fd;
      int data;
      bool ok = 1;  //< Comprobación de transmisión.
      FILE* fdata;
      fdata = fdopen(data_socket, "a+");
      std::cout << "FILE TO PUT: " << arg << std::endl;
      stor_fd = open(arg, O_RDWR | O_CREAT, S_IRWXU);
      if (stor_fd < 0) {
        std::cout << "Error al abrir el fichero" << std::endl;
        fprintf(fd, "550 open error\n");
      }
      fprintf(fd, "150 File creation okay; about to open data connection\n");
      fflush(fd);
      do {
        data = read(data_socket, buf, sizeof(buf));
        std::cout << data << " bytes leídos" << std::endl;
        if (data < 0) {
          if (errno == EINTR) {
            continue;
          } else {
            ok = 0;
          }
        }
        if (write(stor_fd, buf, data) < 0) {
          ok = 0;
          fprintf(fd,
                  "451 Requested action aborted. Local error in processing.\n");
          errexit("Error al escribir el fichero %s\n", errno);
          break;
        }
      } while (data > 0);
      fclose(fdata);
      close(stor_fd);
      close(data_socket);
      fflush(fd);
      if (ok) {
        fprintf(fd, "226 Closing data conection\n");
        fflush(fd);
      } else {
        fprintf(fd, "550 file error.\n");
        fflush(fd);
      }
      fflush(fd);
    } else if (COMMAND("RETR")) {                   /// RETR (args = FILE)
      fscanf(fd, "%s", arg);
      FILE* fdata;
      if (passive_) {
        struct sockaddr_in fsin;
        socklen_t alen;
        alen = sizeof(fsin);
        data_socket = accept(data_socket, (struct sockaddr*)&fsin, &alen);
      }
      char buf[MAX_BUFF];
      int retr_fd;
      int data;
      bool ok = 1;                        //< Comprobación de transmisión.
      fdata = fdopen(data_socket, "a+");  // a+ escribir por el final
      std::cout << "FILE TO GET: " << arg << std::endl;
      retr_fd = open(arg, O_RDWR, S_IRWXU);
      if (retr_fd < 0) {
        fprintf(fd, "550 open error\n");
        ok = 0;
        // std::cout << "Error al abrir el fichero" << std::endl;
        // errexit("Error al abrir el fichero: %d", errno);
      }
      fprintf(fd, "150 File status okay; about to open data connection\n");
      fflush(fd);
      do {
        data = read(retr_fd, buf, sizeof(buf));
        std::cout << data << " bytes leídos" << std::endl;
        if (data < 0) {
          if (errno == EINTR) {
            continue;
          } else {
            ok = 0;
          }
        } else {
          if (write(data_socket, buf, data) < 0) {
            ok = 0;
            fprintf(
                fd,
                "451 Requested action aborted. Local error in processing.\n");
            errexit("Error al escribir el fichero %s\n", errno);
            break;
          }
        }
      } while (data > 0);
      // close(data_socket);
      fclose(fdata);
      if (close(retr_fd) < 0) {
        // errexit("Error closing the file.", errno);
        std::cout << "No se pudo cerrar el socket del fichero." << std::endl;
        close(data_socket);
      }
      fflush(fd);
      if (ok) {
        fprintf(fd, "226 Closing data connection.\n");
        fflush(fd);
      } else {
        fprintf(fd, "550 file error.\n");
        fflush(fd);
      }
      fflush(fd);
    } else if (COMMAND("LIST")) {                     /// LIST
      fprintf(fd, "125 List started OK.\n");
      if (passive_) {
        struct sockaddr_in fsin;
        socklen_t alen;
        alen = sizeof(fsin);
        data_socket = accept(data_socket, (struct sockaddr*)&fsin, &alen);
      }
      FILE* fdata;
      fdata = fdopen(data_socket, "a+");  // a+ escribir por el final
      std::string str_curdir;
      char curdir[200];
      getcwd(curdir, sizeof(curdir));
      std::cout << "LS DE: " << curdir << std::endl;
      str_curdir = curdir;
      fcntl(data_socket, F_SETFL, O_NONBLOCK);
      for (const auto& file : fs::directory_iterator(str_curdir)) {
        fprintf(fdata, "%s\r\n", file.path().filename().string().data());
        fflush(fd);
        fflush(fdata);
      }
      fclose(fdata);
      // close(data_socket);
      fprintf(fd, "250 List completed successfully.\n");
      fflush(fd);
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
