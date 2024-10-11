#
# To learn more about a Podspec see http://guides.cocoapods.org/syntax/podspec.html.
#
Pod::Spec.new do |s|
  s.name             = 'webf'
  s.version          = '0.14.0'
  s.summary          = 'Build flutter apps with HTML/CSS and JavaScript.'
  s.description      = <<-DESC
  WebF (Web on Flutter) is a W3C standards-compliant web rendering engine based on Flutter, allowing web applications to run natively on Flutter.
                       DESC
  s.homepage         = 'https://openwebf.com'
  s.license          = { :file => '../LICENSE' }
  s.author           = { 'OpenWebF' => 'dongtiangche@outlook.com' }
  s.source           = { :path => '.' }
  s.source_files = 'Classes/**/*'
  s.public_header_files = 'Classes/**/*.h'
  s.dependency 'Flutter'
  s.platform = :ios, '11.0'
  s.library = 'c++'
  s.prepare_command = 'bash prepare.sh'
  s.pod_target_xcconfig = {
    'DEFINES_MODULE' => 'YES',
    # Flutter.framework does not contain a i386 slice.
    'EXCLUDED_ARCHS[sdk=iphonesimulator*]' => 'x86_64',
    'OTHER_LDFLAGS' => '-force_load ' + __dir__ + '/libwebf.a',
  }
end
