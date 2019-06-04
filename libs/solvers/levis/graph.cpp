#include <string>
#include <ostream>
#include <sstream>
#include <graphviz/cgraph.h>
#include "graph.hpp"

namespace Words {
  namespace Solvers {
	namespace Levis {
	  void outputToString (const std::string& name, const Graph& g){
		Agraph_t* graph = agopen (const_cast<char*>(name.c_str()),Agdirected,0);
		agattr(graph,AGEDGE,const_cast<char*>("label"),const_cast<char*> (""));
		agattr(graph,AGNODE,const_cast<char*>("label"),const_cast<char*>(""));
		agattr(graph,AGNODE,const_cast<char*>("shape"),const_cast<char*>("rectangle"));
		agattr(graph,AGNODE,const_cast<char*>("color"),const_cast<char*>("black"));
		agattr(graph,AGNODE,const_cast<char*>("fillcolor"),const_cast<char*>("white"));
		agattr(graph,AGNODE,const_cast<char*>("style"),const_cast<char*>("filled"));;

		auto it = g.begin();
		auto end = g.end();
		
		for (; it != end; ++it) {
		  auto& e = *it;
		  auto from = e->from;
		  auto to = e->to;
		  std::stringstream fstr;
		  std::stringstream tstr;
		  std::stringstream ledge;
		  fstr << *(from->opt);
		  tstr << *(to->opt);
		  ledge << e->subs;
		  auto fromn = agnode (graph,const_cast<char*> (fstr.str().c_str()),1);
		  auto ton = agnode (graph,const_cast<char*> (tstr.str().c_str()),1);

		  if (from->ranSMTSolver)
			agset (fromn,const_cast<char*>("fillcolor"),"blue");
		  if (to->ranSMTSolver)
			agset (ton,const_cast<char*>("fillcolor"),"blue");
		  
		  
		  agset (fromn,const_cast<char*>("label"),const_cast<char*>(fstr.str().c_str()));
		  agset (ton,const_cast<char*>("label"),const_cast<char*>(tstr.str().c_str()));
		  
		  auto edge = agedge(graph,fromn,ton,const_cast<char*>(ledge.str().c_str()),1);
		  agset (edge,const_cast<char*>("label"),const_cast<char*>(ledge.str().c_str()));
		  }
		
		auto out = fopen ((name+".dot").c_str(),"w");
		agwrite (graph,out);
		fclose (out);
	  }
	  
	  
	}
  }
}
