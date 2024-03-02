#!/usr/bin/env python
import codecs
import sys
import os
import re
import subprocess
import locale

if len(sys.argv)<3:
    sys.stderr.write(sys.argv[0]+" <target_version_file> <version_text_definition_files_path> [<force_build_version>]")
    sys.exit(1)

build_version = None
release_major = "Example"
release_minor = "Release"

software_major = "0"
software_minor = "0"
software_patchlevel = "0"

versionFilesPath=sys.argv[2]
if (not versionFilesPath.endswith('/')) and (not versionFilesPath.endswith('\\')):
    versionFilesPath=versionFilesPath+"/"

try:
    release_major = open(versionFilesPath+'product_release_major.txt').readline().strip()
    release_minor = open(versionFilesPath+'product_release_minor.txt').readline().strip()
except:
    print("-- Product release version is not defined")

try:
    software_major = open(versionFilesPath+'software_major.txt').readline().strip()
    software_minor = open(versionFilesPath+'software_minor.txt').readline().strip()
    software_patchlevel = open(versionFilesPath+'software_patchlevel.txt').readline().strip()
except:
    print("-- Software version is not defined")

try:
    old_contents = map(lambda s: s.rstrip(), open(sys.argv[1], 'r').readlines())
except IOError:
    old_contents = []

def gen_contents():
    global release_major, release_minor, software_patchlevel, software_minor, server_major, build_version
    lines = []
    lines.append("#ifndef _HATN_VERSION_H")
    lines.append("#define _HATN_VERSION_H")
    lines.append("")
    lines.append("#define HATN_VERSION_RELEASE_NAME (\"{0}.{1}\")".format(release_major, release_minor))
    lines.append("#define HATN_VERSION_SOFTWARE (\"{0}.{1}.{2}\")".format(software_major, software_minor, software_patchlevel))
    lines.append("#define HATN_VERSION_BUILD ({0})".format(build_version))
    lines.append("")    
    lines.append("#endif")
    return lines

if len(sys.argv)>3:
    build_version=sys.argv[3]
else:
    os.environ['LANG'] = 'C'
    svn_info = subprocess.check_output(['svn', 'info'])
    for line in svn_info.splitlines():
        if not isinstance(line, str):
            line = line.decode(locale.getpreferredencoding())
        line = line.strip()
        m = re.match('Last Changed Rev: (.*)$', line)
        if m:
            build_version = m.group(1)

if build_version != None:
    new_contents = gen_contents()
    if new_contents == old_contents:
        print('-- Version data is not changed')
    else:
        print('-- Version data is changed, generating new file')
    with open(sys.argv[1], 'w') as f:
        for l in new_contents:
            f.write(l + '\n')

    sys.exit(0)

sys.stderr.write("Cannot find revision line in svn info output!")
sys.stderr.write(str(subprocess.check_output(['svn', 'info'])))
sys.exit(1)
