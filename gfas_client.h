/*
    TCP client for retrieving a file from an assetserver
    Copyright (C) 2020  Lester Vecsey

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GFAS_CLIENT_H
#define GFAS_CLIENT_H

#include "networking.h"

#include <stdint.h>

typedef struct {

  uint64_t state;
  
  int s;
  
  struct sockaddr_in bind_addr, server_addr;
  
} gfas_client;

typedef struct {

  size_t len;

  void *data;
  
} gfas_fileprep;

int gfas_setup(gfas_client *gfasc, char *serverip_port);

int gfas_fetch(gfas_client *gfasc, char *filename, gfas_fileprep *prep);

#endif
