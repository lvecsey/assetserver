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

#ifdef WINNT
#include "recvfile.h"
#include "sendfile.h"
#endif

#include "readfile.h"

#include "writefile.h"

#include "assetheader.h"

#include "assetrequest.h"

#include "assetresponse.h"

int gfas_setup(gfas_client *gfasc, char *serverip_port) {

  struct protoent *pent;

  socklen_t addrlen;

  int retval;

#ifndef ANDROID  
  pent = getprotobyname("TCP");
  if (pent == NULL) {
    perror("getprotobyname");
    return -1;
  }
#endif
  
#ifdef ANDROID
  gfasc->s = socket(PF_INET, SOCK_STREAM, 6);
#else
  gfasc->s = socket(AF_INET, SOCK_STREAM, pent->p_proto); 
#endif
  
  if (gfasc->s == -1) {
    perror("socket");
    return -1;
  }

  retval = fill_address(serverip_port, &(gfasc->server_addr));
  if (retval == -1) {
    printf("Trouble with call to fill_address.\n");
    return -1;
  }

  memset(&(gfasc->bind_addr), 0, sizeof(struct sockaddr_in));
  gfasc->bind_addr.sin_family = AF_INET;
  gfasc->bind_addr.sin_port = htons(0);
  
  addrlen = sizeof(struct sockaddr_in);
  
  retval = bind(gfasc->s, (const struct sockaddr*) &(gfasc->bind_addr), addrlen);
  if (retval == -1) {
    perror("bind");
    fprintf(stderr, "%s: errno %d\n", __FUNCTION__, errno);
    return -1;
  }
  
  return 0;
  
}

int gfas_fetch(gfas_client *gfasc, char *filename, gfas_fileprep *prep) {

  assetheader ah;

  uint64_t cmd;
  
  uint64_t val;

  size_t len;

  ssize_t bytes_written;

  ssize_t bytes_read;

#ifdef WINNT
  SOCKET in_s, out_s;
#else
  int in_fd, out_fd;
#endif
  
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

#ifdef WINNT
  in_s = gfasc->s;
  out_s = gfasc->s;
#else
  in_fd = gfasc->s;
  out_fd = gfasc->s;
#endif

  cmd = ASR_GETFILE;

  val = htobe64(cmd);
#ifdef WINNT
  bytes_written = sendfile(out_s, &val, sizeof(uint64_t));
#else
  bytes_written = writefile(out_fd, &val, sizeof(uint64_t));
#endif
  if (bytes_written != sizeof(uint64_t)) {
    perror("ah.filename_len write");
    return -1;
  }
  
  memcpy(ah.filename, filename, len);
  ah.filename[len] = 0;
  
  val = htobe64(ah.filename_len);
#ifdef WINNT
  bytes_written = sendfile(out_s, &val, sizeof(uint64_t));
#else
  bytes_written = writefile(out_fd, &val, sizeof(uint64_t));
#endif
  if (bytes_written != sizeof(uint64_t)) {
    perror("ah.filename_len write");
    return -1;
  }

#ifdef WINNT
  bytes_written = sendfile(out_s, ah.filename, ah.filename_len);
#else
  bytes_written = writefile(out_fd, ah.filename, ah.filename_len);
#endif
  if (bytes_written != ah.filename_len) {
    perror("ah.filename write");
    return -1;
  }

#ifdef WINNT
  retval = recvfile(in_s, &val, sizeof(uint64_t));
#else
  retval = readfile(in_fd, &val, sizeof(uint64_t));
#endif
  if (retval != sizeof(uint64_t)) {
    perror("read");
    return -1;
  }

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

#ifdef WINNT
    bytes_read = recvfile(in_s, &val, sizeof(uint64_t));
#else    
    bytes_read = read(in_fd, &val, sizeof(uint64_t));
#endif
    if (bytes_read != sizeof(uint64_t)) {
      perror("read");
      return -1;
    }

#if _BYTE_ORDER == _LITTLE_ENDIAN    
    remaining = be64toh(val);
    remaining = htole64(remaining);
#else
    remaining = be64toh(val);
#endif
    
    prep->len = remaining;

    prep->data = malloc(prep->len);
    if (prep->data == NULL) {
      perror("malloc");
      return -1;
    }

    buf = prep->data;

#ifdef WINNT
    bytes_read = recvfile(in_s, buf, remaining);
#else
    bytes_read = readfile(in_fd, buf, remaining);
#endif
    
    if (bytes_read == -1) {
      perror("read");
      return -1;
    }

  }

  return 0;

}

int ghas_fillhash(gfas_client *gfasc, char *filename, unsigned char *hashbuf, size_t buf_sz) {

  assetheader ah;

  uint64_t cmd;
  
  uint64_t val;

  size_t len;

  ssize_t bytes_written;

  ssize_t bytes_read;

#ifdef WINNT
  SOCKET in_s, out_s;
#else
  int in_fd, out_fd;
#endif
  
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

#ifdef WINNT
  in_s = gfasc->s;
  out_s = gfasc->s;
#else
  in_fd = gfasc->s;
  out_fd = gfasc->s;
#endif

  cmd = ASR_GETHASH;

  val = htobe64(cmd);
#ifdef WINNT
  bytes_written = sendfile(out_s, &val, sizeof(uint64_t));
#else
  bytes_written = writefile(out_fd, &val, sizeof(uint64_t));
#endif
  if (bytes_written != sizeof(uint64_t)) {
    perror("ah.filename_len write");
    return -1;
  }
  
  memcpy(ah.filename, filename, len);
  ah.filename[len] = 0;
  
  val = htobe64(ah.filename_len);
#ifdef WINNT
  bytes_written = sendfile(out_s, &val, sizeof(uint64_t));
#else
  bytes_written = writefile(out_fd, &val, sizeof(uint64_t));
#endif
  if (bytes_written != sizeof(uint64_t)) {
    perror("ah.filename_len write");
    return -1;
  }

#ifdef WINNT
  bytes_written = sendfile(out_s, ah.filename, ah.filename_len);
#else
  bytes_written = writefile(out_fd, ah.filename, ah.filename_len);
#endif
  if (bytes_written != ah.filename_len) {
    perror("ah.filename write");
    return -1;
  }

#ifdef WINNT
  retval = recvfile(in_s, &val, sizeof(uint64_t));
#else
  retval = readfile(in_fd, &val, sizeof(uint64_t));
#endif
  if (retval != sizeof(uint64_t)) {
    perror("read");
    return -1;
  }

  switch(val) {

  case AS_OK:

    break;

  case AS_LENFAIL:
  case AS_OPENFAIL:
  case AS_STATFAIL:
  case AS_MMAPFAIL:
    printf("AS_FAIL.\n");
    return -1;
    
  }

  {

#ifdef WINNT
    bytes_read = recvfile(in_s, hashbuf, buf_sz);
#else    
    bytes_read = read(in_fd, hashbuf, buf_sz);
#endif
    if (bytes_read != buf_sz) {
      perror("read");
      return -1;
    }

  }

  return 0;

}

int gfas_doconnect(gfas_client *gfasc) {

  socklen_t addrlen;

  int retval;
  
  addrlen = sizeof(struct sockaddr_in);
  
  retval = connect(gfasc->s, (const struct sockaddr*) &(gfasc->server_addr), addrlen);
  if (retval == -1) {
    perror("connect");
    return -1;
  }

  return 0;

}

int gfas_sendquit(gfas_client *gfasc) {

  uint64_t cmd;

  uint64_t val;

  ssize_t bytes_written;

#ifdef WINNT
  SOCKET out_s;
#else
  int out_fd;
#endif

#ifdef WINNT
  out_s = gfasc->s;
#else
  out_fd = gfasc->s;
#endif

  cmd = AS_QUIT;

  val = htobe64(cmd);
#ifdef WINNT
  bytes_written = sendfile(out_s, &val, sizeof(uint64_t));
#else
  bytes_written = writefile(out_fd, &val, sizeof(uint64_t));
#endif
  if (bytes_written != sizeof(uint64_t)) {
    perror("quit write");
    return -1;
  }
  
  return 0;

}

char *hashtostr(unsigned char *hashbuf) {

  static char strout[33];

  long int hashno;

  char *wptr;

  wptr = strout;
  
  for (hashno = 0; hashno < 16; hashno++) {

    sprintf((char*) wptr, "%02x", hashbuf[hashno]);
    wptr += 2;

  }

  strout[32] = 0;
  
  return strout;
  
}

int gfas_cacheretrieve(gfas_client *gfasc, char *assetserver_ipportstr, char *request_str, gfas_fileprep *prep, char *cachedir, mode_t create_filemode) {

  unsigned char hashbuf[16];

  int retval;
  
  retval = gfas_setup(gfasc, assetserver_ipportstr);
  if (retval == -1) {
    printf("Trouble setting up gfas client.\n");
    return -1;
  }

  retval = gfas_doconnect(gfasc);
  if (retval == -1) {
    printf("Trouble connecting to assetserver.\n");
    return -1;
  }
    
  retval = ghas_fillhash(gfasc, request_str, hashbuf, sizeof(hashbuf));

  if (retval == -1) {
    printf("Trouble with call to ghas_fillhash.\n");
    return -1;
  }

  {

    char strbuf[240];

    int fd;

    retval = sprintf(strbuf, "%s/%s", cachedir, hashtostr(hashbuf));

    printf("Checking cache file %s\n", strbuf);
      
    fd = open(strbuf, O_RDONLY);

    if (fd != -1) {

      struct stat buf;
	
      ssize_t bytes_read;

      retval = fstat(fd, &buf);
      if (retval == -1) {
	perror("fstat");
	return -1;
      }

      prep->data = malloc(buf.st_size);
      if (prep->data == NULL) {
	perror("malloc");
	return -1;
      }

      bytes_read = readfile(fd, prep->data, buf.st_size);
      if (bytes_read != buf.st_size) {
	perror("read");
	return -1;
      }

      prep->len = buf.st_size;
	
      retval = close(fd);
      if (retval == -1) {
	perror("close");
	return -1;
      }
	
    }
    else {

      ssize_t bytes_written;
      
      fd = open(strbuf, O_WRONLY | O_TRUNC | O_CREAT, create_filemode);

      retval = gfas_fetch(gfasc, request_str, prep);

      if (retval == -1) {
	printf("Trouble with call to gfas_fetch.\n");
	printf("FAIL");
	return -1;
      }

#ifndef ANDROID
      printf("Received file in memory size %lu\n", prep->len);
#endif
	
      bytes_written = writefile(fd, prep->data, prep->len);
      if (bytes_written != prep->len) {
	perror("write");
	return -1;
      }

      retval = close(fd);
      if (retval == -1) {
	perror("close");
	return -1;
      }

    }

  }
    
  retval = gfas_sendquit(gfasc);
  if (retval == -1) {
    printf("Trouble sending quit command to assetserver.\n");
    return -1;
  }

  return 0;

}
