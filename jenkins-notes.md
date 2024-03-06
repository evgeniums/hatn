# Notes for building and testing hatn with jenkins

1. For windows: enable short workspace names for matrix configuration: add -Dhudson.matrix.MatrixConfiguration.useShortWorkspaceName=true to arguments in jenkins.xml located in the jenkins installation folder (e.g. C:\Program Files\Jenkins\jenkins.xml) and restart jenkins service.

2. Add agents with labels windows,linux.macos,ios,android.

3. On each agent add environment parameter DEPS_ROOT_PATH pointing to folder containing root folders of dependencies for each platform.

4. On each agent ensure that git and python is accessible either in PATH or add corresponding tool for git and environment variable (PATH_EXE) on agent configuration page.

5. Install Job DSL plugin.

6. Add new jenkins job to generate all hatn projects from Jenkins SDL description. Create New item, select Freestyle project, name it for example build-jenkins-jobs. On configuration page enable sorce manager from git: URL https:://github.com/evgeniums/hatn, branch main.  Then add Build step "Process Job DSL", check "Look on Filesystem" and type .jenkins-dsl in "DSL Script" field. Save configuration.

7. Run created job. Approve the script in Jenkins if requested. In case of fails due to some plugins missing install required plugins.

8. After successfull run there must be a tree of hatn projects that can be run with desired parameters.


