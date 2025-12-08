/*
 * xSysInfo - Print/export functions header
 */

#ifndef PRINT_H
#define PRINT_H

#include "xsysinfo.h"

/* Output file path */
#define OUTPUT_FILE "RAM:sysinfo_results.txt"

/* Function prototypes */

/* Export all information to file */
BOOL export_to_file(void);

/* Individual section exports (used internally) */
void export_header(BPTR fh);
void export_hardware(BPTR fh);
void export_software(BPTR fh);
void export_benchmarks(BPTR fh);
void export_memory(BPTR fh);
void export_boards(BPTR fh);
void export_drives(BPTR fh);

#endif /* PRINT_H */
