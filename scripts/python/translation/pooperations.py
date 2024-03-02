#!/usr/bin/env python
import sys
import os
import argparse

from langoperations import LangOperations
from projectfiles import ProjectFiles

class PoOperations:
    def __init__(self, programDescription, checkLang=False):
        parser = argparse.ArgumentParser(
                description=programDescription)
        parser.add_argument('--top_project_folder',
                          required=True,
                          help='folder where to get translations.txt file with list of languages')
        parser.add_argument('--project_folder',
                          required=True,
                          help='folder where to put translation files')
        parser.add_argument('--basename',
                        required=False,
                        default="lang",
                        help='a base name of translation file, .po extension will be auto added')
        parser.add_argument('--translations_subfolder',
                          default="translations",
                          required=False,
                          help='subfolder in project folder where to put translation .po files')
        parser.add_argument('--pot_folder',
                        required=False,
                        help='folder where to put .pot file, if empty then the same as project_folder')
        if (checkLang):
            parser.add_argument('--language',
                        required=False,
                        help='language to add, format: language[_territory] (e.g. "ru" or "ru_RU")')

        args = parser.parse_args()

        self.top_project_folder = args.top_project_folder
        self.project_folder = args.project_folder
        self.translations_subfolder = args.translations_subfolder
        self.basename = args.basename
        self.pot_folder = args.pot_folder

        self.projectFiles=ProjectFiles(self.project_folder,self.translations_subfolder)
        self.filenames = self.projectFiles.list_sources()
        self.po_folder=self.projectFiles.po_filesdir()
        if not self.pot_folder:
            self.pot_folder=self.po_folder
        self.tr = LangOperations(self.top_project_folder,self.project_folder,self.po_folder,self.basename,self.pot_folder)
        if (checkLang):
            if args.language is not None:
                self.lang=args.language
            else:
                if not 'NEW_LANG' in os.environ:
                    print(os.linesep+'ERROR: You must set NEW_LANG in environment of script invokation, e.g.: make add-language NEW_LANG=de'+os.linesep)
                    raise EnvironmentError("NEW_LANG undefined")
                self.lang=os.environ['NEW_LANG']

    def update_po(self):
        self.tr.update_po_files(self.filenames, self.projectFiles.get_translation_keywords())

    def add_language(self):
        self.update_po()
        self.tr.add_language(self.lang)
        print("Added translation file '{0}'.".format(self.tr.po_filename(self.lang)))
