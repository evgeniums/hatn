import os
import sys
import shutil

sys.path.append(os.path.join(os.path.dirname(__file__), ".."))

from gettexttools import GetTextTools
from common import mkdir_p

class LangOperations:
    def __init__(self, top_project_folder, project_folder, po_translations_dir, tr_filename, pot_translations_dir="", msg=GetTextTools()):
        self.top_project_folder = top_project_folder
        self.project_folder = project_folder
        self.translations_dir = po_translations_dir
        self.pot_translations_dir = pot_translations_dir
        self.tr_filename = tr_filename
        self.msg = msg

    def languages(self):
        langs_file = open(self.top_project_folder+"/translations.txt",'r')
        return langs_file.read().split(',')

    def add_language(self, language):
        print("Adding language "+language)
        if os.path.exists(self.pot_filename()):
            po_filename = self.po_filename(language)
            if not os.path.exists(po_filename):
                print("PO file "+po_filename+" does not exist, initializing...")
                mkdir_p(os.path.dirname(po_filename))
                self.msg.msginit(po_filename, self.pot_filename(), language)
            else:
                print("PO file "+po_filename+" exists")

    def po_filename(self, lang):
        return os.path.join(self.translations_dir, lang, self.tr_filename + ".po")

    def pot_filename(self):
        if not self.pot_translations_dir:
            raise AttributeError("Path of POT file is not set!")
        return os.path.join(self.pot_translations_dir, self.tr_filename + ".pot")

    def update_po_files(self, source_files, keywords):
        if not os.path.exists(self.translations_dir):
            print("Translation folder "+self.translations_dir+" does not exist, creating...")
            mkdir_p(self.translations_dir)
        if os.path.isfile(self.pot_filename()):
            print("POT file "+self.pot_filename()+" exists, removing...")
            os.remove(self.pot_filename())
        else:
            if not os.path.exists(os.path.dirname(self.pot_filename())):
                print("POT folder "+os.path.dirname(self.pot_filename())+" does not exist, creating...")
                mkdir_p(os.path.dirname(self.pot_filename()))

        self.msg.xgettext(self.pot_filename(), keywords, source_files)

        if os.path.exists(self.pot_filename()):
            print("POT file "+self.pot_filename()+" was generated")
            for lang in self.languages():
                self.add_language(lang)
                out_filename = self.po_filename(lang)
                self.msg.msgmerge(out_filename, self.pot_filename())
        else:
           print("POT file "+self.pot_filename()+" was not generated")

    def compile_mo_single(self, language, destname):
        mkdir_p(os.path.dirname(destname))
        self.msg.compile_mo(self.po_filename(language), destname)
		
    def compile_mo_all(self, dest_langdir, basename=''):
        if basename == '':
                basename = self.tr_filename

        for lang in self.languages():
            po_fname = os.path.join(self.translations_dir, lang, self.tr_filename + ".po")
            if os.path.exists(po_fname):
                mo_fname = os.path.join(dest_langdir, lang, "LC_MESSAGES", basename + ".mo")
                mkdir_p(os.path.join(dest_langdir, lang, "LC_MESSAGES"))
                self.msg.compile_mo(po_fname, mo_fname)
