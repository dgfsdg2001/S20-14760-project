#!/usr/bin/env python3
import sys
import os
import re
import json
import pandas
import dirtyjson
import matplotlib.pyplot as plt
import numpy as np

def main(argv):
    with open(argv[1]) as file:
        d = dirtyjson.load(file)
        print('------ params -----\n')
        for k,v in d['params'].items():
            print('{}: {}'.format(k,v))

        print('------ app-statistics -----\n')
        stats = d['stats']
        print('TxPkts: {}, TxBytes: {}'.format(stats['TxPktCnt'], stats['TxPktCnt']*stats['PktSize']))
        print('RxPkts: {}, RxBytes: {}'.format(stats['RxPktCnt'], stats['RxPktCnt']*stats['PktSize']))
        print('PktLossRate: {}%'.format(1 - (stats['RxPktCnt']/stats['TxPktCnt'])))
        print('')

        print('latency:')
        df = pandas.DataFrame.from_dict(stats['Latency']).dropna()
        series = df['RxTime'] - df['TxTime']
        print(series.describe())
        series.plot(title='')
        plt.ylabel('sec')
        plt.show()


        # df = pandas.DataFrame.from_dict(dirtyjson.load(file))W
        # print(df)
        # print(type(df))
        # df = pandas.read_json(cleanJson(file.))
        # print(df)

if __name__ == '__main__':
    main(sys.argv)