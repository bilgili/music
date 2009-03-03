from distutils.core import setup, Extension

module1 = Extension('music',
                    sources = ['pymusic.cc',
                               'setuptype.cc',
                               'runtimetype.cc'],
                    libraries = ['m', 'mpi_cxx', 'mpi', 'open-rte', 'open-pal',
                                 'dl', 'nsl', 'util', 'music'],
                    include_dirs = ['/usr/lib/openmpi/include',
                                    '/usr/local/include'],
                    library_dirs = ['/usr/lib/openmpi/lib',
                                    '/usr/local/lib'])

setup (name = 'music',
       version = '1.0',
       description = 'Multi-Simulation Coordinator',
       ext_modules = [module1])
