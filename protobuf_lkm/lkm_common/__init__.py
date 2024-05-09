import os
from enum import Enum

from .c_source import generate, Options
from .generate_gitignore import generate_gitignore
from .generate_makefile import generate_makefile
from .generate_pbtools_c import generate_pbtools_c
from .generate_pbtools_h import generate_pbtools_h
from ..lkm_netfilter.generate_main import generate_main_nf
from ..lkm_udp_sock.generate_main import generate_main_udp
from ..parser import SCALAR_VALUE_TYPES
from ..parser import camel_to_snake_case
from ..parser import parse_file


def generate_files(infiles,
                   import_paths=None,
                   output_directory='.',
                   options=None,
                   module_type='netfilter'):
    """Generate C source code from proto-file(s).

    """

    if options is None:
        options = Options()

    generate_path = output_directory + '/generated'
    os.makedirs(generate_path, exist_ok=True)

    for filename in infiles:
        parsed = parse_file(filename, import_paths)
        basename = os.path.basename(filename)
        name = camel_to_snake_case(os.path.splitext(basename)[0])

        filename_h = f'{name}.h'
        filename_c = f'{name}.c'

        header, source = generate(name, parsed, filename_h, options)
        filename_h = os.path.join(generate_path, filename_h)
        filename_c = os.path.join(generate_path, filename_c)

        with open(filename_h, 'w') as fout:
            fout.write(header)

        with open(filename_c, 'w') as fout:
            fout.write(source)

    # get name and path
    basename = os.path.basename(infiles[0])
    name = camel_to_snake_case(os.path.splitext(basename)[0])
    filename_h = f'{name}.h'
    filepath_h = os.path.join('generated', filename_h)

    # generate .gitignore
    generate_gitignore(output_directory=output_directory)

    # generate protobuf_lkm lib files
    generate_pbtools_c(output_directory=output_directory)
    generate_pbtools_h(output_directory=output_directory)

    # generate main.c
    if module_type == 'udp_socket':
        module_name = name + '_udp'
        generate_main_udp(module_name=module_name, import_path=filepath_h, output_directory=output_directory)
        generate_makefile(module_name=module_name, output_directory=output_directory, name=name, insmod_extra='&')
    else:
        module_name = name + '_nf'
        generate_main_nf(module_name=module_name, import_path=filepath_h, output_directory=output_directory)
        generate_makefile(module_name=module_name, output_directory=output_directory, name=name)
