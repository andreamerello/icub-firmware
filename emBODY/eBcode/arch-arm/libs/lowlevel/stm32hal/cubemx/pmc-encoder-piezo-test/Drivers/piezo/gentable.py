#!/usr/bin/python
import numpy as np
import math
import argparse

parser = argparse.ArgumentParser(description = "command line opts")
parser.add_argument('num', metavar = 'num', type=int,
                        help="The number of points to be generated")
args = parser.parse_args()

if 2**int(math.log(args.num, 2)) != args.num:
    print "number of points must be pow of 2"
    exit(-1)

a = []
for i in np.linspace(0,6.28,args.num):
    s_val = math.sin(i);
    val = int(s_val * 1000)
    a.append(val)


print "static const uint32_t PIEZO_PHASETABLE[{:d}] = {{\n\t".format(args.num),
for i, v in enumerate(a):
    if i > 0 and i % 8 == 0:
        print '\n\t',
    print v,',\t',
print "\n};"
