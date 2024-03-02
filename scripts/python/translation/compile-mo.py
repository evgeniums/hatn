#!/usr/bin/env python
import sys
import os
import argparse

sys.path.append(os.path.join(os.path.dirname(__file__), ".."))
from gettexttools import GetTextTools
from common import mkdir_p

parser = argparse.ArgumentParser(
	description='Compiles po-file (human readable, suitable for editing) ' +
	'into mo-file (machine readable).',
	epilog='Should be called from scripts only.')
parser.add_argument('po_file', help='po file name')
parser.add_argument('mo_file', help='mo file name')
parser.add_argument('msgfmt', help='path to gettext\'s msgfmt')
args = parser.parse_args()
po_fname = args.po_file
mo_fname = args.mo_file

mkdir_p(os.path.dirname(mo_fname))
msg = GetTextTools(os.path.dirname(args.msgfmt))
msg.compile_mo(po_fname, mo_fname)
