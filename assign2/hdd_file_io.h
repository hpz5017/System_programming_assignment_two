#ifndef HDD_FILE_IO_INCLUDED
#define HDD_FILE_IO_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File           : crud_file_io.h
//  Description    : This is the header file for the standardized IO functions
//                   for used to access the CRUD storage system.
//
//  Author         : Patrick McDaniel
//  Last Modified  : Sat Sep 2nd 08:56:10 EDT 2017
//
////////////////////////////////////////////////////////////////////////////////
//
// STUDENTS MUST ADD COMMENTS BELOW
//


// Include files
#include <stdint.h>

//
// Interface functions

int16_t hdd_open(char *path);
	// This function opens the file and returns a file handle

int16_t hdd_close(int16_t fd);
	// This function closes the file

int32_t hdd_read(int16_t fd, void *buf, int32_t count);
	// Reads "count" bytes from the file handle "fh" into the buffer  "buf"

int32_t hdd_write(int16_t fd, void *buf, int32_t count);
	// Writes "count" bytes to the file handle "fh" from the buffer  "buf"

int32_t hdd_seek(int16_t fd, uint32_t loc);
	// Seek to specific point in the file

//
// Unit testing for the module

int hddIOUnitTest(void);
	// Perform a test of the CRUD IO implementation

#endif


