#ifndef _TIMING__
#define _TIMING__

#include <vector>

namespace Words {
  namespace Timing {
	class Keeper {
	public:
	  friend class Timer;
	  struct Entry {
		const std::string Name;
		size_t time;
	  };

	  auto begin () const {return entry.begin();}
	  auto end () const {return entry.end();}
	protected:
	  void addEntry (const std:string& name, size_t time ) {
		entry.emplace_back ((name,time));
	  }
	  
	private:
	  std::vector<Entry> entry;
	};

	class Timer {
	public:
	  using timep = std::chrono::high_resolution_clock::time_point;
	  Timer (TimingKeeper& keep,const std::string& s) : name (s),
														keep(keep),
														start (std::chrono::high_resolution_clock::now()) {	
	  }

	  ~Timer () {
		timep stop = std::chrono::high_resolution_clock::now();
		auto duration = duration_cast<microseconds>( stop - start ).count();
		keep.addEntry (name,duration),
	  }
	  
	private:
	  const std::string name;
	  Keeper& keep;
	  timep start;
	}
  }
}

#endif 
