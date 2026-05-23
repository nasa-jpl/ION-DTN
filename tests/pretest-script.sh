# SCRIPT_DIR comes from runtests
export IONDIR="$(cd "$SCRIPT_DIR/.." && pwd)"
export PATH=$PATH:$IONDIR
export LD_LIBRARY_PATH=$IONDIR/.libs:$LD_LIBRARY_PATH
export CONFIGSROOT="$IONDIR/configs"
