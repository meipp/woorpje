#include <murmurhash.h>

#include "host/hash.hpp"

uint32_t hash_impl (const void *addr, std::size_t len, uint32_t seed) {
  uint32_t val;
  lmmh_x86_32 (addr,len,seed,&val);
  return val;
}

