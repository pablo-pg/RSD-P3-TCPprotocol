/**
 * @file ClientConnection.cpp
 * @author Pablo Pérez González (alu0101318318@ull.edu.es)
 * @brief Salida de error en la implementación de FTP.
 * @version 0.1
 * @date 2021-05-21
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef COMMON_H
#define COMMON_H

#include <cstdlib>

inline void errexit(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  exit(1);
}

#endif