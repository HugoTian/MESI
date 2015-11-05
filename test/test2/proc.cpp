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
// perform load 
// return 1, hit for first time
// return 2, miss for first time
// return until load finish
int proc_t:: perform_load (address_t addr, bus_tag_t tag, int *data, bool retried_p) {
          // perform load
          response = cache->load(addr, tag , &data, response.retry_p);
          
          if( !response.retry_p){
              return 1;
          }
            
          // is there is a miss , so keep retry to complete this load
          while(response.retry_p){
                response = cache->load(addr, tag, &data, false);
                if(response.retry_p == false)
                    return 2; 
          }


          return 0;
}

// perform store
// return 1, hit for first time
// return 2, miss for first time
// return until store finish
int  proc_t:: perform_store(address_t addr, bus_tag_t tag, int data, bool retried_p){
           // perform store
          response = cache->store(addr, tag, data, response.retry_p);
          
          if( !response.retry_p){
              return 1;
          }
            
          // is there is a miss , so keep retry to complete this load
          while(response.retry_p){
                response = cache->store(addr, tag, data, false);
                if(response.retry_p ==false)
                    return 2; 
          }

          return 0;
}


// advance one cycle
void proc_t::advance_one_cycle() {
  int data;
  switch (args.test) {
  case 0:
    NOTE("2 processor store and load on same address");
    if(proc == 1 ){
            // first Command
            // store at cycle 1
            addr = 100 % test_args.addr_range;
            NOTE("p1 store A");
            int result = perform_store(addr, 0 , 50 , response.retry_p);
      
          
            addr = 200 % test_args.addr_range;
            NOTE("p1 store B");
            int result = perform_store(addr, 0 , 1 , response.retry_p);
            
    }else if(proc == 2){
            

            addr = 200 % test_args.addr_range;
            NOTE("p2 load B");
            int result = perform_load(addr, 0, &data, response.retry_p); 

            NOTE("p2 load B, until B is not 0");
            while(data == 0){
                int result = perform_load(addr, 0, &data, response.retry_p); 
            }

            addr = 100 % test_args.addr_range;
            NOTE("p2 load A ");
            int result = perform_load(addr, 0 , &data, response.retry_p); 

            if(data != 50){
                ERROR("fail this case");
            }else{
                NOTE("pass this case");
            }

    }
    break;

  default:
    ERROR("don't know this test case");
  }
}







