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

extern "C" {

char *my_expand_location(gimple stmt){
            
  if( gimple_has_location( stmt ) ){
    fprintf(stderr,"(filename: %s, line: %d! [%s]\n",gimple_filename(stmt),gimple_lineno(stmt), gimple_asm_string(stmt));
  }else{
    fprintf(stderr,"No debug info\n");
  }


}

}
