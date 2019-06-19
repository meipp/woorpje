#ifndef _TRACE__
#define _TRACE__

#ifdef ENABLETRACE
#include <iostream>
#define TRACE(str) std::cerr << str << std::endl;
#else
#define TRACE(str) ;
#endif


#endif
