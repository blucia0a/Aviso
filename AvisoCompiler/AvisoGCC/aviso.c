/******************************************************************************
 * aviso.c 
 *
 * aviso - Aviso instrumentation plugin 
 *****************************************************************************/
#include <gcc-plugin.h>
#include <coretypes.h>
#include <diagnostic.h>
#include <gimple.h>
#include <tree.h>
#include <tree-flow.h>
#include <tree-pass.h>
#include <line-map.h>
#include <input.h>


int plugin_is_GPL_compatible = 1;

/* Help info about the plugin if one were to use gcc's --version --help */
static struct plugin_info aviso_info =
{
    .version = "1",
    .help = "Aviso Instrumentation Plugin (email blucia@gmail.com)",
};


static struct plugin_gcc_version aviso_ver =
{
    .basever = "4.7",
};


/* We don't need to run any tests before we execute our plugin pass */
static bool aviso_gate(void)
{
    return true;
}

static unsigned aviso_exec(void)
{
    unsigned i;
    const_tree str, op;
    basic_block bb;
    gimple stmt;
    gimple_stmt_iterator gsi;

    FOR_EACH_BB(bb)
      for (gsi=gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {

          stmt = gsi_stmt(gsi);

          for (i=0; i<gimple_num_ops(stmt); ++i){
         
            my_insert_synth_ev(bb,&gsi,stmt);

          }

      }

    return 0;
}


/* See tree-pass.h for a list and desctiptions for the fields of this struct */
static struct gimple_opt_pass aviso_pass = 
{
    .pass.type = GIMPLE_PASS,
    .pass.name = "aviso",       /* For use in the dump file */
    .pass.gate = aviso_gate,
    .pass.execute = aviso_exec, /* Pass handler/callback */
};



/* Return 0 on success or error code on failure */
int plugin_init(struct plugin_name_args   *info,  /* Argument infor */
                struct plugin_gcc_version *ver)   /* Version of GCC */
{
    struct register_pass_info pass;

    if (strncmp(ver->basever, aviso_ver.basever, strlen("4.7")))
       return -1; /* Incorrect version of gcc */

    pass.pass = &aviso_pass.pass;
    pass.reference_pass_name = "ssa";
    pass.ref_pass_instance_number = 1;
    pass.pos_op = PASS_POS_INSERT_AFTER;



    /* Tell gcc we want to be called after the first SSA pass */
    register_callback("aviso", PLUGIN_PASS_MANAGER_SETUP, NULL, &pass);
    register_callback("aviso", PLUGIN_INFO, NULL, &aviso_info);

    return 0;
}
