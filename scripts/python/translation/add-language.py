#!/usr/bin/env python

import sys

from pooperations import PoOperations

try:
    worker=PoOperations('Adds new language for translation',True)
    worker.add_language()
except:
    sys.exit(-1)

