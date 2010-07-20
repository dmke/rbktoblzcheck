# Generated by jeweler
# DO NOT EDIT THIS FILE DIRECTLY
# Instead, edit Jeweler::Tasks in Rakefile, and run the gemspec command
# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = %q{rbktoblzcheck}
  s.version = "0.1.1"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Sascha Loetz", "Kim Rudolph"]
  s.date = %q{2010-07-20}
  s.default_executable = %q{kbc-ruby}
  s.email = %q{kim.rudolph@web.de}
  s.executables = ["kbc-ruby"]
  s.extensions = ["ext/extconf.rb"]
  s.extra_rdoc_files = [
    "README.rdoc"
  ]
  s.files = [
    "INSTALL",
     "LICENSE",
     "README.rdoc",
     "Rakefile",
     "VERSION.yml",
     "bin/kbc-ruby",
     "ext/extconf.rb",
     "ext/ktoblzcheck.c",
     "lib/ktoblzcheck.rb",
     "pkg/rbktoblzcheck-0.1.0.gem",
     "pkg/rbktoblzcheck-1.1.0.gem",
     "rbktoblzcheck.gemspec",
     "test/test-bankdata.txt",
     "test/test-ktoblzcheck-all.rb",
     "test/test-ktoblzcheck.rb"
  ]
  s.homepage = %q{http://github.com/krudolph/rbktoblzcheck}
  s.rdoc_options = ["--charset=UTF-8"]
  s.require_paths = ["lib", "ext"]
  s.rubygems_version = %q{1.3.7}
  s.summary = %q{rbktoblzcheck is an interface for libktoblzcheck, a library to check German account numbers and bank codes. See http://ktoblzcheck.sourceforge.net for details.}
  s.test_files = [
    "test/test-ktoblzcheck-all.rb",
     "test/test-ktoblzcheck.rb"
  ]

  if s.respond_to? :specification_version then
    current_version = Gem::Specification::CURRENT_SPECIFICATION_VERSION
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
    else
    end
  else
  end
end

