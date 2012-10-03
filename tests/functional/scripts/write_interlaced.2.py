#!/usr/bin/env python3

import os
import sys

mnt1 = os.environ['MNT1']
mnt2 = os.environ['MNT2']

path1 = os.path.join(mnt1, 'pif')
path2 = os.path.join(mnt2, 'pif')

def check_convergence(path1, path2):
    with open(path1, 'r') as f1:
        with open(path2, 'r')  as f2:
            data1, data2 = f1.read(), f2.read()
            if data1 != data2:
                raise Exception("'%s' != '%s'" % (data1, data2))


def write_interlaced(path1, path2):
    with open(path1, 'w') as f1:
        with open(path2, 'w')  as f2:
            f2.write('pif')
        f1.write('bite')

try:
    write_interlaced(path1, path2)
    check_convergence(path1, path2)
    print(write_interlaced.__name__, "passes")
except Exception as err:
    sys.exit("test '{0}' failed: {1}".format(write_interlaced.__name__, str(err)))
