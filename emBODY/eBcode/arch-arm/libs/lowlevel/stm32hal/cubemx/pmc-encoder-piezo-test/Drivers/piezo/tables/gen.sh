#!/bin/bash
mkdir -p generated
cd generated

../faulhaber_csv_to_c.py ../srcdata/delta.csv 8192 65407 128 delta_8192
