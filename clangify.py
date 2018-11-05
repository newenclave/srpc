import os
import re
import argparse

EXTENTIONS = { ".c", ".cpp", ".h", ".hpp" }

def cd(): 
    return os.path.dirname(os.path.realpath(__file__))

def runformat(exe, file):
    command = '%s -style=file -i -verbose "%s"' % (exe, file)
    print(f"Execute: {command}")
    os.system(command)

class ignores:
    def __init__(self, filename=".clang-format-ignore"):
        try:
            self.path = os.path.join(cd(), filename)
            with open(self.path) as f:
                content = f.readlines()
            self.content = [re.compile(x.strip()) for x in content]
            print("got %d ignore lines" % len(self.content))
        except:
            self.content = []

    def match(self, str):
        for r in self.content: 
            if r.match(str): 
                return True
        return False


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument('--clangify', 
        help="clang-format executable", 
        required=False, 
        default="clang-format")
    parser.add_argument('--ignores', 
        help="file with ignores", 
        required=False, 
        default=".clang-format-ignore")

    args = parser.parse_args()
    ignores = ignores(args.ignores)
    for (dirpath, dirnames, filenames) in os.walk(cd()):
        for file in filenames: 
            fullpath = os.path.join(dirpath, file)
            relpath = fullpath[len(cd()) + 1:]
            _, ext = os.path.splitext(relpath)
            if ext in EXTENTIONS:
                if ignores.match(relpath): 
                    print("File %s is in ignores. Skipping..." % fullpath)
                else:
                    print("Format file %s" % fullpath)
                    runformat(args.clangify, fullpath)
