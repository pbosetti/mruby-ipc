MRuby::Gem::Specification.new('mruby-ipc') do |spec|
  # Note that it needs WiringPI libraries (http://wiringpi.com)
  spec.license = 'MIT'
  spec.author  = 'Paolo Bosetti, University of Trento'
  spec.summary = 'IPC library via pipes'
  spec.version = 0.1
  spec.description = spec.summary
  spec.homepage = "Not yet defined"
  spec.add_dependency('mruby-pcre-regexp', '>= 0.0.0')
  spec.add_dependency('mruby-colorize', '>= 1.0.0')
  spec.add_dependency('mruby-sleep', '>= 0.0.0')
  
  if not build.kind_of? MRuby::CrossBuild then
    spec.cc.command = 'gcc' # clang does not work!
    spec.cc.flags << %w||
    spec.cc.include_paths << "/usr/local/include"
  
    spec.linker.library_paths << "/usr/local/lib"
  else
    # complete for your case scenario
  end
end
