// iu.cpp
//   by Derek Chiou
//      March 4, 2007
// 

// STUDENTS: YOU ARE EXPECTED TO MAKE MOST OF YOUR MODIFICATIONS IN THIS FILE.
// for 382N-10

#include "types.h"
#include "helpers.h"
#include "my_fifo.h"
#include "cache.h"
#include "iu.h"


iu_t::iu_t(int __node) {
  node = __node;
  for (int i = 0; i < MEM_SIZE; ++i) 
    for (int j = 0; j < CACHE_LINE_SIZE; ++j)
      mem[i][j] = 0;

  for(int i = 0 ; i < 768 ; i ++){
        others[i] = 0 ;
        directory[i] = 0;
  }
}

void iu_t::bind(cache_t *c, network_t *n) {
  cache = c;
  net = n;
}


void iu_t::advance_one_cycle() {
  // fixed priority: reply from network
  if (net->from_net_p(node, REPLY)) {
    process_net_reply(net->from_net(node, REPLY));

  } else if (net->from_net_p(node, REQUEST)) {
    process_net_request(net->from_net(node, REQUEST));

  } else if (proc_cmd_p && !proc_cmd_processed_p) {
    proc_cmd_processed_p = true;
    process_proc_request(proc_cmd);
  }
}

// processor side

// this interface method only takes and buffers a request from the
// processor.
bool iu_t::from_proc(proc_cmd_t pc) {
  if (!proc_cmd_p) {
    proc_cmd_p = true;
    proc_cmd = pc;

    proc_cmd_processed_p = false;
    return(false);
  } else {
    return(true);
  }
}

bool iu_t::process_proc_request(proc_cmd_t pc) {
  int dest = gen_node(pc.addr);
  int lcl = gen_local_cache_line(pc.addr);

  NOTE_ARGS(("%d: addr = %d, dest = %d", node, pc.addr, dest));
  
  //  if I am the home site
  if (dest == node) { // local

    ++local_accesses;
    // clear proc_cmd
    proc_cmd_p = false;  
    
    switch(pc.busop) {
    case READ:{
          //read shared
           if(pc.permit_tag == SHARED){
                // if the directory is clean
                if(!get_dirtybit(lcl)){
                    // add directory
                    
                    turn_up_directory(lcl,node);
                    // server the data
                    copy_cache_line(pc.data, mem[lcl]);
                    
                    cache->reply(pc);
                    return false;
                }
                // the directory is dirty
                else{
                    //read shared owner
                    int owner;
                    for(int i = 0 ; i < 32 ; i++){
                        if(get_directory(lcl,i)){
                            owner = i;
                            break;
                        }
                    }
                    ++global_accesses;
                    net_cmd_t net_cmd;

                    net_cmd.src = node;
                    net_cmd.dest = owner;
                    net_cmd.proc_cmd = pc;
                    return(net->to_net(node, REQUEST, net_cmd));
                }
           }
           // read modified
           else if(pc.permit_tag == MODIFIED){
               //there is a read miss and I am the home site
               if ( gen_node(pc.addr) == node ){
                    // if the directory is clean
                    if( !get_dirtybit(lcl)){
                        turn_up_directory(lcl, node);
                        turn_up_dirtybit(lcl);
                        copy_cache_line(pc.data, mem[lcl]);
                        cache->reply(pc);
                        return false;
                    }else{
                        //read shared owner
                        int owner;
                        for(int i = 0 ; i < 32 ; i++){
                          if(get_directory(lcl,i)){
                             owner = i;
                             break;
                          }
                        }
                        ++global_accesses;
                        net_cmd_t net_cmd;
                        pc.permit_tag = SHARED;
                        net_cmd.src = node;
                        net_cmd.dest = owner;
                        net_cmd.proc_cmd = pc;
                        return(net->to_net(node, REQUEST, net_cmd));
                    }
               }else {
                 // if thr directory is clean
                if( !get_dirtybit(lcl)){
                    // send out invalidation
                    int inv  = -1;
                    // find invalidation
                    for(int i = 0 ; i < 32 ; i++){
                        if(get_directory(lcl,i)){
                            turn_down_directory(lcl,i);
                            inv = i;
                            pc.busop = INVALIDATE;
                            ++global_accesses;
                            net_cmd_t net_cmd;

                            net_cmd.src = node;
                            net_cmd.dest = inv;
                            net_cmd.proc_cmd = pc;
                            return(net->to_net(node, REQUEST, net_cmd));
                        }
                    }
                    // if all invalidation send out
                    //add directory 
                    //set directory cache line to dirty
                    turn_up_directory(lcl, node);
                    turn_up_dirtybit(lcl);
                    copy_cache_line(pc.data, mem[lcl]);
                    cache->reply(pc);
                    return false;
                }
                // the directory is dirty
                else{
                    //read shared owner
                    int owner;
                    for(int i = 0 ; i < 32 ; i++){
                        if(get_directory(lcl,i)){
                            owner = i;
                            break;
                        }
                    }
                    ++global_accesses;
                    pc.permit_tag = SHARED;
                    net_cmd_t net_cmd;
                    net_cmd.src = node;
                    net_cmd.dest = owner;
                    net_cmd.proc_cmd = pc;
                    return(net->to_net(node, REQUEST, net_cmd));
                }
              }

           }
    }
      
      
    case WRITE:
      copy_cache_line(mem[lcl], pc.data);
      return(false);
      
    case INVALIDATE:
      // ***** FYTD *****
      return(false);  // need to return something for now
      break;
    }
    
  } else { // global
    ++global_accesses;
    net_cmd_t net_cmd;

    net_cmd.src = node;
    net_cmd.dest = dest;
    net_cmd.proc_cmd = pc;

    return(net->to_net(node, REQUEST, net_cmd));
  }
}


// receive a net request
bool iu_t::process_net_request(net_cmd_t net_cmd) {
  proc_cmd_t pc = net_cmd.proc_cmd;

  int lcl = gen_local_cache_line(pc.addr);
  int src = net_cmd.src;
  int dest = net_cmd.dest;

  // ***** FYTD *****


  switch(pc.busop) {
  case READ: // assume local
        { 
           if ( gen_node(pc.addr) == node ){

              if (pc.permit_tag == SHARED) {  // a load miss from Requestor
                    
                    if (!get_dirtybit(lcl)){
                        // turn up directory
                        turn_up_directory(lcl, src);
                        // answer this read
                        net_cmd.dest = src;
                        net_cmd.src = dest;
                        copy_cache_line(pc.data, mem[lcl]);
                        net_cmd.proc_cmd = pc;
                        return(net->to_net(node, REPLY, net_cmd));
                    } 
                    else{   // the directory cache line is dirty
                        // read shared owner
                        int owner;
                        for(int i = 0 ; i < 32 ; i++){
                            if(get_directory(lcl,i)){
                              owner = i;
                              break;
                            }
                        }
                        ++global_accesses;
                        net_cmd_t net_cmd;

                        net_cmd.src = node;
                        net_cmd.dest = owner;
                        net_cmd.proc_cmd = pc;
                        return(net->to_net(node, REQUEST, net_cmd));
                    }
              } 

              else if (pc.permit_tag == MODIFIED){ // a store miss from Requestor
                     if (!get_dirtybit(lcl)){
                          // send out invalidation
                          int inv  = -1;
                          // find invalidation
                          for(int i = 0 ; i < 32 ; i++){
                              if(get_directory(lcl,i)){
                                 turn_down_directory(lcl,i);
                                 inv = i;
                                 pc.busop = INVALIDATE;
                                 ++global_accesses;
                                 net_cmd_t net_cmd;

                                 net_cmd.src = node;
                                 net_cmd.dest = inv;
                                 net_cmd.proc_cmd = pc;
                                 return(net->to_net(node, REQUEST, net_cmd));
                               }
                          }
                          // if all invalidation send out
                          //add directory 
                          //set directory cache line to dirty
                          turn_up_directory(lcl, src);
                          turn_up_dirtybit(lcl);

                          //reply to origin requestor
                          net_cmd.dest = src;
                          net_cmd.src = dest;
                          copy_cache_line(pc.data, mem[lcl]);
                          net_cmd.proc_cmd = pc;
                          return(net->to_net(node, REPLY, net_cmd));
                    
                    } else{  // the directory cache line is dirty
                        // read shared owner
                        int owner;
                        for(int i = 0 ; i < 32 ; i++){
                            if(get_directory(lcl,i)){
                              owner = i;
                              break;
                            }
                        }
                        ++global_accesses;
                        pc.permit_tag = SHARED;
                        net_cmd_t net_cmd;
                        net_cmd.src = node;
                        net_cmd.dest = owner;
                        net_cmd.proc_cmd = pc;
                        return(net->to_net(node, REQUEST, net_cmd));
                    }
              }else{
                    // nothing happens
              }


            }  else{   // I am not the homesite,  instead, I am the owner
                  
                  if(pc.permit_tag == SHARED){
                      //Modified to shared
                      cache-> reply(pc);
                      //request : write back Home Site
                      ++global_accesses;
                      copy_cache_line(pc.data, mem[lcl]);
                      pc.busop = WRITE;
                      net_cmd_t net_cmd;
                      net_cmd.src = node;
                      net_cmd.dest = src;
                      net_cmd.proc_cmd = pc;
                      return(net->to_net(node, REQUEST, net_cmd));
                    }  
            }
     


        }
      
  case WRITE:
          {
                  if (get_dirtybit(lcl)) { // I am getting write request from owner
                        copy_cache_line(mem[lcl], pc.data);
                        turn_down_dirtybit(lcl);
                        turn_up_directory(lcl,src);
                        return false;
                  }

          }
      
  case INVALIDATE:
    // ***** FYTD *****
          { 
                pc.permit_tag = INVALID;
                cache->reply(pc);
                return(false);  // need to return something for now
          }
          
  }
}

bool iu_t::process_net_reply(net_cmd_t net_cmd) {
  proc_cmd_t pc = net_cmd.proc_cmd;

  // ***** FYTD *****

  proc_cmd_p = false; // clear out request that this reply is a reply to

  switch(pc.busop) {
  case READ: // assume local
    cache->reply(pc);
    return(false);
      
  case WRITE:
  case INVALIDATE:
    ERROR("should not have gotten a reply back from a write or an invalidate, since we are incoherent");
    return(false);  // need to return something for now
  }
}

void iu_t::print_stats() {
  printf("------------------------------\n");
  printf("%d: iu\n", node);
  
  printf("num local  accesses = %d\n", local_accesses);
  printf("num global accesses = %d\n", global_accesses);
}
