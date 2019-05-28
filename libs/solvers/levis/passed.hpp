#ifndef _PASSED_
#define _PASSED_

#include <set>
#include <queue>

#include "words/words.hpp"

namespace Words {
  namespace Solvers {
	namespace Levis {
	  class PassedWaiting {
	  public:
		using Element = std::shared_ptr<Words::Options>;
		void insert (const Element& elem) {
		  auto hash = elem->eqhash(0);
          if (!passed.count (hash)) {
			passed.insert(hash);
			queue.push (elem);
		  }
		}

		Element pullElement () {
		  auto h = queue.front();
		  queue.pop ();
		  return h;
		}

		std::size_t size () const  {
		  return queue.size ();
		}

		std::size_t passedsize () const  {
		  return passed.size ();
		}

        void clear() {
            std::queue<Element> empty;
            std::swap( queue, empty );
        }
		
	  private:
		std::set<uint32_t> passed;
		std::queue<Element> queue;
	  };

	}
  }
}

#endif
