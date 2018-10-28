////////////////////////////////////////////////////////////////////////////////
//
//  File           : hdd_file_io.c
//  Description    : This is the implementation of the standardized IO functions
//                   for used to access the HDD storage system.
//
//  Author         : Haohan Zhong
//  Last Modified  : Mon Oct 10th EDT 2017
//
////////////////////////////////////////////////////////////////////////////////
//
// STUDENTS MUST ADD COMMENTS BELOW and FILL INT THE CODE FUNCTIONS
//

// Includes
#include <malloc.h>
#include <string.h>

// Project Includes
#include <hdd_file_io.h>
#include <hdd_driver.h>
#include <hdd_driver.h>
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>


// Defines (you can ignore these)
#define MAX_HDD_FILEDESCR 1024
#define HIO_UNIT_TEST_MAX_WRITE_SIZE 1024
#define HDD_IO_UNIT_TEST_ITERATIONS 10240


// Type for UNIT test interface (you can ignore this)
typedef enum {
	HIO_UNIT_TEST_READ   = 0,
	HIO_UNIT_TEST_WRITE  = 1,
	HIO_UNIT_TEST_APPEND = 2,
	HIO_UNIT_TEST_SEEK   = 3,
} HDD_UNIT_TEST_TYPE;

//struct to store file information
typedef struct{
	char filename[1024];    //name of each file(path)
    int Open;        //open status, 0 for opened, 1 for closed
    int file_handle;   
    int  Block_ID;    //varaible to store the block ID which is corresponding to this file
    int position;     //current position in the file
}File;

//struct to construct a complete command
HddBitCmd cmd_constructor(int64_t Op,int64_t Block_size, int64_t Flags,int64_t R, int64_t Block_ID)
{
    HddBitCmd Constrcuted_cmd;
    //initial Constrcted command to all zeros
    Constrcuted_cmd=0;
    //shift bits to corresponding positions
    Op=Op<<62;
    Block_size=Block_size<<36;
    Flags=Flags<<33;
    R=R<<32;
    Constrcuted_cmd=Op|Block_size|Flags|R|Block_ID;

 return Constrcuted_cmd;
}

//struct to separate different parts from a command
typedef struct{
    int Op;
    int32_t Block_size;
    int32_t Flags;
    int32_t R;
    int64_t Block_ID;
}cmd_decon_type;

 cmd_decon_type cmd_deconstructor(HddBitResp response)
 {
    //deconstructed command
    cmd_decon_type decon_cmd;
   
    //use SHIFT and AND operators to extract specific parts from the whole command
    decon_cmd.Op=response>>62;                          
    decon_cmd.Block_size=(response>>36)&(0x03ffffff);
    decon_cmd.Flags=(response>>33)&(0x00000003);
    decon_cmd.R=(response>>32)&(0x00000001);
    decon_cmd.Block_ID=response&(0x00000000ffffffff);
   
    return decon_cmd;
 }


//an array used to store multiple files and using file handle to track each file
File files[1024];
//file hanle 
int fh;
//
// Implementation

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_open 
// Description  : function to creat a file
//
// Inputs       : path
// Outputs      : result of creat operation
//
int16_t hdd_open(char *path) {
    //variable to store initia result
    int result;
    //initialize file handle to 0
    fh=0;

	//do initialization
    result=hdd_initialize();
    //return -1 if fail
    if(result==-1)
    return -1;
  
    //loop through the file array to find an empty position to store file information
    while(strcmp(files[fh].filename,"")!=0&&fh<1024)
    {
      fh++;
    }

    //set file status to open and initiate other parameters of this file
    files[fh].Block_ID=0;
    files[fh].Open=1;
    files[fh].file_handle=fh;
    files[fh].position=0;
    strcpy(files[fh].filename,path);
    return fh;
    

}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_close
// Description  : function to close the file
//
// Inputs       : file handle 
// Outputs      : result of CLOSE operation
//
int16_t hdd_close(int16_t fh) {
    int delete_res;

    if(files[fh].Open==0)
      return -1;
   
    //delete block
    delete_res=hdd_delete_block(files[fh].Block_ID);

    if(delete_res==-1)
      return -1;

	//set open status to 0
    files[fh].Open=0;
	  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_read
// Description  : read bytes from file and save them to data buffer
//
// Inputs       : file handle, data buffer, count
// Outputs      : number of byte read
//
int32_t hdd_read(int16_t fh, void * data, int32_t count) {
    HddBitCmd constrcted_cmd;
    int block_size;
    cmd_decon_type decon_res;
	HddBitResp result;
    //get block size	
    block_size=hdd_read_block_size(files[fh].Block_ID);

    //buffer to temporarily store original content in block
    char* temp_buff_block_content=malloc(block_size);

    //construct creat command_write 
    constrcted_cmd=cmd_constructor(1,block_size,0,0,files[fh].Block_ID);
    //call hdd_data_lane to write block content into temp_buff_block_content
    result=hdd_data_lane(constrcted_cmd,temp_buff_block_content); 
    decon_res=cmd_deconstructor(result);

    if(decon_res.R!=0)
      return -1;
    
    if(files[fh].position>=block_size)
    { 	
      //no byte read
      return 0;
    }

	//read all the content behind current position
	if(files[fh].position+count>=block_size)
	{
	  //size to store length between current position and the end of block
      int size;
      size=block_size-files[fh].position;
      //copy content behind current position into data
      memcpy(data,temp_buff_block_content+files[fh].position,size);  
      
      free(temp_buff_block_content);

      files[fh].position=block_size;
      return size;
    }
	
	//read specific number of bytes behind current position
	if(files[fh].position+count<block_size)
	{
      //copy content behind current position into data
      memcpy(data,temp_buff_block_content+files[fh].position,count);  
      
      //free buffer
      free(temp_buff_block_content);
      
      //update position
      files[fh].position=files[fh].position+count;
      return count;

	}
  
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_writer
// Description  : write a count number of bytes at the current position in the file
//                associated with the file handle fh from the buffer data.
// Inputs       : count,file handle,data buffer
// Outputs      : number of bytes written to the buffer
//
int32_t hdd_write(int16_t fh, void *data, int32_t count) {
	HddBitResp result_creat;
    HddBitCmd constrcted_cmd;
    cmd_decon_type decon_res;
    int block_size;
    int block_size_new;
   
    //creat a block if there is no one exists
    if(files[fh].Block_ID==0)
    {
      //construct creat command 
      constrcted_cmd=cmd_constructor(0,count,0,0,0);
    
      //call hdd_data_lane to creat a block and get response
      result_creat=hdd_data_lane(constrcted_cmd, data);

      //get R value from response
      decon_res=cmd_deconstructor(result_creat);

      
      if(decon_res.R==0)
      {
        //update information related to this file
        files[fh].Block_ID=decon_res.Block_ID;
        files[fh].file_handle=fh;
        files[fh].position=count;	
        return count;
      }  
      else
        return -1;
    }

    //get the current block size
    block_size=hdd_read_block_size(files[fh].Block_ID);
    if(files[fh].position+count>block_size)
    { 
      //construct creat command 
      //set a buffer to store content of  block
      char* buff=malloc(block_size);       
      //read  the content of the block and save it to buff
      constrcted_cmd=cmd_constructor(1,block_size,0,0,files[fh].Block_ID);
      hdd_data_lane(constrcted_cmd,buff);

      //set the new block size
      block_size_new=count+files[fh].position;

      //set another buffer to store newly edited contents
      char* buff_new=malloc(block_size_new);
      memcpy(buff_new,buff,files[fh].position);
      memcpy(buff_new+files[fh].position,data,count);

      //construct creat command 
      constrcted_cmd=cmd_constructor(0,block_size_new,0,0,0);

      //call hdd_data_lane to creat a block and get response
      result_creat=hdd_data_lane(constrcted_cmd, buff_new);

      //get R value from response
      decon_res=cmd_deconstructor(result_creat);      
      
      //free buffers
      free(buff);
      free(buff_new);

      if(decon_res.R==0)
      {
        //update information related to this file
        files[fh].Block_ID=decon_res.Block_ID;
        files[fh].file_handle=fh;
        files[fh].position=block_size_new;	
        return count;
      }  
      else
        return -1;
    }
    else
    {
      //construct creat command 
      //set a buffer to store content of block
      char* buff=malloc(block_size);       
      //read the content of the block and save it to buff
      constrcted_cmd=cmd_constructor(1,block_size,0,0,files[fh].Block_ID);
      hdd_data_lane(constrcted_cmd,buff);
      //copy data into buff
      memcpy(buff+files[fh].position,data,count);
 
      //rewrite the block with edited content in buff
      constrcted_cmd=cmd_constructor(2,block_size,0,0,files[fh].Block_ID);

      //call hdd_data_lane to creat a block and get response
      result_creat=hdd_data_lane(constrcted_cmd, buff);

      //get R value from response
      decon_res=cmd_deconstructor(result_creat); 

      free(buff);  

      if(decon_res.R==0)
      {
        //update information related to this file
        files[fh].Block_ID=decon_res.Block_ID;
        files[fh].file_handle=fh;
        files[fh].position=files[fh].position+count;	
        return count;
      }  
      else
        return -1;
    }
    //update information related to this file
    files[fh].Block_ID=decon_res.Block_ID;
    files[fh].file_handle=fh;
    files[fh].position=count;

	return count;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_seek
// Description  : seek to the specific location
// 
// Inputs       : file handle,new location
// Outputs      : changed file location
//
int32_t hdd_seek(int16_t fh, uint32_t loc) {
	//if loc is invalid, fail
	if(loc<0)
      return -1;
    else
    {
	  //change position to loc
      files[fh].position=loc;
	  return 0;
	}  
}




////////////////////////////////////////////////////////////////////////////////
//
// Function     : hddIOUnitTest
// Description  : Perform a test of the HDD IO implementation
//
// Inputs       : None
// Outputs      : 0 if successful or -1 if failure
//
// STUDENTS DO NOT MODIFY CODE BELOW UNLESS TOLD BY TA AND/OR INSTRUCTOR 
//
int hddIOUnitTest(void) {

	// Local variables
	uint8_t ch;
	int16_t fh, i;
	int32_t cio_utest_length, cio_utest_position, count, bytes, expected;
	char *cio_utest_buffer, *tbuf;
	HDD_UNIT_TEST_TYPE cmd;
	char lstr[1024];

	// Setup some operating buffers, zero out the mirrored file contents
	cio_utest_buffer = malloc(HDD_MAX_BLOCK_SIZE);
	tbuf = malloc(HDD_MAX_BLOCK_SIZE);
	memset(cio_utest_buffer, 0x0, HDD_MAX_BLOCK_SIZE);
	cio_utest_length = 0;
	cio_utest_position = 0;

	// Start by opening a file
	fh = hdd_open("temp_file.txt");
	if (fh == -1) {
		logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : Failure open operation.");
		return(-1);
	}

	// Now do a bunch of operations
	for (i=0; i<HDD_IO_UNIT_TEST_ITERATIONS; i++) {

		// Pick a random command
		if (cio_utest_length == 0) {
			cmd = HIO_UNIT_TEST_WRITE;
		} else {
			cmd = getRandomValue(HIO_UNIT_TEST_READ, HIO_UNIT_TEST_SEEK);
		}

		// Execute the command
		switch (cmd) {

		case HIO_UNIT_TEST_READ: // read a random set of data
			count = getRandomValue(0, cio_utest_length);
			logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : read %d at position %d", count, cio_utest_position);
			bytes = hdd_read(fh, tbuf, count);
			if (bytes == -1) {
				logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : Read failure.");
				return(-1);
			}

			// Compare to what we expected
			if (cio_utest_position+count > cio_utest_length) {
				expected = cio_utest_length-cio_utest_position;
			} else {
				expected = count;
			}
			if (bytes != expected) {
				logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : short/long read of [%d!=%d]", bytes, expected);
				return(-1);
			}
			if ( (bytes > 0) && (memcmp(&cio_utest_buffer[cio_utest_position], tbuf, bytes)) ) {

				bufToString((unsigned char *)tbuf, bytes, (unsigned char *)lstr, 1024 );
				logMessage(LOG_INFO_LEVEL, "HIO_UTEST R: %s", lstr);
				bufToString((unsigned char *)&cio_utest_buffer[cio_utest_position], bytes, (unsigned char *)lstr, 1024 );
				logMessage(LOG_INFO_LEVEL, "HIO_UTEST U: %s", lstr);

				logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : read data mismatch (%d)", bytes);
				return(-1);
			}
			logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : read %d match", bytes);


			// update the position pointer
			cio_utest_position += bytes;
			break;

		case HIO_UNIT_TEST_APPEND: // Append data onto the end of the file
			// Create random block, check to make sure that the write is not too large
			ch = getRandomValue(0, 0xff);
			count =  getRandomValue(1, HIO_UNIT_TEST_MAX_WRITE_SIZE);
			if (cio_utest_length+count < HDD_MAX_BLOCK_SIZE) {

				// Log, seek to end of file, create random value
				logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : append of %d bytes [%x]", count, ch);
				logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : seek to position %d", cio_utest_length);
				if (hdd_seek(fh, cio_utest_length)) {
					logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : seek failed [%d].", cio_utest_length);
					return(-1);
				}
				cio_utest_position = cio_utest_length;
				memset(&cio_utest_buffer[cio_utest_position], ch, count);

				// Now write
				bytes = hdd_write(fh, &cio_utest_buffer[cio_utest_position], count);
				if (bytes != count) {
					logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : append failed [%d].", count);
					return(-1);
				}
				cio_utest_length = cio_utest_position += bytes;
			}
			break;

		case HIO_UNIT_TEST_WRITE: // Write random block to the file
			ch = getRandomValue(0, 0xff);
			count =  getRandomValue(1, HIO_UNIT_TEST_MAX_WRITE_SIZE);
			// Check to make sure that the write is not too large
			if (cio_utest_length+count < HDD_MAX_BLOCK_SIZE) {
				// Log the write, perform it
				logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : write of %d bytes [%x]", count, ch);
				memset(&cio_utest_buffer[cio_utest_position], ch, count);
				bytes = hdd_write(fh, &cio_utest_buffer[cio_utest_position], count);
				if (bytes!=count) {
					logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : write failed [%d].", count);
					return(-1);
				}
				cio_utest_position += bytes;
				if (cio_utest_position > cio_utest_length) {
					cio_utest_length = cio_utest_position;
				}
			}
			break;

		case HIO_UNIT_TEST_SEEK:
			count = getRandomValue(0, cio_utest_length);
			logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : seek to position %d", count);
			if (hdd_seek(fh, count)) {
				logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : seek failed [%d].", count);
				return(-1);
			}
			cio_utest_position = count;
			break;

		default: // This should never happen
			CMPSC_ASSERT0(0, "HDD_IO_UNIT_TEST : illegal test command.");
			break;

		}
	}

	// Close the files and cleanup buffers, assert on failure
	if (hdd_close(fh)) {
		logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : Failure read comparison block.", fh);
		return(-1);
	}
	free(cio_utest_buffer);
	free(tbuf);

	// Return successfully
	return(0);
}

































