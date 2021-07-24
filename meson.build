project('nordvpn-client', 'cpp',
  version : '0.1',
  default_options : [
    'warning_level=3', 
		'cpp_std=c++2a', 
    'build.cpp_std=c++2a',
    ])

dep = dependency('gtkmm-4.0')
epoxy_dep = dependency('epoxy')


gnome = import('gnome')

if epoxy_dep.found()
  demo_resources = gnome.compile_resources('demo-resources',
    'demo.gresource.xml',
    source_dir: meson.current_source_dir(),
    install: false,
  )


  executable('nordvpn-client',
             'main.cc', demo_resources, 
             dependencies: [epoxy_dep, dep],
             install: true)
endif
