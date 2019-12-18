#include <queue>
#include <stack>
#include "solvers/solvers.hpp"
#include "words/words.hpp"
#include "passed.hpp"

namespace Words {
  namespace Solvers {
    namespace Levis {	  

      SearchOrder order;
      std::unique_ptr<SearchQueue> queue = nullptr;
      
      class Queue : public SearchQueue {
      public:
	using Element = std::shared_ptr<Words::Options>;
	virtual std::size_t size () const {return queue.size();} 
	virtual Element front () const {return queue.front();}
	virtual void pop () {queue.pop ();}
	virtual void push (const Element& elem) {queue.push (elem);}
	virtual void clear () {
	  std::queue<Element> empty;
	  std::swap (empty,queue);
	}
	virtual const std::string getName () const {return "BFS";} 
      private:
	std::queue<Element> queue;
      };
      
      class Stack : public SearchQueue {
      public:
	using Element = std::shared_ptr<Words::Options>;
	virtual std::size_t size () const {return stack.size();} 
	virtual Element front () const {return stack.top();}
	virtual void pop () {stack.pop ();}
	virtual void push (const Element& elem) {stack.push (elem);}
	virtual void clear () {
	  std::stack<Element> empty;
	  std::swap (empty,stack);
	}	
	virtual const std::string getName () const {return "DFS";}
      private:
	std::stack<Element> stack;
      };

      template<>
      void setSearchOrder<SearchOrder::DepthFirst> () {queue = std::make_unique<Stack> ();}

      template<>
      void setSearchOrder<SearchOrder::BreadthFirst> () {queue = std::make_unique<Queue> ();}

      SearchQueue& getQueue () {
	if (!queue) {
	  queue = std::make_unique<Queue> ();
	}
	assert(queue);
	return* queue;
      }
      
    }
  }
}
