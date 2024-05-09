#!/bin/bash

# exit if fails
set -e

# where am i ?
FULL_PATH="$(readlink -f $0)"
LOCAL_PATH="$(dirname $FULL_PATH)"

# PATHS
cd "$LOCAL_PATH"
ROOT=$(pwd) 

# clone source repo
git submodule update --init --recursive

# tmp folder
####

echo "Creating .tmp folder..."
mkdir -p "$ROOT"/.tmp
echo "OK"

# generate_pbtools_c.py
####
echo "Composing lkm_common/generate_pbtools_c.py ..."

# replace float -> uint32_t
sed 's/\bfloat\b/uint32_t/g' "$ROOT"/libs/pbtools/lib/src/pbtools.c > "$ROOT"/.tmp/pbtools.c
sed -i 's/\bdouble\b/uint64_t/g' "$ROOT"/.tmp/pbtools.c
sed -i 's/"pbtools\.h"/"protobuf_lkm\.h"/g' "$ROOT"/.tmp/pbtools.c
sed -i '/#include <limits\.h>/d' "$ROOT"/.tmp/pbtools.c
sed -i '/#include <stdalign\.h>/d' "$ROOT"/.tmp/pbtools.c
sed -i 's/alignof(/__alignof__(/g' "$ROOT"/.tmp/pbtools.c


# prelude
cat > "$ROOT"/protobuf_lkm/lkm_common/generate_pbtools_c.py<< EOF
import os

PBTOOLS_C_FMT = '''
EOF

# C file content
cat "$ROOT"/.tmp/pbtools.c >> "$ROOT"/protobuf_lkm/lkm_common/generate_pbtools_c.py
cat "$ROOT"/.tmp/pbtools.c > "$ROOT"/examples/common/protobuf_lkm.c
rm -f "$ROOT"/.tmp/pbtools.c

# ending
cat >> "$ROOT"/protobuf_lkm/lkm_common/generate_pbtools_c.py << EOF
'''


def generate_pbtools_c(output_directory):

    filename = os.path.join(output_directory, 'protobuf_lkm.c')

    with open(filename, 'w') as fout:
        fout.write(PBTOOLS_C_FMT)

EOF

echo "OK"

# generate_pbtools_h.py
####

echo "Composing lkm_common/generate_pbtools_h.py ..."

# replace float -> uint32_t
sed 's/\bfloat\b/uint32_t/g' "$ROOT"/libs/pbtools/lib/include/pbtools.h > "$ROOT"/.tmp/pbtools.h
sed -i 's/\bdouble\b/uint64_t/g' "$ROOT"/.tmp/pbtools.h
sed -i 's/<stdint\.h>/<linux\/init\.h>/g' "$ROOT"/.tmp/pbtools.h
sed -i 's/<stdbool\.h>/<linux\/kernel\.h>/g' "$ROOT"/.tmp/pbtools.h
sed -i 's/<string\.h>/<linux\/module\.h>/g' "$ROOT"/.tmp/pbtools.h

# prelude
cat > "$ROOT"/protobuf_lkm/lkm_common/generate_pbtools_h.py << EOF
import os

PBTOOLS_H_FMT = '''
EOF

# h file content
cat "$ROOT"/.tmp/pbtools.h >> "$ROOT"/protobuf_lkm/lkm_common/generate_pbtools_h.py
cat "$ROOT"/.tmp/pbtools.h > "$ROOT"/examples/common/protobuf_lkm.h
rm -f "$ROOT"/.tmp/pbtools.h

# ending
cat >> "$ROOT"/protobuf_lkm/lkm_common/generate_pbtools_h.py << EOF
'''


def generate_pbtools_h(output_directory):

    filename = os.path.join(output_directory, 'protobuf_lkm.h')

    with open(filename, 'w') as f:
        f.write(PBTOOLS_H_FMT)

EOF

echo "OK"

# c_source.py
####

echo "Composing lkm_common/c_source.py ..."

# change import path
sed 's/pbtools\.h/\.\.\/protobuf_lkm\.h/g' "$ROOT"/libs/pbtools/pbtools/c_source/__init__.py > "$ROOT"/.tmp/c_source.py

# char bit
sed -i '/#define {include_guard}/a\
\
#ifndef CHAR_BIT\
#define CHAR_BIT  __CHAR_BIT__\
#endif' "$ROOT"/.tmp/c_source.py

# delete input
sed -i '/#include <limits\.h>/d' "$ROOT"/.tmp/c_source.py

# fix types
sed -i '/elif type in \['\''float'\'', '\''double'\'', '\''bool'\''\]:/c\
        elif type in ['\''float'\'']:\
            type = '\''uint32_t '\''\
        elif type in ['\''double'\'']:\
            type = '\''uint64_t '\''\
        elif type in ['\''bool'\'']:' "$ROOT"/.tmp/c_source.py

# delete end function as it is implemented elsewere
sed -i '/def generate_files(infiles,/,$d' "$ROOT"/.tmp/c_source.py

# place tmp file inside project
cat "$ROOT"/.tmp/c_source.py > "$ROOT"/protobuf_lkm/lkm_common/c_source.py

echo "OK"

echo "Deleting .tmp folder"
rm -rf "$ROOT"/.tmp
echo "OK"