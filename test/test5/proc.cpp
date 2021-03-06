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
bool command2[4] = {false, false, false,false};
bool beginning = false;

// advance one cycle
void proc_t::advance_one_cycle() {
  int data;
  int A = 100 % test_args.addr_range;
  int B = 200 % test_args.addr_range;
  switch (args.test) {
  case 0:
    if(proc == 0){
            // first Command
            // store at cycle 1
    
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
            // store to same address
            // should hit
            else if(command1[0] && !command1[1]){
                addr = B;
                NOTE("p1 store B");
                response = cache->store(addr, 0, 80, false);
                 if(response.retry_p == false){
                    command1[1] = true;
                    NOTE("p1 second store finish");
                 }
          
            }
    }else {
            
            if(!beginning){
                NOTE("proc store A + i ");
                addr = A + proc;
                response = cache->store(addr, 0, 50+proc, false);
                 if(response.retry_p == false){
                    beginning = true;
                    NOTE("p store finish");
                 }
            }

            else if(!command2[0]){
                NOTE("proc load B + i -1 ");
                addr = B + proc - 1;
                response = cache->load(addr, 0, &data, false);
          
                if(response.retry_p == false){
                      command2[0] = true;
                      NOTE("load finish");
                }
            }
            else if( command2[0] && !command2[1]){
                NOTE("proc load B, until B is 80 + proc - 1");
                if(data !=  80 + proc -1){
                     addr = B + proc - 1;
                     NOTE("proc load B");
                     response = cache->load(addr, 0, &data, false);
                }else{
                   command2[1] = true;
                   NOTE("proc pass while loop");
                }
            }
            else if(command2[1] && !command2[2]){
                NOTE("proc load A + i -1 ");
                addr = A + proc - 1;
               
                response = cache->load(addr, 0, &data, false);
                if(response.retry_p == false){
                      command2[2] = true;
                      NOTE("proc finish final load");
                       if(data != 50 + proc -1 ){
                          ERROR("fail  this test ");
                       } else{
                          NOTE_ARGS(("%d: get correct value", proc));
                          NOTE("pass this case");
                       }
                }
            }else if(command2[2] && !command2[3]){
                  NOTE("proc store 80 + i to  B + i  ");
                  addr = B + proc;
                    response = cache->store(addr, 0, 80+proc, false);
                    if(response.retry_p == false){
                      command2[3] = true;
                      NOTE("p store finish");
                    }

            }else{
               
            }

    }
    break;

  default:
    ERROR("don't know this test case");
  }
}







