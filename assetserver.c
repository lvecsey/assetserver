/*
    assetserver to send files over TCP with tcpserver
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

#include <endian.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>
#include <errno.h>

#include <sys/sendfile.h>

#include "assetheader.h"

#include "assetresponse.h"

#include "readfile.h"

#include "writefile.h"

#define PATH_MAX 256

int send_response(uint64_t response) {

  uint64_t ret;
  
  ssize_t bytes_written;

  ret = htobe64(response);
  
  bytes_written = writefile(1, &ret, sizeof(uint64_t));
  if (bytes_written != sizeof(uint64_t)) {
    return -1;
  }
  
  return 0;
  
}

int main(int argc, char *argv[]) {

  char *env_PROTO;

  assetheader ah;

  int retval;

  uint64_t val;

  char *root_path;

  ssize_t bytes_written;
  
  env_PROTO = getenv("PROTO");
  if (env_PROTO == NULL) {
    printf("Please run from tcpserver.\n");
    return 0;
  }

  root_path = argc>1 ? argv[1] : NULL;

  printf("Getting filename length.\n");
  
  retval = readfile(0, &val, sizeof(uint64_t));
  if (retval != sizeof(uint64_t)) {
    return -1;
  }

  ah.filename_len = be64toh(val);

  memset(ah.filename, 0, sizeof(ah.filename));

  if (ah.filename_len < sizeof(ah.filename)) {
  
    retval = readfile(0, ah.filename, ah.filename_len);
    if (retval != ah.filename_len) {

      send_response(AS_LENFAIL);
      
      return -1;

    }

    {

      char strbuf[PATH_MAX];

      char *fn_ptr;
      
      struct stat buf;
      int fd;

      if (root_path != NULL) {

	retval = sprintf(strbuf, "%s/%s", root_path, ah.filename);
	
	fn_ptr = strbuf;

      }
      else {
	fn_ptr = ah.filename;
      }

      fd = open(fn_ptr, O_RDONLY);
      if (fd == -1) {

	send_response(AS_OPENFAIL);

	return -1;
	
      }

      retval = fstat(fd, &buf);
      if (retval == -1) {

	send_response(AS_STATFAIL);

	return -1;
	
      }

      send_response(AS_OK);

      {

	off_t offset;

	size_t remaining;
	
	offset = 0;

	{
	  uint64_t sz;
	  sz = buf.st_size;
	  val = htobe64(sz);
	  bytes_written = writefile(1, &val, sizeof(uint64_t));
	  if (bytes_written != sizeof(uint64_t)) {
	    return -1;
	  }
	}
	  
	remaining = buf.st_size;

	while (remaining > 0) {
	
	  retval = sendfile(1, fd, &offset, remaining);

	  if (retval == -1) {
	    return -1;
	  }
	  
	  remaining -= retval;
	  
	}
	  

      }

    }
      
  }
    
  return 0;

}
