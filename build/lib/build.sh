set -e

export hatn_lib=$1
export hatn_compiler=$2
export hatn_arch=$3
export hatn_build=$4
export hatn_link=$5
export hatn_plugins=$6

my_path=${BASH_SOURCE[0]}
source ${my_path}/do-build.sh
