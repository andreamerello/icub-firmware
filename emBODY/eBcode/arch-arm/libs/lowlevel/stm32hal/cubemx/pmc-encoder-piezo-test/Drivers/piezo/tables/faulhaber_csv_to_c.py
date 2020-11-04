#!/usr/bin/python

import csv
import numpy as np
import argparse

def parse_csv(fname):
    values = []
    header = True
    with open(fname) as c:
        reader = csv.reader(c, delimiter=',')
        for row in reader:
            try:
                v = int(row[3])
            except ValueError as e:
                if header:
                    continue
                else:
                    raise e
            header = False
            values.append(v)
    return values

"""
def parse_csv(fname):
    return read_csv(fname, usecols = [3], skiprows = 9, squeeze=True)
"""
def write_csv(fname, values):
    with open(fname, 'w') as c:
        for v in values:
            c.write(str(int(v)) +',\n')


def write_c_array(fname, values):
    with open(fname, 'w') as f:
        f.write("static const uint32_t PIEZO_PHASETABLE[{:d}] = {{\n\t".format(len(values)))
        for i, v in enumerate(values):
            if i > 0 and i % 8 == 0:
                f.write('\n\t')
            f.write(str(v) + ',\t')
        f.write("\n};")


def scale_data(values, target_max, target_min):
    data_max = max(values)
    data_min = min(values)
    print "data in [ {:f}:{:f} ]".format(data_min, data_max)
    # kill offset
    values = [v - data_min for v in values]
    data_max -= data_min

    # scale factor; we will apply offset later, so take this into account
    k = (target_max - target_min) / float(data_max)
    values = [v * k for v in values]
    values = [int(v + target_min) for v in values]

    print "new data in [ {:d}:{:d} ]".format(min(values), max(values))
    return values

def interpolate_decimate_data(values, n_points):
    orig = np.linspace(0, len(values), len(values))
    target = np.linspace(0, len(values), n_points)
    result = np.interp(target, orig, values)
    return result


def parse_cli():
    parser = argparse.ArgumentParser(description = "command line opts")
    parser.add_argument('n_points', metavar = 'n_points', type=int,
                        help="The target number of points for interpolation/decimation")
    parser.add_argument('max', metavar = 'max', type=int,
                        help="max value of the wave")
    parser.add_argument('min', metavar = 'min', type=int,
                        help="min value of the wave")

    return parser.parse_args()

args = parse_cli()

values = parse_csv('Delta8_rhomb_8192p.csv')
write_csv('raw_table.csv', values)
values = interpolate_decimate_data(values, args.n_points)
write_csv('interpolated_table.csv', values)
values = scale_data(values, args.max, args.min)
write_csv('interpolated_and_scaled_table.csv', values)
write_c_array('table.c', values)
