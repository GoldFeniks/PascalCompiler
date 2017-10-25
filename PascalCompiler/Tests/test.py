from os import listdir
import re
import subprocess
import argparse
import os

parser = argparse.ArgumentParser(description='Compiler tests')
parser.add_argument('dir', metavar='dir', type=str, help='Tests path');
parser.add_argument('argument', metavar='argument', type=str, help='An argument for compiler');
args = parser.parse_args()

r = re.compile('(?P<name>.+)\.pas')
f = re.compile('(?P<name>[^.]+)$')
c, w = 0, 0

def test(d):
    global c, w
    for i in listdir(d):
        name = r.match(i)
        if name:
            subprocess.call(['../../x64/Debug/PascalCompiler.exe', '-' + args.argument, d + '/' + i, 'output.txt'])
            f1, f2 = open(d + '/{}.result'.format(name.group('name'))), open('output.txt')
            a, b = f1.read(), f2.read();
            if a != b:
                print('Test {}/{} failed'.format(d, i))
                print(b, file=open(d + '/{}.result'.format(name.group('name')), 'w'), end='')
                w += 1
            else:
                print('Test {}/{} passed'.format(d, i))
                c += 1
            f1.close()
            f2.close()
            continue
        name = f.match(i)
        if name:
            print()
            test(d + '/' + name.group('name'))

test(args.dir)

print("{} tests passed. {} tests failed".format(c, w))

os.remove('output.txt')