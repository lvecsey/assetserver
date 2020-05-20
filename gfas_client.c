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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "platendian.h"

#include <string.h>

#include <errno.h>

#include "gfas_client.h"

#include "fill_address.h"

#include "readfile.h"

#include "writefile.h"

#include "assetheader.h"

#include "assetresponse.h"

int gfas_setup(gfas_client *gfasc, char *serverip_port) {

  struct protoent *pent;

  socklen_t addrlen;

  int retval;
  
  retval = fill_address(serverip_port, &(gfasc->server_addr));
  if (retval == -1) {
    printf("Trouble with call to fill_address.\n");
    return -1;
  }

  memset(&(gfasc->bind_addr), 0, sizeof(struct sockaddr_in));
  gfasc->bind_addr.sin_family = AF_INET;
  gfasc->bind_addr.sin_port = htons(0);
  
  pent = getprotobyname("TCP");
  if (pent == NULL) {
    perror("getprotobyname");
    return -1;
  }

  gfasc->s = socket(AF_INET, SOCK_STREAM, pent->p_proto); 
  if (gfasc->s == -1) {
    perror("socket");
    return -1;
  }

  addrlen = sizeof(struct sockaddr_in);
  
  retval = bind(gfasc->s, (const struct sockaddr*) &(gfasc->bind_addr), addrlen);
  if (retval == -1) {
    perror("bind");
    fprintf(stderr, "%s: errno %d\n", __FUNCTION__, errno);
    return -1;
  }

  retval = connect(gfasc->s, (const struct sockaddr*) &(gfasc->server_addr), addrlen);
  if (retval == -1) {
    perror("connect");
    return -1;
  }
  
  return 0;
  
}

int gfas_fetch(gfas_client *gfasc, char *filename, gfas_fileprep *prep) {

  assetheader ah;
  
  uint64_t val;

  size_t len;

  ssize_t bytes_written;

  ssize_t bytes_read;

  int in_fd, out_fd;

  int retval;
  
  if (filename == NULL) {
    printf("Please specify filename to retrieve.\n");
    return -1;
  }
  
  len = strlen(filename);

  if (len >= sizeof(ah.filename)) {
    printf("File name too long.\n");
    return -1;
  }

  ah.filename_len = len;

  in_fd = gfasc->s;
  out_fd = gfasc->s;

  memcpy(ah.filename, filename, len);
  ah.filename[len] = 0;
  
  val = htobe64(ah.filename_len);
  bytes_written = writefile(out_fd, &val, sizeof(uint64_t));
  if (bytes_written != sizeof(uint64_t)) {
    perror("write");
    return -1;
  }

  bytes_written = writefile(out_fd, ah.filename, ah.filename_len);
  if (bytes_written != ah.filename_len) {
    perror("write");
    return -1;
  }

  retval = readfile(in_fd, &val, sizeof(uint64_t));
  if (retval != sizeof(uint64_t)) {
    perror("read");
    return -1;
  }

  val = be64toh(val);
  
  switch(val) {

  case AS_OK:

    break;

  case AS_LENFAIL:
  case AS_OPENFAIL:
  case AS_STATFAIL:
    printf("AS_FAIL.\n");
    return -1;
    
  }

  {

    size_t remaining;

    unsigned char *buf;

    bytes_read = read(in_fd, &val, sizeof(uint64_t));
    if (bytes_read != sizeof(uint64_t)) {
      perror("read");
      return -1;
    }

    remaining = be64toh(val);

    prep->len = remaining;

    prep->data = malloc(prep->len);
    if (prep->data == NULL) {
      perror("malloc");
      return -1;
    }

    buf = prep->data;
    
    while (remaining > 0) {

      bytes_read = read(in_fd, buf, 4096);

      if (bytes_read == -1) {
	perror("read");
	return -1;
      }

      remaining -= bytes_read;

      buf += bytes_read;
      
    }

    val = htobe64(AS_QUIT);
    bytes_written = writefile(out_fd, &val, sizeof(uint64_t));
    if (bytes_written != sizeof(uint64_t)) {
      perror("write");
      return -1;
    }
    
  }
  
  return 0;

}
  
