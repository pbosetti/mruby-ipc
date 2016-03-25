#!/usr/bin/env ruby

if __FILE__ == $PROGRAM_NAME
  require 'fileutils'
  FileUtils.mkdir_p 'tmp'
  unless File.exists?('tmp/mruby')
    system 'git clone --depth 1 https://github.com/mruby/mruby.git tmp/mruby'
  end
  system(%Q[cd tmp/mruby; MRUBY_CONFIG=#{File.expand_path __FILE__} ./minirake #{ARGV.join(' ')}])
  exit system %Q"ln -s tmp/mruby/bin/mirb ."
end

MRuby::Build.new do |conf|
  toolchain :clang
  #conf.cc.flags += %w(-fexceptions -Wno-deprecated-declarations)
  conf.cc.defines += %w(ENABLE_READLINE)
  conf.cc.include_paths << "/usr/local/include"
  conf.linker.library_paths << "/usr/local/lib"
  
  conf.gembox 'default'
  conf.gem :github => 'pbosetti/mruby-yaml',        :branch => 'master'
  conf.gem :github => 'pbosetti/mruby-sleep',       :branch => 'master'
  conf.gem :github => 'pbosetti/mruby-pcre-regexp', :branch => 'master'
  conf.gem :github => 'pbosetti/mruby-colorize',    :branch => 'master'
  conf.gem File.dirname(__FILE__)
end