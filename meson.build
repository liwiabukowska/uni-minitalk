project('minitalk', 'c')

if get_option('buildtype').startswith('debug')
add_project_arguments('-DDEBUG', language : 'c')
endif

openssl = subproject('openssl')

executable('minitalk-server',
    'src/server.c',

    'src/algorithm.c',
    'src/logs.c',
    'src/buffers.c',
    'src/netutils.c',
    dependencies: [
        openssl.get_variable('openssl_dep')
    ]
)

executable('minitalk-client',
    'src/client.c',

    'src/algorithm.c',
    'src/logs.c',
    'src/buffers.c',
    'src/netutils.c',
    dependencies: [
        openssl.get_variable('openssl_dep')
    ]
)