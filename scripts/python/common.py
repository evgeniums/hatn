import os
import codecs
import errno

def is_windows():
    return os.name != 'posix'

# copied from http://stackoverflow.com/a/600612
def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc: # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else: raise

def find_files(path):
	for dirpath, dirnames, filenames in os.walk(path):
		for filename in filenames:
			yield os.path.join(dirpath, filename)

def write_file(fname, contents):
	with codecs.open(fname, "w", "utf-8") as f:
		f.write(contents)
