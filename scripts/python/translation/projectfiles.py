import os
import sys

class ProjectFiles:
    def __init__(self, top_src_dir, po_dirname="translations"):
        self.top_src_dir = top_src_dir
        self.po_dirname = po_dirname

    def src_root(self):
        return self.top_src_dir

    def po_filesdir(self):
        return os.path.join(self.top_src_dir, self.po_dirname)

    def list_sources(self):
        # source file directories which want translation (server only, not client)
        source_dirnames = [
                    os.path.join(self.top_src_dir,"src"),
                    os.path.join(self.top_src_dir,"include"),
		    self.top_src_dir
        ]
        result=[]
        for dir in source_dirnames:
            for root, directories, filenames in os.walk(dir):
                for filename in filenames:
                    if filename.endswith('.cpp') or filename.endswith('.h'):
                        result.append(os.path.normpath(os.path.join(root,filename)))
        return result

    def get_translation_keywords(self):
        return ['_TR:1,1t',
                '_TR:1,2c',
                            ]
