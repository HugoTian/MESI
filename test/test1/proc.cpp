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
            // first Command
            // load at cycle 1
            if (cur_cycle == 1) {
                addr = 100 % test_args.addr_range;
                NOTE("Issue first load");
                response = cache->load(addr, 0, &data, response.retry_p);
                if(response.hit){
                   ERROR("should miss in this load");
                   failure = true;
                }
            }
            
            // there should be a miss , so keep retry to complete this load
            while(!response.retry_p && !completeCommand[0]){
                 response = cache->load(addr, 0, &data, response.retry_p);
                 if(response.retry_p = false)
                     completeCommand[0] = true;
            }
            
            
            // second Command
            // store to same address
            // should hit
            if(!response.retry_p && completeCommand[0]){
                NOTE("Issue first store");
                response = cache->cache->store(addr, 0, 50, response.retry_p);
                if (!response.retry_p){
                  ERROR("should retry this store, since store shared copy");
                  failure = true;           
                }
            }


            // there should be a miss , so keep retry to complete this load
            while(!response.retry_p && !completeCommand[1]){
                 response = cache->cache->store(addr, 0, 50, response.retry_p);
                 if(response.retry_p = false)
                     completeCommand[1] = true;
            }
        
            // Third Command 
            // load to pre-stored value
            // should hit
            if (!response.retry_p && completeCommand[1]){
                NOTE("Issue second load");
                response = cache->load(addr, 0, &data, response.retry_p);
                if (!response.hit){
                   ERROR("should hit on this load");
                   failure = true;           
                }
                if(data != 50){
                   ERROR("load wrong data");
                   failure = true; 
                }
            }
        // see whether pass this test case
        if (cur_cycle = args.num_cycles-1){
              if(failure){
                    ERROR("failed this test case");
              }else{
                    ERROR("succeed this test case");
              }
        }
    }
    break;

  default:
    ERROR("don't know this test case");
  }
}







