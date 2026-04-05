require 'json'

package = JSON.parse(File.read(File.join(__dir__, '..', 'package.json')))

Pod::Spec.new do |s|
  s.name         = 'JigNative'
  s.version      = package['version']
  s.summary      = 'React Native module for Jig — programmatic control of running apps'
  s.homepage     = 'https://github.com/jigtesting/jig'
  s.license      = { :type => 'MIT' }
  s.author       = 'Jig'
  s.source       = { :git => 'https://github.com/jigtesting/jig.git', :tag => s.version.to_s }

  s.platforms    = { :ios => '15.1' }
  s.swift_version = '5.4'

  s.dependency 'ExpoModulesCore'

  s.source_files = 'ios/**/*.{h,m,mm,swift}', 'core/**/*.{h,c}'
  s.exclude_files = 'ios/JigStandaloneInit.m', 'core/tests/**', 'core/build/**'

  # Note: libwebsockets integration for Expo/CocoaPods builds is deferred.
  # Standalone injection (Jig.framework) builds lws via CMake in build-framework.sh.
  # For Expo module builds, lws will be added as a vendored library or pod dependency
  # in a future slice.
end
