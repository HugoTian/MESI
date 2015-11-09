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

  for (int i = 0; i < MEM_SIZE; ++i) {
    dir[i].dirty_p = false;
    dir[i].nodes = 0;
  }

  net_cmd_t empty;
  for (int i = 0; i < 32; i++)
    reads_to_answer[i] = empty;

  msgs_to_send.init(32);
}

void iu_t::bind(cache_t *c, network_t *n) {
  cache = c;
  net = n;
}


void iu_t::advance_one_cycle() {
  // fixed priority: reply from network
  if (net->from_net_p(node, REPLY)) {
    process_net_reply(net->from_net(node, REPLY));
    NOTE_ARGS(("%d: get net reply", node));

  // clear messages first
  } else if (!msgs_to_send.empty()) {
    send_message();
    NOTE_ARGS(("%d: send a message", node));

  // clear proc_cmd if it is a write, 
  // since this possibly blocks requests snooping to cache
  } else if (proc_cmd_p && !proc_cmd_processed_p && proc_cmd.busop == WRITE) {
    process_proc_request(proc_cmd);
    NOTE_ARGS(("%d: write getting processed", node));

  // retry existing net request
  } else if (net_request_retry_p) {
    net_request_retry_p = process_net_request(net_request);
    NOTE_ARGS(("%d: retry net request", node));

  // new net request
  } else if (net->from_net_p(node, REQUEST)) {
    net_request = net->from_net(node, REQUEST);
    net_request_retry_p = process_net_request(net_request);
    NOTE_ARGS(("%d: new net request", node));

  // new proc request
  } else if (proc_cmd_p && !proc_cmd_processed_p) {
    if (proc_cmd.busop == INVALIDATE)
      ERROR("proc can never issue INVALIDATE");
    proc_cmd_processed_p = true;
    process_proc_request(proc_cmd);
    NOTE_ARGS(("%d: read getting processed", node));
  }
}

bool iu_t::find_reads_to_answer(int *n, proc_cmd_t wb) {
  bool found = false;

  if (proc_cmd.busop == READ && proc_cmd.addr == wb.addr) {
    *n = node;
    found = true;
  }

  for (int i = 0; i < 32; ++i) {
    net_cmd_t net_cmd = reads_to_answer[i];
    proc_cmd_t pc = net_cmd.proc_cmd;
    if (net_cmd.valid_p && wb.addr == pc.addr) {
      if (found)
        ERROR("two reads_to_answer to same address");

      *n = i;
      found = true;
    }
  }

  if (found)
    NOTE_ARGS(("%d: to answer read from %d", node, *n));

  return found;
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

  if (dest == node) { // local
    ++local_accesses;
    
    switch(pc.busop) {
    case READ:
      // read for a load
      if (pc.permit_tag == SHARED) {
        // no one owns the line
        if (!dir[lcl].dirty_p) {
          proc_cmd_p = false; // clear proc_cmd

          dir[lcl].nodes |= (1 << node);

          copy_cache_line(pc.data, mem[lcl]);
          cache->reply(pc);

          NOTE_ARGS(("%d: all shared, load satisfied locally", node));
        // demote the owner
        } else {
          bool owner_p = false;
          unsigned int nodes = dir[lcl].nodes;

          for (int i = 0; i < 32; ++i) {
            if ((nodes >> i) & 0x1) {
              if (owner_p)
                ERROR("multiple modifier found");

              owner_p = true;
              net_cmd_t net_cmd;

              net_cmd.src = node;
              net_cmd.dest = i;
              net_cmd.proc_cmd = (proc_cmd_t){READ, pc.addr, node, SHARED};
              msgs_to_send.enqueue(net_cmd);
              NOTE_ARGS(("%d: owned by %d, read forwarding from there", node, i));
            }
          }
        }

      // read for a store
      } else if (pc.permit_tag == MODIFIED) {
        unsigned int nodes = dir[lcl].nodes;

        // the line is shared to at most only me
        if (nodes == 0 || nodes == (1 << node)) {
          proc_cmd_p = false; // clear proc_cmd

          dir[lcl].nodes |= (1 << node);
          dir[lcl].dirty_p = true;

          copy_cache_line(pc.data, mem[lcl]);
          cache->reply(pc);
          NOTE_ARGS(("%d: owned at most by me, load satisfied locally", node));
        
        // invalidate others
        } else {
          for (int i = 0; i < 32; ++i) {
            if ((nodes >> i) & 0x1) {
              net_cmd_t net_cmd;

              net_cmd.src = node;
              net_cmd.dest = i;
              net_cmd.proc_cmd = (proc_cmd_t){INVALIDATE, pc.addr, node};
              msgs_to_send.enqueue(net_cmd);
              NOTE_ARGS(("%d: owned by %d, rwitm forwarding from there", node, i));
            }
          }
        }
      }
      return(false);
      
    case WRITE:
      proc_cmd_p = false;
      proc_cmd_processed_p = true;
      int reply_dst;

      if (pc.permit_tag == MODIFIED) {
        // write back due to replacement
        if (!find_reads_to_answer(&reply_dst, pc)) {
          dir[lcl].dirty_p = false;
          dir[lcl].nodes &= ~(1 << node);

          copy_cache_line(mem[lcl], pc.data);

          NOTE_ARGS(("%d: lru write back addr %d to memory", node, pc.addr));
          return false;
        }

        // write back due to demotion
        // prepare the reply first, 
        // and do maintainence after reply sent for sure

        net_cmd_t reply = reads_to_answer[reply_dst];
        reply.src = node;
        reply.dest = reply_dst;
        copy_cache_line(reply.proc_cmd.data, pc.data);

        if (net->to_net(node, REPLY, reply)) {
          dir[lcl].dirty_p = false;
          dir[lcl].nodes &= ~(1 << node);

          copy_cache_line(mem[lcl], pc.data);

          reads_to_answer[reply_dst].valid_p = false;
          NOTE_ARGS(("%d: demotion write back addr %d to memory", node, pc.addr));
          return false;
        } else {
          proc_cmd_p = true;
          proc_cmd_processed_p = false;
          NOTE_ARGS(("%d: retry demotion write back addr %d to memory", node, pc.addr));
          return true;
        }

      } else if (pc.permit_tag == SHARED) {
        // write back due to replacement
        if (!find_reads_to_answer(&reply_dst, pc)) {
          dir[lcl].nodes &= ~(1 << node);
          NOTE_ARGS(("%d: lru ack addr %d to memory", node, pc.addr));
          return false;
        } 

        unsigned int nodes = dir[lcl].nodes;

        // first few acks for invalidation
        if (nodes != (1 << node)) {
          dir[lcl].nodes &= ~(1 << node);
          NOTE_ARGS(("%d: ack from self to addr %d", node, pc.addr));
          return false;

        // last ack for invalidation
        } else {
          net_cmd_t reply = reads_to_answer[reply_dst];
          reply.src = node;
          reply.dest = reply_dst;
          copy_cache_line(reply.proc_cmd.data, mem[lcl]);
     
          if (net->to_net(node, REPLY, reply)) {
            dir[lcl].nodes &= ~(1 << node);

            reads_to_answer[reply_dst].valid_p = false;
            NOTE_ARGS(("%d: last ack addr %d to memory", node, pc.addr));
            return false;
          } else {
            proc_cmd_p = true;
            proc_cmd_processed_p = false;
            NOTE_ARGS(("%d: retry last ack addr %d to memory", node, pc.addr));
            return true;
          }
        }
      }
      
    case INVALIDATE:
      // ***** FYTD *****
      ERROR("proc can never issue a INVALIDATE");
      return(false);  // need to return something for now
      break;
    }
    
  } else { // global
    ++global_accesses;
    net_cmd_t net_cmd;

    net_cmd.src = node;
    net_cmd.dest = dest;
    net_cmd.proc_cmd = pc;

    msgs_to_send.enqueue(net_cmd);
    NOTE_ARGS(("%d: request buffered to send", node));
    return(false);
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
  case READ:
    // not home site
    if (gen_node(pc.addr) != node) {
      // demoted to SHARED
      response_t r = cache->snoop(net_cmd);
      NOTE_ARGS(("%d: not home: snoop for addr = %d to SHARED", node, pc.addr));
      return r.retry_p;

    // home site
    } else {
      if (pc.permit_tag == SHARED) {
        // the line is shared only
        if (!dir[lcl].dirty_p) {
          net_cmd.dest = src;
          net_cmd.src = dest;
          copy_cache_line(pc.data, mem[lcl]);
          net_cmd.proc_cmd = pc;

          // retry if the reply cannot be received
          if (net->to_net(node, REPLY, net_cmd)) {
            dir[lcl].nodes |= (1 << src);
            NOTE_ARGS(("%d: home: all shared, reply to %d for addr = %d", node, src, pc.addr));
            return false;
          } else {
            NOTE_ARGS(("%d: home: all shared, retry request from %d for addr = %d", node, src, pc.addr));
            return true;
          }

        // demote the owner
        } else {
          net_cmd.valid_p = true;
          reads_to_answer[src] = net_cmd;

          unsigned int nodes = dir[lcl].nodes;

          // home site is the owner
          if (nodes == (1 << node)) {
            response_t r = cache->snoop(net_cmd);
            NOTE_ARGS(("%d: home: snoop for addr = %d to SHARED", node, pc.addr));
            return r.retry_p;

          // owner is somewhere else
          } else {
            bool owner_p = false;

            for (int i = 0; i < 32; ++i) {
              if ((nodes >> i) & 0x1) {
                if (owner_p)
                  ERROR("multiple modifier found");
                if (i == node)
                  ERROR("inconsistent directory");

                owner_p = true;

                net_cmd.src = node;
                net_cmd.dest = i;
                net_cmd.proc_cmd = (proc_cmd_t){READ, pc.addr, node, SHARED};
                msgs_to_send.enqueue(net_cmd);
                NOTE_ARGS(("%d: home: load forwarding from owner %d", node, i));
              }
            }
            return false;
          }
        }
      } else if (pc.permit_tag == MODIFIED) {
        unsigned int nodes = dir[lcl].nodes;

        // the line is shared to at most only src
        if (nodes == 0 || nodes == (1 << src)) {
          net_cmd.dest = src;
          net_cmd.src = dest;
          copy_cache_line(pc.data, mem[lcl]);
          net_cmd.proc_cmd = pc;

          if (net->to_net(node, REPLY, net_cmd)) {
            dir[lcl].nodes |= (1 << src);
            dir[lcl].dirty_p = true;
            NOTE_ARGS(("%d: home: owned at most by %d, reply to %d", node, src, src));
            return false;
          } else {
            NOTE_ARGS(("%d: home: retry, owned at most by %d, reply to %d", node, src, src));
            return true;
          }

        // invalidate others
        } else {
          net_cmd.valid_p = true;
          reads_to_answer[src] = net_cmd;

          for (int i = 0; i < 32; ++i) {
            if ((nodes >> i) & 0x1) {
              net_cmd_t inv;

              inv.src = node;
              inv.dest = i;
              inv.proc_cmd = (proc_cmd_t){INVALIDATE, pc.addr, node};
              msgs_to_send.enqueue(inv);
              NOTE_ARGS(("%d: home: invalidate %d", node, i));
            }
          }
          return(false);
        }
      }
    }

  case WRITE:
    // sanity check
    if (gen_node(pc.addr) != node) 
      ERROR("sent to wrong home site!");

    int reply_dst;

    if (pc.permit_tag == MODIFIED) {
      // write back due to replacement
      if (!find_reads_to_answer(&reply_dst, pc)) {
        dir[lcl].dirty_p = false;
        dir[lcl].nodes &= ~(1 << src);

        copy_cache_line(mem[lcl], pc.data);
        NOTE_ARGS(("%d: home: lru replacement for addr %d", node, pc.addr));
        return false;
      }

      // write back due to demotion
      // prepare the reply first, 
      // and do maintainence after reply sent for sure

      net_cmd_t reply = reads_to_answer[reply_dst];
      reply.src = node;
      reply.dest = reply_dst;
      copy_cache_line(reply.proc_cmd.data, pc.data);

      if (net->to_net(node, REPLY, reply)) {
        dir[lcl].dirty_p = false;
        dir[lcl].nodes &= ~(1 << src);

        copy_cache_line(mem[lcl], pc.data);

        reads_to_answer[reply_dst].valid_p = false;
        NOTE_ARGS(("%d: home: demotion write back addr %d to memory", node, pc.addr));
        return false;
      } else {
        NOTE_ARGS(("%d: home: retry demotion write back addr %d to memory", node, pc.addr));
        return true;
      }

    } else if (pc.permit_tag == SHARED) {
      // write back due to replacement
      if (!find_reads_to_answer(&reply_dst, pc)) {
        dir[lcl].nodes &= ~(1 << src);
        NOTE_ARGS(("%d: home: lru ack for addr %d", node, pc.addr));
        return false;
      } 

      unsigned int nodes = dir[lcl].nodes;

      // first few acks for invalidation
      if (nodes != (1 << src)) {
        dir[lcl].nodes &= ~(1 << src);
        NOTE_ARGS(("%d: home: ack from %d for addr %d", node, src, pc.addr));
        return false;

      // last ack for invalidation
      } else {
        net_cmd_t reply = reads_to_answer[reply_dst];
        reply.src = node;
        reply.dest = reply_dst;
        copy_cache_line(reply.proc_cmd.data, mem[lcl]);
     
        if (net->to_net(node, REPLY, reply)) {
          dir[lcl].nodes &= ~(1 << src);

          reads_to_answer[reply_dst].valid_p = false;
          NOTE_ARGS(("%d: home: last ack from %d for addr %d", node, src, pc.addr));
          return false;
        } else {
          NOTE_ARGS(("%d: home: retry last ack from %d for addr %d", node, src, pc.addr));
          return true;
        }
      }
    }
      
  case INVALIDATE:
    // ***** FYTD *****

    // demote to INVALID
    response_t r = cache->snoop(net_cmd);
    NOTE_ARGS(("%d: snoop for addr %d to INVALID", node, pc.addr));
    return(r.retry_p);
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

void iu_t::send_message() {
  if (msgs_to_send.empty()) return;

  if (net->to_net(node, REQUEST, msgs_to_send.front())) {
    msgs_to_send.dequeue();
  }
}

void iu_t::print_stats() {
  printf("------------------------------\n");
  printf("%d: iu\n", node);
  
  printf("num local  accesses = %d\n", local_accesses);
  printf("num global accesses = %d\n", global_accesses);
}
