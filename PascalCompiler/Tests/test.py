from os import listdir
import re
import subprocess
import argparse
import os

parser = argparse.ArgumentParser(description='Compiler tests')
parser.add_argument('dir', metavar='dir', type=str, help='Tests path');
parser.add_argument('-co', '--check_output', help='Check exe output', action='store_true')
parser.add_argument('-go', '--generate_output', help="Don't check anything. Just get output", action='store_true')
parser.add_argument('argument', metavar='argument', type=str, help='An argument for compiler');
parser.add_argument('-s', '--suffix', type=str, default='', help='Suffix to be added to files. Default is empty', const='', nargs='?')
args = parser.parse_args()

r = re.compile(r'(?P<name>.+)\.pas')
f = re.compile(r'(?P<name>[^.]+)$')
c, w = 0, 0
devnull = open(os.devnull, 'w')

def run():
    subprocess.call(['ml.exe', '/c', '/coff', 'output.txt'], stdout=devnull, stderr=devnull)
    subprocess.call(['link.exe', '/subsystem:console', 'output.obj'], stdout=devnull, stderr=devnull)
    return subprocess.check_output(['output.exe']).decode('cp1251').replace('\r\n', '\n')

def test(d):
    global c, w
    for i in listdir(d):
        name = r.match(i)
        if name:
            subprocess.call(['../../x64/Debug/PascalCompiler.exe', '-' + args.argument, d + '/' + i, 'output.txt']) 
            a, b = '', ''
            if args.check_output:
                b = run()
                if args.generate_output:
                    print(b, file=open(d + '/{}.output'.format(name.group('name') + args.suffix), 'w'), end='')
                    continue   
                a = open(d + '/{}.output'.format(name.group('name') + args.suffix)).read()
            else:
                b = open('output.txt').read()
                if args.generate_output:
                    print(b, file=open(d + '/{}.result'.format(name.group('name') + args.suffix), 'w'), end='')
                    continue
                a = open(d + '/{}.result'.format(name.group('name') + args.suffix)).read()
            if a != b:
                print('Test {}/{} failed'.format(d, i))
                if args.check_output:
                    print(b, file=open(d + '/{}.output'.format(name.group('name') + args.suffix), 'w'), end='')
                else:
                    print(b, file=open(d + '/{}.result'.format(name.group('name') + args.suffix), 'w'), end='')
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

if not args.generate_output:
    print("\n{} tests passed. {} tests failed".format(c, w))

os.remove('output.txt')
if args.check_output:
    os.remove('output.obj')
    os.remove('output.exe')