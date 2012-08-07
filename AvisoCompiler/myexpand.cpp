#include <gcc-plugin.h>
#include <coretypes.h>
#include <diagnostic.h>
#include <gimple.h>
#include <tree.h>
#include <tree-flow.h>
#include <tree-pass.h>
#include <line-map.h>
#include <input.h>
#include <set>
#include <fstream>
#include <iostream>

using namespace std;

set< std::pair<std::string, unsigned int> > processed;
set< std::pair<std::string, unsigned int> > points;
tree synthEv_type;
tree synthEv_decl;
gimple synthEv_call;
bool inited = false;

void get_points( set< std::pair<std::string, unsigned int> > &pts ){

  char *pFile = getenv("AVISOPOINTS");
  
  if( pFile != NULL ){
    cerr << "[AVISO] Instrumenting point in \"" << string(pFile) << "\"" << endl;
  }

  std::ifstream reader(pFile);

  char filename[512];

  unsigned int line;

  while (reader.good()) {

      reader >> filename;

      reader >> line;

      std::pair<std::string, unsigned int> point(filename, line);
  
      pts.insert(point);

  }

  cerr << "[AVISO] Instrumenting at " << points.size() << " points\n";

}

extern "C" {

void init_new_func(void){

  if( !inited ){
    get_points(points);
    inited = true;
  }
    
  tree param_type_list = tree_cons(NULL_TREE,void_type_node,NULL_TREE);

  synthEv_type = build_function_type(void_type_node,param_type_list); 
    
  synthEv_decl = build_fn_decl ("IR_SyntheticEvent", synthEv_type);
    

}


void my_insert_synth_ev(basic_block bb, gimple_stmt_iterator *gsi, gimple stmt){

  if( gimple_has_location( stmt ) ){

    location_t loc = gimple_location(stmt);

    string filename = string(gimple_filename(stmt));
    unsigned int line = (unsigned int) gimple_lineno(stmt);

    std::pair<std::string, unsigned int> point(filename, line);

    if( points.find(point) == points.end() ){
      
      return;

    }

    if( processed.find(point) == processed.end() ){
    
      fprintf(stderr,"[AVISO] Instrumenting %s:%d\n",gimple_filename(stmt),gimple_lineno(stmt));
   
      gimple synthEv_call = gimple_build_call(synthEv_decl,0);
  
      gsi_insert_before(gsi, synthEv_call, GSI_SAME_STMT);

      cgraph_node *current_fun_decl_node = cgraph_get_create_node(current_function_decl);

      cgraph_node *synth_ev_decl_node = cgraph_get_create_node(synthEv_decl);

      cgraph_create_edge(current_fun_decl_node, 
                       synth_ev_decl_node, 
                       synthEv_call, 
                       compute_call_stmt_bb_frequency(current_function_decl, bb), 
                       bb->loop_depth);

      processed.insert(point);  
  
    }

  }

}
}
