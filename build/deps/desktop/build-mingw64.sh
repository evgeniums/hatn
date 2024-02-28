export compiler=gcc
export toolchain=mingw64
export address_model=64
export platform=windows

export scripts_root="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

export keep_path=$PATH
export PATH=$PATH:/mingw64/bin

source $scripts_root/scripts/runner.sh

export PATH=$keep_path