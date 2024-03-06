#!/bin/bash

set -e

export scripts_root="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"/..

set current_path=$PWD

if [ -f "$working_dir/$toolchain-env.sh" ];
then
	source $working_dir/$toolchain-env.sh	
fi

source $scripts_root/paths.sh
source $scripts_root/cfg.sh
source $scripts_root/lib/configurations/$configuration.sh

export project_path=$src_root
export project_full=$toolchain-$module-$build_type
if [ $build_static = 1 ]; 
	then
		export project_full=$project_full-static 
fi
if [ $install_dev -eq 1 ];
	then 
		export project_full=$project_full-dev
fi

export build_path=$build_root/$project_full
export install_path=$install_root/$project_full

if [ -d $build_path ]; 
	then
		rm -rf $build_path
fi
mkdir -p $build_path

if [ -d $install_path ]; 
	then
		rm -rf $install_path
fi

source $scripts_root/lib/platforms/$platform/$toolchain-env.sh
source $scripts_root/lib/platforms/$platform/$toolchain-cfg.sh


if [[ "$PREPARE_TESTS" == "1" ]];
then
    	echo "Preparing tests script $toolchain $working_dir/run-tests.sh ..."
	if [ -L $working_dir/run-tests ]
	then
		rm $working_dir/run-tests
	fi
	if [ -f $working_dir/run-tests.sh ]
	then
		rm $working_dir/run-tests.sh
	fi

    if [ ! -z "$hatn_test_name" ];
    then
	echo "Will run test $hatn_test_name"
        run_tests="--run_test=$hatn_test_name"
    else
	echo "Will run all tests"
    fi

cat <<EOT > $working_dir/run-tests.sh
#!/bin/bash

# *** This file is auto generated, do not edit! ***

current_dir=\$PWD
test_dir=$build_path/test
export PATH=$PATH:$deps_root/bin:$deps_root/lib:\$test_dir
cd \$test_dir
hatnlibs-test --logger=XML,all,$working_dir/test-out.xml --logger=HRF,test_suite --report_level=no --result_code=no $run_tests
cd \$current_dir

EOT

cat <<EOT > $working_dir/run-tests-manual.sh
#!/bin/bash

# *** This file is auto generated, do not edit! ***

current_dir=\$PWD
test_dir=$build_path/test
export PATH=$PATH:$deps_root/bin:$deps_root/lib:\$test_dir
cd \$test_dir
hatnlibs-test --log_level=test_suite $run_tests
cd \$current_dir

EOT

	chmod +x $working_dir/run-tests.sh
	ln -s $working_dir/run-tests.sh $working_dir/run-tests
	chmod +x $working_dir/run-tests-manual.sh
fi

cat <<EOT > $working_dir/run-codechecker.sh
#!/bin/bash

# *** This file is auto generated, do not edit! ***

current_dir=\$PWD
build_path=$build_path
cd \$build_path
export PATH=~/codechecker/build/CodeChecker/bin:$PATH
source ~/codechecker/venv/bin/activate
if [ ! -f $working_dir/codechecker-$module.log ]; then
    CodeChecker log -o $working_dir/codechecker-$module.log -b "make clean;make -j$build_workers install"
fi
tag=\$RANDOM
CodeChecker analyze $working_dir/codechecker-$module.log -j$build_workers -i $project_path/codechecker-skip.cfg --tidy-config $project_path/clang-tidy.cfg -o ./codechecker\$tag
CodeChecker store -n hatn-$module ./codechecker\$tag --tag \$tag
cd \$current_dir

EOT

chmod +x $working_dir/run-codechecker.sh

cat <<EOT > $working_dir/run-clang-tidy.sh
#!/bin/bash

# *** This file is auto generated, do not edit! ***

current_dir=\$PWD
build_path=$build_path
cd \$build_path
export PATH=~/codechecker/build/CodeChecker/bin:$PATH
source ~/codechecker/venv/bin/activate
if [ ! -f $working_dir/codechecker-$module.log ]; then
    CodeChecker log -o $working_dir/codechecker-$module.log -b "make clean;make -j$build_workers install"
fi
tag=\$RANDOM
CodeChecker analyze $working_dir/codechecker-$module.log --analyzers clang-tidy -j$build_workers -i $project_path/codechecker-skip.cfg --analyzers clang-tidy --tidy-config $project_path/clang-tidy.cfg -o ./codechecker\$tag
CodeChecker store -n hatn-$module ./codechecker\$tag --tag \$tag
cd \$current_dir

EOT

chmod +x $working_dir/run-clang-tidy.sh


cd $build_path

$scripts_root/lib/platforms/run.sh

cd $current_path
