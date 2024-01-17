#!/bin/bash
# 
# makes a graph of how the libraries are linked togethere
#

#set -x

DOTFILE=ionlinks.dot

while getopts "ho:x" opt; do
    case "$opt" in 
        [?h])
            echo "Usage: $0 [-x] [Additional files to verify]"
            echo ""
            echo "Options:"
            echo "  -o  <FILENAME>  Use FILENAME instead of ionlinks.dot"
            echo "  -x              Also verify all executables"
            exit 1
            ;;

        o)
            DOTFILE=$OPTARG
            ;;

        x)
            CHECK_EXE=yes
            ;;
    esac
done
shift $(expr $OPTIND - 1 )

SLIBS=.libs/*.so
BINS=$(find .libs -perm /111 -and -not -name '*.*' -and -not -name 'lt-*')

export LD_LIBRARY_PATH=$PWD/.libs


LIBS=$SLIBS

if [ -n "$CHECK_EXE" ]; then
    LIBS="$SLIBS $BINS"
fi

# make the .dot file
(
    echo "strict digraph ionlibs {"

    for LIB in $LIBS $@; do

        # node
        LIB_NAME=$(basename $LIB .so)
        echo "${LIB_NAME}"

        # explicit libs
        LINKED_LIBS=$(ldd $LIB| grep .libs | sed -r 's/^.*\.libs\/(lib[^\.]+).*$/\1/')
        if [ ! -z "$LINKED_LIBS" ]; then
            for LINKED_LIB in $LINKED_LIBS; do
                echo "$LIB_NAME->$LINKED_LIB [color=green] ;"
            done
        fi

        # missing libs
        MISSING_SYMBOLS=$(ldd -r $LIB 2>&1 | egrep '^undefined symbol' | sed -r 's/^undefined symbol: ([^ 	]+).*$/\1/')
        if [ ! -z "$MISSING_SYMBOLS" ]; then
            for SYM in $MISSING_SYMBOLS; do
                MISSING_LINK=$(nm -A .libs/*.so | egrep -i " [td] $SYM" | sed -r 's/^.*\/(lib[^.]+).*$/\1/')
                if [ -z "$MISSING_LINK" ]; then
                    echo "lib $LIB needs symbol $SYM, but can't find it" 1>&2
                    echo "$LIB_NAME->NOT_FOUND [color=red] ;"
                    continue
                fi
                for I in $(echo $MISSING_LINK | sort | uniq); do
                    echo "$LIB_NAME->$I [color=red] ;"
                done
            done
        fi

        # unused links
        UNUSED_LINKS=$(ldd -r -u $LIB 2> /dev/null | egrep -v '^(Unused direct dependencies:|[    ]+)$' | sed -r 's/^.*\/(lib[^\.]+)\..*$/\1/')
        if [ ! -z "$UNUSED_LINKS" ]; then
            for UNUSED_LINK in $UNUSED_LINKS; do
                # "linux-gate" has been reported as an unused dependency since the Ubuntu 11.10 -> 12.04 x86 upgrade.  
                # http://www.trilithium.com/johan/2005/08/linux-gate/ gives a good explanation of the library, which isn't actually a "physical"
                # but rather "a virtual DSO, a shared object exposed by the kernel at a fixed address in every process' memory"
                #
                # Since it isn't a "real" library, for now I think it's safe to exclude it from unused dependency concerns.
                #
                # Exception added by Josh Schendel on 5/3/2012 for IOS 3.0.1 release.
				#
				#
                #Ignore libpthread here because we need it for nearly all of our libraries/executables and not including it where
				#its strictly unneeded would complicate the autotools build system.
				# 
				# Exception added by Samuel Jero on 6/27/2013
                if [[ "$UNUSED_LINK" != *linux-gate* ]] && [[ "$UNUSED_LINK" != *pthread* ]]; then
                    echo "$LIB_NAME->$UNUSED_LINK [color=purple] ;"
                fi
            done
        fi

    done

    echo "}"
) > $DOTFILE


#PNGFILE=${DOTFILE/.dot/.png}
#dot -Tpng -o "$PNGFILE" "$DOTFILE"

