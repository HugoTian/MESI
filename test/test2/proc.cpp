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
bool command2[3] = {false, false, false};

// advance one cycle
void proc_t::advance_one_cycle() {
  int data;
  int A = 100 % test_args.addr_range;
  int B = 200 % test_args.addr_range;
  switch (args.test) {
  case 0:
    if(proc == 0){
            // first Command
            // store 50 to A
    
            if(!command1[0]){
                addr = A;
                NOTE("p1 store A");
                response = cache->store(addr, 0, 50, false);
          
                if(response.retry_p == false){
                      command1[0] = true;
                      NOTE("p1 first store finish");
                }
            }
            
            // second Command
            // store 1 to B
            
            else if(command1[0] && !command1[1]){
                addr = B;
                NOTE("p1 store B");
                response = cache->store(addr, 0, 1, false);
                 if(response.retry_p == false){
                    command1[1] = true;
                    NOTE("p1 second store finish");
                 }
          
            }
    }else if(proc == 1){
            
            // p2 load B
            if(!command2[0]){
                addr = B;
                NOTE("p2 load B");
                response = cache->load(addr, 0, &data, false);
          
                if(response.retry_p == false){
                      command2[0] = true;
                      NOTE("p2 first load finish");
                }
            }
            // keep load B until B is 1
            else if( command2[0] && !command2[1]){
                NOTE("p2 load B, until B is 1");
                if(data != 1){
                     addr = B;
                     NOTE("p2 load B");
                     response = cache->load(addr, 0, &data, false);
                }else{
                   command2[1] = true;
                   NOTE("p2 pass while loop");
                }
            }
            // load A again
            else if(command2[1] && !command2[2]){
                addr = A;
                NOTE("p2 load A ");
               
                response = cache->load(addr, 0, &data, false);
                if(response.retry_p == false){
                      command2[2] = true;
                      NOTE("p2  finish final load");
                      if(data != 50){
                          ERROR("fail this case");
                      }else{
                          NOTE("pass this case");
                      }
                }
            }
            else{
                
            }

    }
    break;

  default:
    ERROR("don't know this test case");
  }
}







