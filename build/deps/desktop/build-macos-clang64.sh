export compiler=clang
export toolchain=clang
export address_model=64
export platform=darwin

export scripts_root="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

export keep_path=$PATH

source $scripts_root/scripts/runner.sh

export PATH=$keep_path