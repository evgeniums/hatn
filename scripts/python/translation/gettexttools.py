import os
import subprocess
import codecs
import shutil

class GetTextTools:
    def __init__(self, path=''):
        self.path = path

    def run(self, exe, *params):
        old_exe = exe
        if 'GETTEXT_PATH' in os.environ and os.environ['GETTEXT_PATH']:
            exe = os.path.join(os.environ['GETTEXT_PATH'], exe)
        if self.path:
            exe = os.path.join(self.path, exe)
        #print("Using {0} from {1}, PATH={2}".format(old_exe, exe, os.environ['PATH']))
        print("Running "+exe)
        subprocess.check_call([exe] + list(params))

    def compile_mo(self, po_fname, mo_fname):
        self.run("msgfmt", po_fname, "-o", mo_fname + ".tmp")
        if os.path.exists(mo_fname):
            os.remove(mo_fname)
        os.rename(mo_fname + ".tmp", mo_fname)

    def msgmerge(self, out_filename, po_filename):
        shutil.copy(out_filename, out_filename + ".tmp")
        self.run("msgmerge", '-q', '-U', '-N', out_filename + '.tmp', po_filename)
        if os.path.exists(out_filename):
            os.remove(out_filename)
        os.rename(out_filename + ".tmp", out_filename)
        if os.path.exists(out_filename+ ".tmp~"):
            os.remove(out_filename+ ".tmp~")

    def msginit(self, out_filename, po_filename, lang):
        self.run("msginit", '--no-translator',
                '-o', out_filename + '.tmp',
                '-i', po_filename,
                '-l', lang)
        if os.path.exists(out_filename):
            os.remove(out_filename)
        os.rename(out_filename + ".tmp", out_filename)
        if os.path.exists(out_filename+ ".tmp~"):
            os.remove(out_filename+ ".tmp~")

    def xgettext(self, output, keywords, sources):      
        l = ['-o', output]
        l += ["--from-code=UTF-8"]
        l += ['-c++']
        for keyword in keywords:
            l += ['--keyword=' + keyword]
        l += list(sources)
        self.run("xgettext", *l)
