#include <set>

template<class Stream>
	struct PassthroughStream {
	  PassthroughStream (Stream& s, std::set<char>& terminals, char c = '_') : stream(s),
												   terminals(terminals),
												   dummy(c) {}
	  void operator<< (char c) {
		state (c,*this);
	  }

	private:
	  
	  static void standard (char c,PassthroughStream<Stream>& s) {

		switch (c) {
		case '\\':
		  s.state = first;
		  break;
		default:
		  s.push(c);
		}
	  }
	  
	  static void first (char c,PassthroughStream<Stream>& s) {
	
		switch (c) {
		case 'a':
		case 'b':
		case 'f':
		case 'n':
		case 'r':
		case 't':
		case 'v':
		  s.push (s.dummy);
		  s.state = standard;
		  break;
		case 'x':
		  s.state = second;
		  break;
		default:
		  s.push ('\\');
		  s.push (c);
		}
		
		
	  }
	  
	  static void second (char c,PassthroughStream<Stream>& s) {

		s.state = third;
	  }
	  
	  static void third (char c,PassthroughStream<Stream>& s) {

		s.stream << s.dummy;
		s.state = standard;
	  }

	  void push (char c) {
		if (terminals.count(c)) {
		  stream << c;
		}

		else {
		  stream << dummy;
		}
								  
	  }
	  
	  std::function<void(char,PassthroughStream<Stream>&)> state = standard;
	  Stream& stream;
	  std::set<char>& terminals;
	  char dummy;
	};
