#!/usr/bin/env python
import sys
import os

import argparse

from langoperations import LangOperations
from projectfiles import ProjectFiles

parser = argparse.ArgumentParser(
        description='Compile translation files (po format) to mo format for all languages.')
parser.add_argument('--top_project_folder',
                  required=True,
                  help='folder where to get translations.txt file with list of languages')
parser.add_argument('--project_folder',
                  required=True,
                  help='folder where to get translation files')
parser.add_argument('--basename',
                required=False,
                default="lang",
                help='a base name of translation file, .po extension will be auto added')
parser.add_argument('--translations_subfolder',
                  default="translations",
                  required=False,
                  help='subfolder in project folder where to put translation files')
parser.add_argument('--mo_folder',
                  required=False,
                  default=".",
                  help='folder where to put compiled files')

args = parser.parse_args()

top_project_folder = args.top_project_folder
project_folder = args.project_folder
translations_subfolder = args.translations_subfolder
basename = args.basename
mo_folder=args.mo_folder

projectFiles=ProjectFiles(project_folder,translations_subfolder)

tr = LangOperations(top_project_folder,project_folder,projectFiles.po_filesdir(),basename)
tr.compile_mo_all(mo_folder, basename)
