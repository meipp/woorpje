#ifndef _TIMING__
#define _TIMING__

#include <string>
#include <chrono>
#include <vector>

namespace Words {
  namespace Solvers {
  namespace Timing {
	class Keeper {
	public:
	  friend class Timer;
	  struct Entry {
		
		Entry (const std::string& n,double t) : name(n),time(t) {}
		const std::string name;
		double time;
	  };

	  auto begin () const {return entry.begin();}
	  auto end () const {return entry.end();}
	protected:
	  void addEntry (const std::string& name, double time ) {
		entry.emplace_back (name,time);
	  }
	  
	private:
	  std::vector<Entry> entry;
	};

	class Timer {
	public:
	  using timep = std::chrono::high_resolution_clock::time_point;
	  Timer (Keeper& keep,const std::string& s) : name (s),
														keep(keep),
														start (std::chrono::high_resolution_clock::now()) {	
	  }

	  ~Timer () {
		timep stop = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( stop - start ).count();
		keep.addEntry (name,duration);
	  }
	  
	private:
	  const std::string name;
	  Keeper& keep;
	  timep start;
	};
  }
  }
}

#endif 
