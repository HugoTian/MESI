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
bool command1[2] = {false,false};

// advance one cycle
void proc_t::advance_one_cycle() {
  int data;
  int A = 100 % test_args.addr_range;
  int B = 200 % test_args.addr_range;
  switch (args.test) {
  case 0:

            // first Command
            // store at cycle 1
    
            if(!command1[0]){
                addr = A;
                NOTE("proc store A");
                response = cache->store(addr, 0, 50, false);
          
                if(response.retry_p == false){
                      command1[0] = true;
                      NOTE("proc first store finish");
                }
            }
            
            // second Command
            // load to same address
            // should hit
            else if(command1[0] && !command1[1]){
                addr = A;
                NOTE("proc load A");
                response = cache->load(addr, 0, &data, false);
                 if(response.retry_p == false){
                    command1[1] = true;
                    NOTE("proc load finish");
                 

                    if(data != 50){
                        ERROR("fail this case");
                    }else{
                        NOTE("pass this case");
                    }
                }
          
            }
   
            

    
    break;

  default:
    ERROR("don't know this test case");
  }
}







