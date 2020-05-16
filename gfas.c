/*
    Retrieve a file with tcpclient from an assetserver
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

#include <endian.h>
#include <errno.h>
#include <string.h>

#include <libgen.h>

#include <sys/sendfile.h>

#include "assetheader.h"

#include "assetresponse.h"

#include "readfile.h"

#include "writefile.h"

int main(int argc, char *argv[]) {

  assetheader ah;
  
  uint64_t val;

  char *filename;

  size_t len;

  ssize_t bytes_written;

  ssize_t bytes_read;

  int in_fd, out_fd;

  int verbose;

  int retval;
  
  verbose = getenv("VERBOSE") != NULL ? 1 : 0;
  
  filename = argc>1 ? argv[1] : NULL;

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

  in_fd = 6;
  out_fd = 7;

  memcpy(ah.filename, filename, len);
  ah.filename[len] = 0;
  
  if (verbose) {
    printf("Requesting filename %s.\n", ah.filename);
  }
    
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


    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    char *bn;
    
    int fd;

    size_t remaining;

    unsigned char *buf;

    buf = malloc(4096);
    if (buf==NULL) {
      perror("malloc");
      return -1;
    }
    
    bytes_read = read(in_fd, &val, sizeof(uint64_t));
    if (bytes_read != sizeof(uint64_t)) {
      perror("read");
      return -1;
    }

    remaining = be64toh(val);

    if (verbose) {
      printf("Expecting file of length %lu\n", remaining);
    }
      
    bn = basename(ah.filename);

    if (verbose) {
      printf("Basename is %s\n", bn);
    }
      
    fd = open(bn, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (fd == -1) {
      perror("open");
      return -1;
    }

    offset = 0;
    
    while (remaining > 0) {

      bytes_read = read(in_fd, buf, 4096);

      if (bytes_read == -1) {
	perror("read");
	return -1;
      }

      bytes_written = writefile(fd, buf, bytes_read);
      if (bytes_written != bytes_read) {
	perror("write");
	return -1;
      }
      
      remaining -= bytes_read;

    }

    retval = close(fd);
    if (retval == -1) {
      perror("close");
      return -1;
    }

    free(buf);
    
  }
  
  
  return 0;

}
