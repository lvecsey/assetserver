/*
    Fill an IP Address and Port string into a sockaddr_in structure.
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

#include <stdio.h>
#include <stdint.h>

#include "networking.h"

#include "platendian.h"

#include <string.h>

#include "fill_address.h"

int fill_address(char *ipaddress_port, struct sockaddr_in *addr) {

  long int input_ipaddress[4];
  long int input_port;

  int retval;

  retval = sscanf(ipaddress_port, "%ld.%ld.%ld.%ld:%ld", input_ipaddress+0, input_ipaddress+1, input_ipaddress+2, input_ipaddress+3, &input_port);

  if (retval != 5) {
    return -1;
  }

  {
    unsigned char ip_address[4];
    uint16_t port;

    ip_address[0] = input_ipaddress[0];
    ip_address[1] = input_ipaddress[1];
    ip_address[2] = input_ipaddress[2];
    ip_address[3] = input_ipaddress[3];    
    port = input_port;

    memset(addr, 0, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    
    memcpy(&(addr->sin_addr.s_addr), ip_address, 4);
    addr->sin_port = be16toh(port);

  }
    
  return 0;
  
}
