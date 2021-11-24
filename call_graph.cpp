/*
 * Copyright 2002-2020 Intel Corporation.
 *
 * This software is provided to you as Sample Source Code as defined in the accompanying
 * End User License Agreement for the Intel(R) Software Development Products ("Agreement")
 * section 1.L.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

/*! @file
 *  This file contains an ISA-portable PIN tool for counting dynamic instructions
 */

#include "pin.H"
#include <iostream>
#include <vector>
#include <string.h>
using std::cerr;
using std::endl;
using std::string;

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */

// COS375 TIP: Add global variables here
string routineName;
bool foundMain = false;
FILE *outFile;

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */


/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr <<
        "This tool is the template for all instrumentations in project-2.\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary();

    cerr << endl;

    return -1;
}

/* ===================================================================== */
// callback function to be placed before load instructions to record the memory and
// instruction addresses.
VOID loadHandler(ADDRINT memAddr, ADDRINT instrAddr)
{
    if (foundMain){
      fprintf(outFile, "0x%lx 0x%lx L\n", instrAddr, memAddr);
    }
}

/* ===================================================================== */
// callback function to be placed before store instructions to record the memory and
// instruction addresses.
VOID storeHandler(ADDRINT memAddr, ADDRINT instrAddr)
{
    if (foundMain){
      fprintf(outFile, "0x%lx 0x%lx S\n", instrAddr, memAddr);
    }
}

/* ===================================================================== */
// A callback function executed at runtime before executing first
// instruction in a function
void executeBeforeRoutine(ADDRINT ip)
{
    // Check if main function is called
    // If so then set foundMain to true
    routineName = (RTN_FindNameByAddress(ip));
    if (routineName.compare("main") == 0){
        foundMain=true;
    }

    // Do nothing until main function is seen
    if (!foundMain){
        return;
    }

    // Check if exit function is called
    if(routineName.compare("exit") == 0){
        foundMain=false;
    }
}

/* ===================================================================== */
// Function executed everytime a new routine is found
VOID Routine(RTN rtn, VOID *v)
{
    RTN_Open(rtn);
    //Insert callback to function executeBeforeRoutine which will be
    //executed just before executing first instruction in the routine
    //at runtime
    INS_InsertCall(RTN_InsHead(rtn), IPOINT_BEFORE, (AFUNPTR)executeBeforeRoutine, IARG_INST_PTR, IARG_END);

    //Iterate over all instructions of routne rtn
    for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)){
      // if the instruction is a memory read aka a load go to loadHandler function.
      if (INS_IsMemoryRead(ins)){
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)loadHandler, IARG_MEMORYREAD_EA, IARG_ADDRINT, INS_Address(ins),  IARG_END);
      }
      // if the instruction is a memory write aka a store go to storeHandler function.
      else if (INS_IsMemoryWrite(ins)){
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)storeHandler, IARG_MEMORYWRITE_EA, IARG_ADDRINT, INS_Address(ins), IARG_END);
      }
    }
    RTN_Close(rtn);
}

/* ===================================================================== */
// Function executed after instrumentation
VOID Fini(INT32 code, VOID *v)
{
  fclose(outFile);
}


// DO NOT EDIT CODE AFTER THIS LINE
/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    PIN_InitSymbols();
    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }

    outFile = fopen("mem_trace.out","w");
    RTN_AddInstrumentFunction(Routine, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
