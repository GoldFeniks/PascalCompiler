from os import listdir
import re
import subprocess
import argparse
import os

parser = argparse.ArgumentParser(description='Compiler tests')
parser.add_argument('dir', metavar='dir', type=str, help='Tests path');
parser.add_argument('-co', '--check_output', help='Check exe output', action='store_true')
parser.add_argument('argument', metavar='argument', type=str, help='An argument for compiler');
args = parser.parse_args()

r = re.compile(r'(?P<name>.+)\.pas')
f = re.compile(r'(?P<name>[^.]+)$')
c, w = 0, 0
devnull = open(os.devnull, 'w')

def test(d):
    global c, w
    for i in listdir(d):
        name = r.match(i)
        if name:
            subprocess.call(['../../x64/Debug/PascalCompiler.exe', '-' + args.argument, d + '/' + i, 'output.txt'])
            a, b = '', ''
            if args.check_output:
                a = open(d + '/{}.output'.format(name.group('name'))).read()
                subprocess.call(['ml.exe', '/c', '/coff', 'output.txt'], stdout=devnull, stderr=devnull)
                subprocess.call(['link.exe', '/subsystem:console', 'output.obj'], stdout=devnull, stderr=devnull)
                b = subprocess.check_output(['output.exe']).decode('cp1251').replace('\r\n', '\n')
            else:
                a, b = open(d + '/{}.result'.format(name.group('name'))).read(), open('output.txt').read()
            if a != b:
                print('Test {}/{} failed'.format(d, i))
                if args.check_output:
                    print(b, file=open(d + '/{}.output'.format(name.group('name')), 'w'), end='')
                else:
                    print(b, file=open(d + '/{}.result'.format(name.group('name')), 'w'), end='')
                w += 1
            else:
                print('Test {}/{} passed'.format(d, i))
                c += 1
            continue
        name = f.match(i)
        if name:
            print()
            test(d + '/' + name.group('name'))

test(args.dir)

print("{} tests passed. {} tests failed".format(c, w))

os.remove('output.txt')
if args.check_output:
    os.remove('output.obj')
    os.remove('output.exe')