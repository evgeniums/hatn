export LC_ALL=C                                                                                                                                           
export LANG=C

set -e

keep_pwd=$PWD

export working_dir=$PWD/deps
if [ ! -d "deps" ]; then 
    mkdir deps 
fi

cd deps

if [ -f "env-mingw.sh" ]; then
source env-mingw.sh
fi

check_for_program() {
  local program 
  program="${1}"

  type $program > /dev/null 2>&1 || { echo >&2 "$program is not installed, aborting"; exit 1; }    

}

check_for_program "curl"
check_for_program "7z"

if [ -f "hatn.src" ]; then
    echo "Do not run build scripts in source directory!"
    echo "Please, copy build scripts to some other working directory"
    exit 1
fi

source $scripts_root/config.sh
source $scripts_root/paths.sh

if [ ! -d "$build_dir" ]; then
    mkdir -p $build_dir 
fi

if [ ! -d "$downloads_dir" ]; then
    mkdir -p $downloads_dir 
fi

if [ ! -d "$src_dir" ]; then
    mkdir -p $src_dir 
fi

if [ ! -d "$root_dir" ]; then
    mkdir -p $root_dir 
fi

for lib in $dep_libs 
do 
    echo "Building $lib ..."
    source $deps_root/$lib.sh
    cd $working_dir
done

cd $keep_pwd    