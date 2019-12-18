#ifndef _PASSED_
#define _PASSED_

#include <memory>
#include <set>

#include "words/words.hpp"

namespace Words {
  namespace Solvers {
	namespace Levis {	  
	  class SearchQueue {
	  public:
	    using Element = std::shared_ptr<Words::Options>;
	    virtual ~SearchQueue () {}
	    virtual std::size_t size () const  = 0;
	    virtual Element front () const = 0;
	    virtual void pop () = 0;
	    virtual void push (const Element& elem) = 0;
	    virtual void clear () = 0;
	    virtual const std::string getName () const {return "Unknown";} 
	  };

	  SearchQueue& getQueue ();
	  	  
	  class PassedWaiting {
	  public:
	    PassedWaiting (SearchQueue& ptr) : queue (ptr) {}
	    using Element = std::shared_ptr<Words::Options>;
	    void insert (const Element& elem) {
	      auto hash = elem->eqhash(0);
	      if (!passed.count (hash)) {
		passed.insert(hash);
		queue.push (elem);
	      }
	    }
	    
	    bool contains(const Element& elem){
	      auto hash = elem->eqhash(0);
	      return passed.count(hash);
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
	      queue.clear();
	    }
	    
	  private:
	    std::set<uint32_t> passed;
	    SearchQueue& queue;
	  };
	  
	}
  }
}

#endif
