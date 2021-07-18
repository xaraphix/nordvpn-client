project('nordvpn-client', 'cpp',
  version : '0.1',
  default_options : [
    'warning_level=3', 
		'cpp_std=c++2a', 
    'build.cpp_std=c++2a',
    ])

dep = dependency('gtkmm-4.0')

executable('nordvpn-client',
           'main.cc',
           dependencies: dep,
           install: true)
