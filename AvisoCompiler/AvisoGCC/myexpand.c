#include <gcc-plugin.h>
#include <coretypes.h>
#include <diagnostic.h>
#include <gimple.h>
#include <tree.h>
#include <tree-flow.h>
#include <tree-pass.h>
#include <line-map.h>
#include <input.h>

//extern expanded_location expand_location(source_location sl) asm("__Z15expand_locationj");
bool inited = false;

extern "C" {

char *my_expand_location(gimple stmt){
            
  if( gimple_has_location( stmt ) ){

    fprintf(stderr,"(filename: %s, line: %d!\n",gimple_filename(stmt),gimple_lineno(stmt));

  }else{

    fprintf(stderr,"No debug info\n");

  }


}


tree synthEv_type;
tree synthEv_decl;
gimple synthEv_call;
void my_insert_synth_ev(basic_block bb, gimple_stmt_iterator *gsi, gimple stmt){
    
  fprintf(stderr,"Entered Insert Synth.\n");
  if(!inited){

    synthEv_type = build_function_type_list(void_type_node,void_type_node); 
  
    fprintf(stderr,"Built function type list.\n");
    synthEv_decl = build_fn_decl ("IR_SyntheticEvent", synthEv_type);
  
    fprintf(stderr,"built function decl.\n");
    synthEv_call = gimple_build_call(synthEv_decl,0);
   
    fprintf(stderr,"Built call.\n");

    inited = true;
  }

  gsi_insert_before(gsi, synthEv_call, GSI_SAME_STMT);

  fprintf(stderr,"Inserted Call.\n");
  cgraph_node *current_fun_decl_node = cgraph_get_create_node(current_function_decl);

  fprintf(stderr,"created cur fun decl node.\n");
  cgraph_node *synth_ev_decl_node = cgraph_get_create_node(synthEv_decl);

  fprintf(stderr,"create synth fun decl node.\n");

  cgraph_create_edge(current_fun_decl_node, 
                     synth_ev_decl_node, 
                     synthEv_call, 
                     compute_call_stmt_bb_frequency(current_function_decl, bb), 
                     bb->loop_depth);

  fprintf(stderr,"created call graph edge.\n");

}


}
