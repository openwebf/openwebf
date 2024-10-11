#
# To learn more about a Podspec see http://guides.cocoapods.org/syntax/podspec.html.
# Run `pod lib lint webf.podspec' to validate before publishing.
#
Pod::Spec.new do |s|
  s.name             = 'webf'
  s.version          = '0.1.0'
  s.summary          = 'A W3C standard compliant Web rendering engine based on Flutter.'
  s.description      = <<-DESC
A W3C standard compliant Web rendering engine based on Flutter..
                       DESC
  s.homepage         = 'https://openwebf.com'
  s.license          = { :file => '../LICENSE' }
  s.author           = { 'WebF' => 'dongtiangche@outlook.com' }
  s.source           = { :path => '.' }
  s.source_files     = 'Classes/**/*'
  s.public_header_files = 'Classes/**/*.h'
  s.dependency 'FlutterMacOS'
  s.prepare_command = 'bash prepare.sh'
  s.platform = :osx, '10.11'
  s.library = 'c++'
  s.pod_target_xcconfig = {
   'DEFINES_MODULE' => 'YES',
   'OTHER_LDFLAGS' => '-force_load ' + __dir__ + '/libwebf.a',
  }
  s.swift_version = '5.0'
end
