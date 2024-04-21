project('sandbox', 'cpp', default_options : [
  'cpp_std=c++23',
  'cpp_eh=none',
  'warning_level=3',
])

fmt_dep = dependency('fmt')

src = [
   'src/main.cc',
   'src/server.cc',
   'src/str_util.cc',
   'src/router.cc',
   'src/response.cc',
   'src/request.cc',
   'src/file_descriptor.cc',
 ]
executable('main', src, dependencies: [fmt_dep], native: true)
