// cache.c
//   by Derek Chiou
//      March 4, 2007
// 

// for 382N-10

// STUDENTS: YOU MAY NEED TO MODIFY THIS FILE, BUT KEEP YOUR CHANGES TO A MINIMUM.

#include <stdio.h>
#include "types.h"
#include "cache.h"
#include "iu.h"
#include "helpers.h"


cache_t::cache_t(int __node, int __lg_assoc, int __lg_num_sets, int __lg_cache_line_size) {
  init(__node, __lg_assoc, __lg_num_sets, __lg_cache_line_size);
}

void cache_t::init(int __node, int __lg_assoc, int __lg_num_sets, int __lg_cache_line_size) {
  node = __node;
  lg_assoc = __lg_assoc;
  lg_num_sets = __lg_num_sets;
  lg_cache_line_size = __lg_cache_line_size;

  full_hits = partial_hits = misses = 0;

  num_sets = (1 << lg_num_sets);
  assoc = (1 << lg_assoc);

  set_shift = lg_cache_line_size;
  set_mask = (1 << lg_num_sets) - 1;

  address_tag_shift = lg_cache_line_size + lg_num_sets;

  cache_line_mask = (1 << lg_cache_line_size) - 1;
  if ((cache_line_mask + 1) != CACHE_LINE_SIZE) {
    ERROR("inconsistent cache_line_size");
  }
  
  tags = new cache_line_t*[num_sets];

  for (int i = 0; i < num_sets; ++i) {
    tags[i] = new cache_line_t[assoc];

    for (int j = 0; j < assoc; ++j) {
      tags[i][j].permit_tag = INVALID;
      tags[i][j].replacement = -1;

      for (int k = 0; k < cache_line_size; ++k) {
	tags[i][j].data[k] = 0;
      }
    }
  }
}

void cache_t::bind(iu_t *i) {
  iu = i;
}

void cache_t::print_stats() {
  printf("------------------------------\n");
  printf("node %d\n", node);
  printf("full_hits      = %d\n", full_hits);
  printf("partial_hits   = %d\n", partial_hits);
  printf("misses         = %d\n", misses);
  printf("total accesses = %d\n", full_hits + partial_hits + misses);
  printf("hit rate = %f\n", hit_rate());
}

double cache_t::hit_rate() {
  return((double)full_hits / (full_hits + partial_hits + misses));
}
  

address_tag_t cache_t::gen_address_tag(address_t addr) {
  return(addr >> address_tag_shift);
}

int cache_t::gen_offset(address_t addr) {
  return(addr & cache_line_mask);
}

int cache_t::gen_set(address_t addr) {
  int set = (addr >> set_shift) & set_mask;
  NOTE_ARGS(("addr = %x, set_shift %d, set_mask %x, set %d\n", addr, set_shift, set_mask, set));
  return(set);
}


bool cache_t::cache_access(address_t addr, permit_tag_t permit, cache_access_response_t *car) {
  int set = gen_set(addr);
  address_tag_t address_tag = gen_address_tag(addr);

  car->set = set;

  for (int a = 0; a < assoc; ++a) {
    if (tags[set][a].address_tag == address_tag) { // found in cache, check permit
      car->address_tag = address_tag;
      car->permit_tag = tags[set][a].permit_tag;
      car->way = a;

      return (car->permit_tag >= permit);

    }
  }

  car->permit_tag = INVALID;
  return(false);
}

void cache_t::modify_permit_tag(cache_access_response_t car, permit_tag_t permit) {
  tags[car.set][car.way].permit_tag = permit;
}

void cache_t::modify_address_permit_tags(cache_access_response_t car) {
  // nop
}

void cache_t::cache_fill(cache_access_response_t car, data_t data) {
  tags[car.set][car.way].address_tag = car.address_tag;
  tags[car.set][car.way].permit_tag = car.permit_tag;

  NOTE_ARGS(("set tags[%d][%d].address_tag = %d", car.set, car.way, car.address_tag));
  NOTE_ARGS(("set tags[%d][%d].permit_tag  = %d", car.set, car.way, car.permit_tag));

  touch_replacement(car);

  for (int i = 0; i < CACHE_LINE_SIZE; ++i) 
    tags[car.set][car.way].data[i] = data[i];
}

int cache_t::read_data(address_t addr, cache_access_response_t car) {
  int offset = gen_offset(addr);

  return(tags[car.set][car.way].data[offset]);
}
  

void cache_t::write_data(address_t addr, cache_access_response_t car, int data) {
  int offset = gen_offset(addr);

  tags[car.set][car.way].data[offset] = data;
}

cache_access_response_t cache_t::lru_replacement(address_t addr) {
  cache_access_response_t car;

  // already in the cache?
  if (cache_access(addr, INVALID, &car)) {
    // ack in case of requests on the fly
    if (car.permit_tag >= SHARED) {
      proc_cmd_t wb = (proc_cmd_t){WRITE, addr, node, car.permit_tag};
      copy_cache_line(wb.data, tags[car.set][car.way].data);
      iu->from_proc(wb);
    }

    // real write back
    if (car.permit_tag == MODIFIED)
      NOTE_ARGS(("%d: write back when replaceing addr %d", node, addr));

    return(car);
  }


  bool done_p = false;
  int set = gen_set(addr);

  // find LRU way
  car.address_tag = gen_address_tag(addr);
  car.set = set;

  // see if you can find yourself
  for (int a = 0; a < assoc; ++a) {
    if (tags[set][a].replacement == -1) {
      car.way = a;
      return(car);
    } else if (tags[set][a].replacement == 0) {
      done_p = true;
      car.way = a;
    }
  }

  if (done_p) return(car);

  ERROR("should be a way that is LRU");

}

// perfect LRU
void cache_t::touch_replacement(cache_access_response_t car) {
  int cur_replacement = tags[car.set][car.way].replacement;
  if (cur_replacement < 0) cur_replacement = 0;

  tags[car.set][car.way].replacement = assoc;

  for (int a = 0; a < assoc; ++a) {
    if (tags[car.set][a].replacement > cur_replacement) // demote
      --tags[car.set][a].replacement;
  }

  // consistency check
  bool found_zero_p = false;
  for (int a = 0; a < assoc; ++a) {
    if (tags[car.set][a].replacement == 0) 
      if (found_zero_p) {
	ERROR("touch_replacement: error\n"); 
      } else 
	found_zero_p = true;

    if (tags[car.set][a].replacement > assoc - 1) {
      ERROR("touch_replacement: error: assoc too large\n"); 
    }      
  }
}


response_t cache_t::load(address_t addr, bus_tag_t tag, int *data, bool retried_p) {
  response_t r;
  cache_access_response_t car;
  int a;
  

  if (cache_access(addr, SHARED, &car)) {
    if (!retried_p) ++full_hits;
    r.hit_p = true;
    r.retry_p = false;

    *data = read_data(addr, car);
    NOTE_ARGS(("%d: load hit: addr %d, tag %d", node, addr, tag));

    touch_replacement(car);

  } else {  // miss, service request
    if (!retried_p) {
      ++misses;
      NOTE_ARGS(("%d: load miss: addr %d, tag %d", node, addr, tag));
    }

    // issue a read to next level of memory hierarchy.  In this case,
    // it's the IU
      
    proc_cmd_t proc_cmd = (proc_cmd_t){READ, addr, tag, SHARED};
    bool iu_retry_p = iu->from_proc(proc_cmd);
    if (!iu_retry_p) {
      NOTE_ARGS(("%d: load to addr %x buffered", node, addr));
    }

    // create a response.  We know that it will take at least one
    // cycle to get a response and we are modeling a blocking cache
    // for now (if there is an outstanding miss, retry all memory
    // operations until that miss is statisfied.)

    r.hit_p = false;
    r.retry_p = true; // blocking cache for now
  }
  return(r);
}


response_t cache_t::store(address_t addr, bus_tag_t tag, int data, bool retried_p) {
  response_t r;
  cache_access_response_t car;
  int a;

  cache_access(addr, EXCLUSIVE, &car);

  switch (car.permit_tag) {
  case INVALID: {
    if (!retried_p) {
      ++misses;
      NOTE_ARGS(("%d: store miss: addr %d, tag %d", node, addr, tag));
    }

    proc_cmd_t proc_cmd = (proc_cmd_t){READ, addr, tag, MODIFIED};
    if (!iu->from_proc(proc_cmd)) {
      NOTE_ARGS(("%d: store to addr %x buffered", node, addr));
    }

    r.hit_p = false;
    r.retry_p = true;
    break;
  }

  case SHARED: {
    if (!retried_p) {
      ++partial_hits;
      NOTE_ARGS(("%d: store partial hit: addr %d, tag %d", node, addr, tag));
    }

    proc_cmd_t proc_cmd = (proc_cmd_t){READ, addr, tag, MODIFIED};
    if (!iu->from_proc(proc_cmd)) {
      NOTE_ARGS(("%d: store to addr %x buffered", node, addr));
    }

    r.hit_p = false;
    r.retry_p = true;
    break;
  }

  case EXCLUSIVE:
    if (!retried_p) {
      ++full_hits;
      NOTE_ARGS(("%d: store hit: addr %d, tag %d", node, addr, tag));
    }

    modify_permit_tag(car, MODIFIED);
    touch_replacement(car);
    write_data(addr, car, data);

    r.hit_p = true;
    r.retry_p = false;
    break;

  case MODIFIED:
    if (!retried_p) {
      ++full_hits;
      NOTE_ARGS(("%d: store hit: addr %d, tag %d", node, addr, tag));
    }

    r.hit_p = true;
    r.retry_p = false;
    touch_replacement(car);
    write_data(addr, car, data);
    break;
  default:
    ERROR("store: illegal permission");
  }

  return(r);
}

void cache_t::reply(proc_cmd_t proc_cmd) {
  // fill cache, return to processor

  cache_access_response_t car = lru_replacement(proc_cmd.addr);

  NOTE_ARGS(("%d: replacing addr_tag %d into set %d, assoc %d", node, car.address_tag, car.set, car.way));
  
  car.permit_tag = proc_cmd.permit_tag;
  cache_fill(car, proc_cmd.data);
  
}

response_t cache_t::snoop(net_cmd_t net_cmd) {
  response_t r;
  cache_access_response_t car;
  proc_cmd_t pc = net_cmd.proc_cmd;

  if (pc.busop == WRITE)
    ERROR("snoop: error: busop WRITE\n");

  permit_tag_t before = (pc.busop == READ) ? EXCLUSIVE : SHARED;
  permit_tag_t after = (pc.busop == READ) ? SHARED : INVALID;
  const char *op_str = (pc.busop == READ) ? "READ" : "INVALIDATE";
  
  // demotable in cache?
  r.hit_p = cache_access(pc.addr, before, &car);

  if (r.hit_p) {
    // ack in case of requests on the fly
    proc_cmd_t wb = (proc_cmd_t){WRITE, pc.addr, node, car.permit_tag};
    copy_cache_line(wb.data, tags[car.set][car.way].data);
    r.retry_p = iu->from_proc(wb);

    if (!r.retry_p) {
      // real write back
      if (car.permit_tag == MODIFIED)
        NOTE_ARGS(("%d: write back addr %d when snooping for %s", node, pc.addr, op_str));

      modify_permit_tag(car, after);
    }
  } else
    r.retry_p = false;

  return(r);
}

