folder('hatn') {
    description('Folder containing all jobs for hatn project')
}
folder('hatn/hatn-desktop') {
    description('Folder containing all desktop jobs for hatn project')
}
folder('hatn/hatn-ios') {
    description('Folder containing all iOS jobs for hatn project')
}
folder('hatn/hatn-android') {
    description('Folder containing all iOS jobs for hatn project')
}

job('hatn/hatn-desktop/hatn-desktop-single') {

  parameters {
        choiceParam("hatn_lib", ["all","common","validator","dataunit","base"], "hatn library")
        labelParam('hatn_platform') {
            defaultValue('windows')
            description('Platform (operating system)')
        }
        stringParam('hatn_plugins', '', 'List of plugins to build, separated with semicolon (;)')
        choiceParam("hatn_compiler", ["msvc","clang","gcc"], "Compiler")
        choiceParam("hatn_build", ["release","debug"], "Build type")
        choiceParam("hatn_link", ["shared","static"], "Linking mode")
        choiceParam("hatn_arch", ["x86_64","x86"], "Processor architecture")
        choiceParam("CMAKE_CXX_STANDARD", ["17","14","20"], "C++ standard")
        stringParam('hatn_test_name', '', 'Name of test to run')
        stringParam('hatn_branch', 'main', 'Name of git branch')
        choiceParam("HATN_SMARTPOINTERS_STD", ["NO","YES"], "Use smart pointers from C++ standard library instead of hatn smart pointers")
        booleanParam("cleanws", false, "Clean workspace before build")
    }

        logRotator(-1, 5)

    wrappers {
      preBuildCleanup {
          deleteDirectories()
          cleanupParameter('cleanws')
      }
      timestamps()
    }

    scm {
        git {
            remote {
                github('evgeniums/hatn')
            }
                branch('$hatn_branch')
            extensions {
              submoduleOptions {
                        relativeTargetDirectory('hatn')
                        recursive(true)
              }
            }
        }
    }

        environmentVariables(PREPARE_TESTS: '1',TREAT_WARNING_AS_ERROR:'1')

    steps {
          conditionalSteps {
            condition {
              not {
                stringsMatch('${hatn_platform}', 'windows', true)
              }
            }
            steps {
                shell('hatn/build/lib/build ${hatn_lib} ${hatn_compiler} ${hatn_arch} ${hatn_build} ${hatn_link} ${hatn_plugins}')
                shell('build/run-tests')
            }
          }
          conditionalSteps {
            condition {
                stringsMatch('${hatn_platform}', 'windows', true)
            }
            steps {
                batchFile('hatn/build/lib/build %hatn_lib% %hatn_compiler% %hatn_arch% %hatn_build% %hatn_link% %hatn_plugins%')
                batchFile('build/run-tests')
            }
          }
    }

      publishers {
        archiveXUnit {
            boostTest {
                pattern('build/result-xml/*.xml')
                failIfNotNew()
                skipNoTestFiles(false)
                stopProcessingIfError()
            }
          thresholdMode(ThresholdMode.PERCENT)
          skippedThresholds {
              failure(100)
              failureNew(100)
              unstable(100)
              unstableNew(100)
                        }
        }
    }
}

matrixJob('hatn/hatn-desktop/hatn-desktop-matrix') {

  parameters {
        choiceParam("hatn_lib", ["all","common","validator","dataunit","base"], "hatn library")
        stringParam('hatn_plugins', '', 'List of plugins separated with semicolon (;)')
        stringParam('hatn_test_name', '', 'Name of test to run')
        stringParam('hatn_branch', 'main', 'Name of git branch')
        choiceParam("HATN_SMARTPOINTERS_STD", ["NO","YES"], "Use smart pointers from C++ standard library instead of hatn smart pointers")
        booleanParam("cleanws", false, "Clean workspace before build")
    }

    axes {
        label('hatn_platform','windows','linux','macos')
        text('hatn_compiler','msvc','clang','gcc')
        text('hatn_build','release','debug')
        text('hatn_link','shared','static')
        text('hatn_arch','x86_64','x86')
        text('CMAKE_CXX_STANDARD', '17','14')
    }

        combinationFilter('((hatn_arch=="x86" && hatn_compiler=="msvc") || (hatn_arch=="x86_64")) &&'+
                      '((hatn_compiler=="msvc" && hatn_platform=="windows") || (hatn_compiler=="clang" && hatn_platform!="windows") || (hatn_compiler=="gcc" && hatn_platform!="macos")) && '+
                      '(CMAKE_CXX_STANDARD!="14" || (CMAKE_CXX_STANDARD=="14" && hatn_compiler!="msvc"))'
        )

        logRotator(-1, 5)

    wrappers {
      preBuildCleanup {
          deleteDirectories()
          cleanupParameter('cleanws')
      }
      timestamps()
    }

    scm {
        git {
            remote {
                github('evgeniums/hatn')
            }
                branch('$hatn_branch')
            extensions {
              submoduleOptions {
                        relativeTargetDirectory('hatn')
                        recursive(true)
              }
            }
        }
    }

        environmentVariables(PREPARE_TESTS: '1',TREAT_WARNING_AS_ERROR:'1')

    steps {
          conditionalSteps {
            condition {
              not {
                stringsMatch('${hatn_platform}', 'windows', true)
              }
            }
            steps {
                shell('hatn/build/lib/build ${hatn_lib} ${hatn_compiler} ${hatn_arch} ${hatn_build} ${hatn_link} ${hatn_plugins}')
                shell('build/run-tests')
            }
          }
          conditionalSteps {
            condition {
                stringsMatch('${hatn_platform}', 'windows', true)
            }
            steps {
                batchFile('hatn/build/lib/build %hatn_lib% %hatn_compiler% %hatn_arch% %hatn_build% %hatn_link% %hatn_plugins%')
                batchFile('build/run-tests')
            }
          }
    }

      publishers {
        archiveXUnit {
            boostTest {
                pattern('build/result-xml/*.xml')
                failIfNotNew()
                skipNoTestFiles(false)
                stopProcessingIfError()
            }
          thresholdMode(ThresholdMode.PERCENT)
          skippedThresholds {
              failure(100)
              failureNew(100)
              unstable(100)
              unstableNew(100)
                        }
        }
    }
}

job('hatn/hatn-android/hatn-android-single') {

  label('android')

  parameters {
        choiceParam("hatn_lib", ["all","common","validator","dataunit","base"], "hatn library")
        choiceParam("hatn_arch", ["x86_64","aarch64","arm-linux-androideabi","x86"], "Processor architecture")
        choiceParam("hatn_api_level", ["21","34"], "Android API level")
        stringParam('hatn_plugins', '', 'List of plugins to build, separated with semicolon (;)')
        choiceParam("CMAKE_CXX_STANDARD", ["17","14","20"], "C++ standard")
        stringParam('hatn_test_name', '', 'Name of test to run')
        stringParam('hatn_branch', 'main', 'Name of git branch')
        choiceParam("HATN_SMARTPOINTERS_STD", ["NO","YES"], "Use smart pointers from C++ standard library instead of hatn smart pointers")
        booleanParam("cleanws", false, "Clean workspace before build")
    }

        logRotator(-1, 5)

    wrappers {
      preBuildCleanup {
          deleteDirectories()
          cleanupParameter('cleanws')
      }
      timestamps()
    }

    scm {
        git {
            remote {
                github('evgeniums/hatn')
            }
                branch('$hatn_branch')
            extensions {
              submoduleOptions {
                        relativeTargetDirectory('hatn')
                        recursive(true)
              }
            }
        }
    }

        environmentVariables(PREPARE_TESTS: '1',TREAT_WARNING_AS_ERROR:'1')

    steps {
      shell('hatn/build/lib/android-build.sh ${hatn_lib} ${hatn_arch} ${hatn_api_level} ${hatn_plugins}')
      conditionalSteps {
        condition {
          stringsMatch('${hatn_arch}', 'x86_64', true)
        }
        steps {
          shell('build/run-tests.sh')
        }
      }
    }

        publishers {
      archiveXUnit {
        boostTest {
          pattern('build/result-xml/*.xml')
          failIfNotNew()
          skipNoTestFiles(true)
          stopProcessingIfError()
        }
        thresholdMode(ThresholdMode.PERCENT)
        skippedThresholds {
          failure(100)
          failureNew(100)
          unstable(100)
          unstableNew(100)
        }
      }
    }
}

matrixJob('hatn/hatn-android/hatn-android-matrix') {

  parameters {
        choiceParam("hatn_lib", ["all","common","validator","dataunit","base"], "hatn library")
        stringParam('hatn_plugins', '', 'List of plugins to build, separated with semicolon (;)')
        stringParam('hatn_test_name', '', 'Name of test to run')
        stringParam('hatn_branch', 'main', 'Name of git branch')
        choiceParam("HATN_SMARTPOINTERS_STD", ["NO","YES"], "Use smart pointers from C++ standard library instead of hatn smart pointers")
        booleanParam("cleanws", false, "Clean workspace before build")
    }

    axes {
        text('hatn_arch',"x86_64","aarch64","arm-linux-androideabi","x86")
        text('hatn_api_level',"21","34")
        text('CMAKE_CXX_STANDARD', '17','14')
        label('hatn_platform','android')
    }

        logRotator(-1, 5)

    wrappers {
      preBuildCleanup {
          deleteDirectories()
          cleanupParameter('cleanws')
      }
      timestamps()
    }

    scm {
        git {
            remote {
                github('evgeniums/hatn')
            }
                branch('$hatn_branch')
            extensions {
              submoduleOptions {
                        relativeTargetDirectory('hatn')
                        recursive(true)
              }
            }
        }
    }

        environmentVariables(PREPARE_TESTS: '1',TREAT_WARNING_AS_ERROR:'1')

    steps {
      shell('hatn/build/lib/android-build.sh ${hatn_lib} ${hatn_arch} ${hatn_api_level} ${hatn_plugins}')
      conditionalSteps {
        condition {
          stringsMatch('${hatn_arch}', 'x86_64', true)
        }
        steps {
          shell('build/run-tests.sh')
        }
      }
    }

        publishers {
      archiveXUnit {
        boostTest {
          pattern('build/result-xml/*.xml')
          failIfNotNew()
          skipNoTestFiles(true)
          stopProcessingIfError()
        }
        thresholdMode(ThresholdMode.PERCENT)
        skippedThresholds {
          failure(100)
          failureNew(100)
          unstable(100)
          unstableNew(100)
        }
      }
    }
}

job('hatn/hatn-ios/hatn-ios-single') {

  label('ios')

  parameters {
        choiceParam("hatn_lib", ["all","common","validator","dataunit","base"], "hatn library")
        choiceParam("hatn_arch", ["x86_64","arm64"], "Processor architecture")
        choiceParam("hatn_bitcode", ["native","bitcode"], "Bitcode mode")
        choiceParam("hatn_visibility", ["normal","hidden"], "Visibility mode")
        stringParam('hatn_plugins', '', 'List of plugins to build, separated with semicolon (;)')
        choiceParam("hatn_build", ["release","debug"], "Build type")
        choiceParam("CMAKE_CXX_STANDARD", ["17","14","20"], "C++ standard")
        stringParam('hatn_test_name', '', 'Name of test to run')
        stringParam('hatn_branch', 'main', 'Name of git branch')
        choiceParam("HATN_SMARTPOINTERS_STD", ["NO","YES"], "Use smart pointers from C++ standard library instead of hatn smart pointers")
        booleanParam("cleanws", false, "Clean workspace before build")
    }

        logRotator(-1, 5)

    wrappers {
      preBuildCleanup {
          deleteDirectories()
          cleanupParameter('cleanws')
      }
      timestamps()
    }

    scm {
        git {
            remote {
                github('evgeniums/hatn')
            }
                branch('$hatn_branch')
            extensions {
              submoduleOptions {
                        relativeTargetDirectory('hatn')
                        recursive(true)
              }
            }
        }
    }

        environmentVariables(PREPARE_TESTS: '1',TREAT_WARNING_AS_ERROR:'1')

    steps {
      shell('hatn/build/lib/ios-build.sh ${hatn_lib} ${hatn_arch} ${hatn_build} ${hatn_bitcode} ${hatn_visibility} ${hatn_plugins}')
      conditionalSteps {
        condition {
          and {
                stringsMatch('${hatn_arch}', 'x86_64', true)
                stringsMatch('${hatn_bitcode}', 'native', true)
          }
        }
        steps {
          shell('build/run-tests.sh')
        }
      }
    }

        publishers {
      archiveXUnit {
        boostTest {
          pattern('build/result-xml/*.xml')
          failIfNotNew()
          skipNoTestFiles(true)
          stopProcessingIfError()
        }
        thresholdMode(ThresholdMode.PERCENT)
        skippedThresholds {
          failure(100)
          failureNew(100)
          unstable(100)
          unstableNew(100)
        }
      }
    }
}

matrixJob('hatn/hatn-ios/hatn-ios-matrix') {

  parameters {
        choiceParam("hatn_lib", ["all","common","validator","dataunit","base"], "hatn library")
        stringParam('hatn_plugins', '', 'List of plugins to build, separated with semicolon (;)')
        stringParam('hatn_test_name', '', 'Name of test to run')
        stringParam('hatn_branch', 'main', 'Name of git branch')
        choiceParam("HATN_SMARTPOINTERS_STD", ["NO","YES"], "Use smart pointers from C++ standard library instead of hatn smart pointers")
        booleanParam("cleanws", false, "Clean workspace before build")
    }

     axes {
        text('hatn_arch',"x86_64","arm64")
        text('hatn_bitcode',"native","bitcode")
        text('hatn_visibility',"normal","hidden")
        text('hatn_build',"release","debug")
        text('CMAKE_CXX_STANDARD', '17','14')
        label('hatn_platform','ios')
    }

        combinationFilter('(hatn_build=="release" || hatn_build=="debug" && hatn_bitcode=="native" && hatn_visibility=="normal") && (hatn_bitcode=="native" || hatn_bitcode=="bitcode" && hatn_arch=="arm64")')

        logRotator(-1, 5)

    wrappers {
      preBuildCleanup {
          deleteDirectories()
          cleanupParameter('cleanws')
      }
      timestamps()
    }

    scm {
        git {
            remote {
                github('evgeniums/hatn')
            }
                branch('$hatn_branch')
            extensions {
              submoduleOptions {
                        relativeTargetDirectory('hatn')
                        recursive(true)
              }
            }
        }
    }

        environmentVariables(PREPARE_TESTS: '1',TREAT_WARNING_AS_ERROR:'1')

    steps {
      shell('hatn/build/lib/ios-build.sh ${hatn_lib} ${hatn_arch} ${hatn_build} ${hatn_bitcode} ${hatn_visibility} ${hatn_plugins}')
      conditionalSteps {
        condition {
          and {
                stringsMatch('${hatn_arch}', 'x86_64', true)
                stringsMatch('${hatn_bitcode}', 'native', true)
          }
        }
        steps {
          shell('build/run-tests.sh')
        }
      }
    }

        publishers {
      archiveXUnit {
        boostTest {
          pattern('build/result-xml/*.xml')
          failIfNotNew()
          skipNoTestFiles(true)
          stopProcessingIfError()
        }
        thresholdMode(ThresholdMode.PERCENT)
        skippedThresholds {
          failure(100)
          failureNew(100)
          unstable(100)
          unstableNew(100)
        }
      }
    }
}
