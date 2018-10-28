////////////////////////////////////////////////////////////////////////////////
//
//  File          : crudsim.c
//  Description   : This is the main program for the CMPSC311 programming
//                  assignment #3 (beginning of HDD interface).
//
//   Author : Patrick McDaniel
//  Last Modified  : Sat Sep 2nd 08:56:10 EDT 2017
//
////////////////////////////////////////////////////////////////////////////////
//
// STUDENTS MUST ADD COMMENTS BELOW
//


// Include Files
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// Project Includes
#include <hdd_driver.h>
#include <hdd_file_io.h>
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>
#include <cmpsc311_hashtable.h>

// Defines
#define HDD_ARGUMENTS "hvul:"
#define USAGE \
	"USAGE: crud [-h] [-v] [-l <logfile>] [-c <sz>] <workload-file>\n" \
	"\n" \
	"where:\n" \
	"    -h - help mode (display this message)\n" \
	"    -u - run the unit tests instead of the simulator\n" \
	"    -v - verbose output\n" \
	"    -l - write log messages to the filename <logfile>\n" \
	"\n" \
	"    <workload-file> - file contain the workload to simulate\n" \
	"\n" \

//
// Global Data
int verbose;

//
// Functional Prototypes

int simulate_HDD( char *wload );

//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : main
// Description  : The main function for the HDD simulator
//
// Inputs       : argc - the number of command line parameters
//                argv - the parameters
// Outputs      : 0 if successful test, -1 if failure

int main( int argc, char *argv[] ) {
	// Local variables
	int ch, verbose = 0, unit_tests = -0, log_initialized = 0;
	uint32_t cache_size = 1024; // Defaults to 1024 cache lines

	// Process the command line parameters
	while ((ch = getopt(argc, argv, HDD_ARGUMENTS)) != -1) {

		switch (ch) {
		case 'h': // Help, print usage
			fprintf( stderr, USAGE );
			return( -1 );

		case 'v': // Verbose Flag
			verbose = 1;
			break;

		case 'u': // Unit Tests Flag
			unit_tests = 1;
			break;

		case 'l': // Set the log filename
			initializeLogWithFilename( optarg );
			log_initialized = 1;
			break;

		case 'c': // Set cache line size
			if ( sscanf( optarg, "%u", &cache_size ) != 1 ) {
			    logMessage( LOG_ERROR_LEVEL, "Bad  cache size [%s]", argv[optind] );
			}
			break;

		default:  // Default (unknown)
			fprintf( stderr, "Unknown command line option (%c), aborting.\n", ch );
			return( -1 );
		}
	}

	// Setup the log as needed
	if ( ! log_initialized ) {
		initializeLogWithFilehandle( CMPSC311_LOG_STDERR );
	}
	if ( verbose ) {
		enableLogLevels( LOG_INFO_LEVEL );
	}

	// If we are running the unit tests, do that
	if ( unit_tests ) {

		// Enable verbose, run the tests and check the results
		enableLogLevels( LOG_INFO_LEVEL );
//		if ( hashTableUnitTest() || hdd_unit_test() || hddIOUnitTest() ) {
		if ( hddIOUnitTest() ) {
			logMessage( LOG_ERROR_LEVEL, "HDD unit tests failed.\n\n" );
		} else {
			logMessage( LOG_INFO_LEVEL, "HDD unit tests completed successfully.\n\n" );
		}

	} else {

		// The filename should be the next option
		if ( optind >= argc ) {

			// No filename
			fprintf( stderr, "Missing command line parameters, use -h to see usage, aborting.\n" );
			return( -1 );

		}

		// Run the simulation
		if ( simulate_HDD(argv[optind]) == 0 ) {
			logMessage( LOG_INFO_LEVEL, "HDD simulation completed successfully.\n\n" );
		} else {
			logMessage( LOG_INFO_LEVEL, "HDD simulation failed.\n\n" );
		}
	}

	// Return successfully
	return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : simulate_HDD
// Description  : The main control loop for the processing of the HDD
//                simulation.
//
// Inputs       : wload - the name of the workload file
// Outputs      : 0 if successful test, -1 if failure

int simulate_HDD( char *wload ) {

	// Local variables
	char line[1024], fname[128], command[128], text[1204], *sep;
	FILE *fhandle = NULL;
	int32_t err=0, len, off, fields, linecount;

	// Open the workload file
	linecount = 0;
	if ( (fhandle=fopen(wload, "r")) == NULL ) {
		logMessage( LOG_ERROR_LEVEL, "Failure opening the workload file [%s], error: %s.\n",
			wload, strerror(errno) );
		return( -1 );
	}

	// While file not done
	while (!feof(fhandle)) {

		// Get the line and bail out on fail
		if (fgets(line, 1024, fhandle) != NULL) {

			// Parse out the string
			linecount ++;
			fields = sscanf(line, "%s %s %d %d", fname, command, &len, &off);
			sep = strchr(line, ':');
			if ( (fields != 4) || (sep == NULL) ) {
				logMessage( LOG_ERROR_LEVEL, "HDD un-parsable workload string, aborting [%s], line %d",
						line, linecount );
				fclose( fhandle );
				return( -1 );
			}

			// Just log the contents
			logMessage(LOG_INFO_LEVEL, "File [%s], command [%s], len=%d, offset=%d",
					fname, command, len, off);

			// If there is write processing todo
			if (strncmp(command, "WRITE", 5) == 0) {

				// Now see if we need more data to fill, terminate the lines
				CMPSC_ASSERT1(len<1024, "Simulated workload command text too large [%d]", len);
				CMPSC_ASSERT2((strlen(sep+1)>=len), "Workload str [%d<%d]", strlen(sep+1), len);
				strncpy(text, sep+1, len);
				text[len] = 0x0;

				// Log the write text
				logMessage(LOG_INFO_LEVEL, "<TEXT>%s</TEXT>", text );
			}

			// Check for the virtual level failing
			if ( err ) {
				logMessage( LOG_ERROR_LEVEL, "CRUS system failed, aborting [%d]", err );
				fclose( fhandle );
				return( -1 );
			}
		}
	}

	// Close the workload file, successfully
	fclose( fhandle );
	return( 0 );
}

