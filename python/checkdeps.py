#!/usr/bin/env python
import os
import sys
import subprocess

dir='../include/cxx/'

def process(filename):
    f=open('test.cpp','w')
    f.write('#include "{}"\n'.format(filename))
    f.write('int main(int argc, char* argv[]) {\n')
    f.write('  return 0;\n}\n')
    f.close()
    rc=subprocess.call(['g++','-c','-o','objfile.o','-I../include','-I/usr/include/eigen3','-Wfatal-errors','test.cpp'])
    sys.stdout.write('\033[0m{}'.format(filename))
    sys.stdout.write(' '*(40-len(filename)))
    if rc==0:
        print '\033[32mOK'
    else:
        print '\033[31mFailed'
    os.remove('objfile.o')
    os.remove('test.cpp')

def main():
    for (root,dirs,files) in os.walk(dir):
        for filename in files:
            process(os.path.join(root,filename))

if __name__=='__main__':
    main()