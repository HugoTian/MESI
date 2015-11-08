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

bool Command[3] = {false,false,false};

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

// advance one cycle

void proc_t::advance_one_cycle() {
  int data;
  
  int A =  100 % test_args.addr_range;
  switch (args.test) {
  case 0:
   
    if(proc == 0){
            // first Command
            // first load
            addr = A;
            if(!Command[0]){
                NOTE("single processor test");
                response = cache->load(addr, 0 , &data, false);
          
                if(response.retry_p == false){
                      Command[0] = true;
                      NOTE("first load finish");
                }
            }
            
            // second Command
            // store to same address
            // should hit
            else if(Command[0] && !Command[1]){
            
                response = cache->store(addr, 0, 50, false);
                 if(response.retry_p == false){
                    Command[1] = true;
                    NOTE("store finish");
                 }
          
            }
               
            // Third Command 
            // load to pre-stored value
            // should hit
            
            else if(Command[1] && !Command[2]){
                response = cache->load(addr, 0 , &data ,false);

                if(response.retry_p == false){
                    Command[2] = true;
                    NOTE("Second load finish");
                }

                if(response.retry_p){    
                  ERROR("should hit on this load");
                  ERROR("fail this test case");
                  return;
                              
                }
                else if(data != 50){
                  ERROR("load wrong data");
                  ERROR("fail this test case");
                  return;
                }else{
                  // see whether pass this test case
                   ERROR("succeed this test case");
                }
            }
        
    }
    break;

  default:
    ERROR("don't know this test case");
  }
}






