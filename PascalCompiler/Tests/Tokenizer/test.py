from os import listdir
import re
import subprocess

r = re.compile('(?P<name>.+)\.pas')

for i in listdir('./'):
    name = r.match(i)
    if name:
        subprocess.call(['../../../x64/Debug/PascalCompiler.exe', '-l', i, 'output.txt'])
        f1, f2 = open('{}.result'.format(name.group('name'))), open('output.txt')
        a, b = f1.read(), f2.read();
        if a != b:
            print('Test "{}" failed'.format(i))
            #print(b, file=open('{}.result'.format(name.group('name')), 'w'), end='')
        else:
            print('Test "{}" passed'.format(i))