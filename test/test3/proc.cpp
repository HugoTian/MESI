// proc.cpp
//   by Derek Chiou
//      March 4, 2007
// 

// STUDENTS: YOU ARE EXPECTED TO MODIFY THIS FILE TO INSERT YOUR
// TESTS.  HOWEVER, YOU MUST BE ABLE TO ACCEPT OTHER PROC.CPP FILES,
// AS I WILL BE REPLACING YOUR PROC.CPP WITH MINE (AND YOUR FELLOW
// STUDENTS') FOR TESTING PURPOSES.

// for 382N-10



#include <stdio.h>
#include <stdlib.h>
#include "generic_error.h"
#include "cache.h"
#include "proc.h"
#include "test.h"


proc_t::proc_t(int __p) {
  proc = __p;
  init();
}

void proc_t::init() {
  response.retry_p = false;
  ld_p = false;
}

void proc_t::bind(cache_t *c) {
  cache = c;
}


// ***** FYTD ***** 

// this is just a simple random test.  I'm not giving
// you any more test cases than this.  You will be tested on the
// correctness and performance of your solution.

void proc_t::advance_one_cycle() {
  int data;
  bool[3] completeCommand = {false, false, false};
  bool failure = false;
  switch (args.test) {
  case 0:
    NOTE("single processor test");
    if(proc == 1){
          // issue a store to loc A
    }else if (proc == 2){
          // read back the stored value after the processor 1 succeeds store
    }else if (proc == 3){
          // store to a new location B
    }else if (proc == 4){
          // When the processor is still reading back the data the processor 4 issues a read
          // Will get the new value because the read from proc 4 will be queued by the network and by the time processing starts, 
          // the directory and homesit will ensure that it gets the correct value
    }
    break;

  default:
    ERROR("don't know this test case");
  }
}







