import os

GITIGNORE_FMT = '''
# MacOS
.DS_Store

# IDEs
**/.idea
**/.fleet
**/.vscode

# exe
**/main

# LKM artifacts
**/*.ko
**/*.o
**/*.o.d
**/*.o.cmd
**/*.mod.c
**/*.mod
**/*.mod.cmd
**/*.ko.cmd
**/*.cmd
**/modules.order
**/Module.symvers
**/cmake-build-debug

# keep go mod
!**/go.mod
'''


def generate_gitignore(output_directory):

    filename = os.path.join(output_directory, '.gitignore')

    with open(filename, 'w') as fout:
        fout.write(GITIGNORE_FMT)
