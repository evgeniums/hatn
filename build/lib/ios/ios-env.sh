#!/bin/bash

#export ios_deployment_target=10.0

#export build_workers=6

#export deps_root=$working_dir/../deps/root-ios

## smarpointers mode
#export HATN_SMARTPOINTERS_STD=0

if [ -z "$gettext_path" ]
then
    export gettext_path=/usr/local/opt/gettext/bin
fi
