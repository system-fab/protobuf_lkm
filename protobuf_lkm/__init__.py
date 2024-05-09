import sys
import argparse

from .parser import parse_file
from .version import __version__
from .errors import Error


def _main():
    parser = argparse.ArgumentParser(description='Google Protobuf serialization/deserialization inside linux kernel.')

    parser.add_argument('-d', '--debug', action='store_true')
    parser.add_argument('--version',
                        action='version',
                        version=__version__,
                        help='Print version information and exit.')

    # Workaround to make the subparser required in Python 3.
    subparsers = parser.add_subparsers(title='subcommands', dest='subcommand')
    subparsers.required = True

    from .subparsers import generate_lkm_source

    generate_lkm_source.add_subparser(subparsers)

    args = parser.parse_args()

    if args.debug:
        args.func(args)
    else:
        try:
            args.func(args)
        except BaseException as e:
            sys.exit('error: ' + str(e))