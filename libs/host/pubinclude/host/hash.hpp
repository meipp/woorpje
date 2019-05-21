#ifndef _HASH__
#define _HASH__

#include <cstdint>


uint32_t hash_impl (const void *addr, std::size_t len, uint32_t seed);

namespace Words {
  namespace Hash {
	template<class T>
	uint32_t Hash (const T* data, std::size_t size,uint32_t seed) {
	  return hash_impl (data,size*sizeof(T),seed);
	}
  }
}


#endif
