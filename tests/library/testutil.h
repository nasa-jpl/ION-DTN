/*

	testutil.h:	Helpful utility code for tests in C.

									*/

#include <bp.h>

/* A "default" amount of time to wait for bundle delivery before timing out. */
#define IONTEST_DEFAULT_RECEIVE_WAIT    (5)

/* Helper function to call an "xadmin" command.
 *
 * If path_prefix is NULL, it is replaced with "$CONFIGSROOT/", and then the
 * pseudoshell command "<xadmin> <path_prefix><xrc>" is executed.
 *
 *        call									pseudoshell equivalent
 * _xadmin("ionadmin", "", "node.ionrc");		ionadmin node.ionrc
 * _xadmin("ionadmin", NULL, "node.ionrc");		ionadmin $CONFIGSROOT/node.ionrc
 *
 * But, see the ionadmin()/ionsecadmin()/... wrappers first. */
void _xadmin(const char *xadmin, const char *path_prefix, const char *xrc);

/* Convenient wrappers for _xadmin */
#define _ionadmin(path_prefix, ionrc)         _xadmin("ionadmin", path_prefix, ionrc)
#define _ionsecadmin(path_prefix, ionsecrc)   _xadmin("ionsecadmin", path_prefix, ionsecrc)
#define _ltpadmin(path_prefix, ltprc)         _xadmin("ltpadmin", path_prefix, ltprc)
#define _bpadmin(path_prefix, bprc)           _xadmin("bpadmin", path_prefix, bprc)
#define _ipnadmin(path_prefix, ipnrc)         _xadmin("ipnadmin", path_prefix, ipnrc)
#define _dtn2admin(path_prefix, ipnrc)        _xadmin("dtn2admin", path_prefix, dtn2rc)

#define ionadmin(ionrc)                       _ionadmin("", ionrc)
#define ionsecadmin(ionsecrc)                 _ionsecadmin("", ionsecrc)
#define ltpadmin(ltprc)                       _ltpadmin("", ltprc)
#define bpadmin(bprc)                         _bpadmin("", bprc)
#define ipnadmin(ipnrc)                       _ipnadmin("", ipnrc)
#define dtn2admin(dtn2rc)                     _dtn2admin("", dtn2rc)

#define ionadmin_default_config(ionrc)        _ionadmin(NULL, ionrc)
#define ionsecadmin_default_config(ionsecrc)  _ionsecadmin(NULL, ionsecrc)
#define ltpadmin_default_config(ltprc)        _ltpadmin(NULL, ltprc)
#define bpadmin_default_config(bprc)          _bpadmin(NULL, bprc)
#define ipnadmin_default_config(ipnrc)        _ipnadmin(NULL, ipnrc)
#define dtn2admin_default_config(dtn2rc)      _dtn2admin(NULL, dtn2rc)

/* Helper function to start a simple ION node.
 *
 * If path_prefix is NULL, it is replaced with "$CONFIGSROOT/".  Then, for
 * each *rc argument that is non-NULL, the corresponding *admin program is
 * called.
 *
 * But, see the ionstart()/ionstart_default_config() wrappers first. */
void _ionstart(const char* path_prefix, const char *ionrc, 
    const char *ionsecrc, const char *ltprc, const char *bprc, 
    const char *ipnrc, const char *dtn2rc);

/* Convenient wrappers for _ionstart.
 * To start an ION node with configurations in your working directory:
 *
 *   ionstart("node.ionrc", "node.ionsecrc", "node.ltprc", "node.bprc",
 *				"node.ipnrc", "node.dtn2rc");
 *
 * To start an ION node using configurations in the default configs directory
 * defined by $CONFIGSROOT:
 *
 *   ionstart_default_config("node.ionrc", "node.ionsecrc", "node.ltprc", 
 *				"node.bprc", "node.ipnrc", "node.dtn2rc");
 *
 * Any argument for a config file not used by your node can be st to NULL. */
#define ionstart_default_config(...)            _ionstart(NULL, __VA_ARGS__)
#define ionstart(...)                           _ionstart("", __VA_ARGS__)

/* Calls each admin program using the stop argument (e.g. "ionadmin ."), to
 * shut down an ION node. */
void ionstop();

/* Returns a statically-allocated string like "../../configs/" that is the
 * prefix to the directory containing the default configurations, relative 
 * to the working directory of "dotest"; this is obtained by the test
 * infrastructure setting the CONFIGSROOT environment variable.
 */
const char *get_configs_path_prefix();

/* Loads a security key defined in a default configuration. */
int sec_addKey_default_config(char *keyName, char *fileName);
