/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

const { src, dest, series, parallel, task } = require('gulp');
const mkdirp = require('mkdirp');
const path = require('path');
const { readFileSync, writeFileSync, mkdirSync } = require('fs');
const { spawnSync, execSync, fork, spawn, exec } = require('child_process');
const { join, resolve } = require('path');
const { program } = require('commander');
const chalk = require('chalk');
const fs = require('fs');
const del = require('del');
const os = require('os');
const uploader = require('./utils/uploader');

program
.option('--static-quickjs', 'Build quickjs as static library and bundled into webf library.', false)
.option('--enable-log', 'Enable log printing')
.parse(process.argv);

const SUPPORTED_JS_ENGINES = ['v8', 'quickjs'];
const targetJSEngine = process.env.WEBF_JS_ENGINE || 'quickjs';

if (SUPPORTED_JS_ENGINES.indexOf(targetJSEngine) < 0) {
  throw new Error('Unsupported js engine:' + targetJSEngine);
}

const WEBF_ROOT = join(__dirname, '..');
const TARGET_PATH = join(WEBF_ROOT, 'targets');
const platform = os.platform();
const buildMode = process.env.WEBF_BUILD || 'Debug';
const paths = {
  targets: resolveWebF('targets'),
  scripts: resolveWebF('scripts'),
  example: resolveWebF('webf/example'),
  webf: resolveWebF('webf'),
  bridge: resolveWebF('bridge'),
  polyfill: resolveWebF('bridge/polyfill'),
  codeGen: resolveWebF('bridge/scripts/code_generator'),
  thirdParty: resolveWebF('third_party'),
  tests: resolveWebF('integration_tests'),
  sdk: resolveWebF('sdk'),
  templates: resolveWebF('scripts/templates'),
  performanceTests: resolveWebF('performance_tests')
};

const NPM = platform == 'win32' ? 'npm.cmd' : 'npm';
const pkgVersion = readFileSync(path.join(paths.webf, 'pubspec.yaml'), 'utf-8').match(/version: (.*)/)[1].trim();
const isProfile = process.env.ENABLE_PROFILE === 'true';

exports.paths = paths;
exports.pkgVersion = pkgVersion;

let winShell = null;
if (platform == 'win32') {
  winShell = path.join(process.env.ProgramW6432, '\\Git\\bin\\bash.exe');

  if (!fs.existsSync(winShell)) {
    return done(new Error(`Can not location bash.exe, Please install Git for Windows at ${process.env.ProgramW6432}. \n https://git-scm.com/download/win`));
  }
}

function resolveWebF(submodule) {
  return resolve(WEBF_ROOT, submodule);
}

task('clean', () => {
  execSync('git clean -xfd', {
    cwd: paths.example,
    env: process.env,
    stdio: 'inherit'
  });

  if (buildMode === 'All') {
    return del(join(TARGET_PATH, platform));
  } else {
    return del(join(TARGET_PATH, platform, buildMode.toLowerCase()));
  }
});

const libOutputPath = join(TARGET_PATH, platform, 'lib');

task('build-darwin-webf-lib', done => {
  let externCmakeArgs = [];
  let buildType = 'Debug';
  if (process.env.WEBF_BUILD === 'Release') {
    buildType = 'RelWithDebInfo';
  }

  if (isProfile) {
    externCmakeArgs.push('-DENABLE_PROFILE=TRUE');
  }

  if (process.env.ENABLE_ASAN === 'true') {
    externCmakeArgs.push('-DENABLE_ASAN=true');
  }

  if (process.env.USE_SYSTEM_MALLOC === 'true') {
    externCmakeArgs.push('-DUSE_SYSTEM_MALLOC=true');
  }

  // Bundle quickjs into webf.
  if (program.staticQuickjs) {
    externCmakeArgs.push('-DSTATIC_QUICKJS=true');
  }

  if (program.enableLog) {
    externCmakeArgs.push('-DENABLE_LOG=true');
  }

  execSync(`cmake -DCMAKE_BUILD_TYPE=${buildType} -DENABLE_TEST=true ${externCmakeArgs.join(' ')} \
    -G "Unix Makefiles" -B ${paths.bridge}/cmake-build-macos-x86_64 -S ${paths.bridge}`, {
    cwd: paths.bridge,
    stdio: 'inherit',
    env: {
      ...process.env,
      WEBF_JS_ENGINE: targetJSEngine,
      LIBRARY_OUTPUT_DIR: path.join(paths.bridge, 'build/macos/lib/x86_64')
    }
  });

  let webfTargets = ['webf'];
  if (targetJSEngine === 'quickjs' || targetJSEngine === 'v8') {
    webfTargets.push('webf_unit_test');
  }
  // if (buildMode === 'Debug') {
  //   webfTargets.push('webf_test');
  // }

  let cpus = os.cpus();
  execSync(`cmake --build ${paths.bridge}/cmake-build-macos-x86_64 --target ${webfTargets.join(' ')} -- -j ${cpus.length}`, {
    stdio: 'inherit'
  });

  const binaryPath = path.join(paths.bridge, `build/macos/lib/x86_64/libwebf.dylib`);

  if (buildMode == 'Release' || buildMode == 'RelWithDebInfo') {
    execSync(`dsymutil ${binaryPath}`, { stdio: 'inherit' });
    execSync(`strip -S -X -x ${binaryPath}`, { stdio: 'inherit' });
  }

  done();
});

task('run-bridge-unit-test', done => {
  if (platform === 'darwin') {
    execSync(`${path.join(paths.bridge, 'build/macos/lib/x86_64/webf_unit_test')}`, {stdio: 'inherit'});
  } else if (platform === 'linux') {
    execSync(`${path.join(paths.bridge, 'build/linux/lib/webf_unit_test')}`, {stdio: 'inherit'});
  } else if (platform == 'win32') {
    execSync(`${path.join(paths.bridge, 'build/windows/lib/webf_unit_test.exe')}`, {stdio: 'inherit'});
  }
   done();
});

task('compile-polyfill', (done) => {
  if (!fs.existsSync(path.join(paths.polyfill, 'node_modules'))) {
    spawnSync(NPM, ['install'], {
      cwd: paths.polyfill,
      stdio: 'inherit'
    });
  }

  let result = spawnSync(NPM, ['run', (buildMode === 'Release' || buildMode === 'RelWithDebInfo') ? 'build:release' : 'build'], {
    cwd: paths.polyfill,
    env: {
      ...process.env,
      WEBF_JS_ENGINE: targetJSEngine
    },
    stdio: 'inherit'
  });

  if (result.status !== 0) {
    return done(result.status);
  }

  done();
});


function matchError(errmsg) {
  return errmsg.match(/(Failed assertion|\sexception\s|Dart\nError)/i);
}

task('integration-test', (done) => {
  const childProcess = spawn('npm', ['run', 'test'], {
    stdio: 'pipe',
    cwd: paths.tests
  });

  let stdout = '';

  childProcess.stderr.pipe(process.stderr);
  childProcess.stdout.pipe(process.stdout);

  childProcess.stderr.on('data', (data) => {
    stdout += data + '';
  });

  childProcess.stdout.on('data', (data) => {
    stdout += data + '';
  });

  childProcess.on('error', (error) => {
    done(error);
  });

  childProcess.on('close', (code) => {
    let dartErrorMatch = matchError(stdout);
    if (dartErrorMatch) {
      let error = new Error('UnExpected Flutter Assert Failed.');
      done(error);
      return;
    }

    if (code === 0) {
      done();
    } else {
      // TODO: collect error message from stdout.
      const err = new Error('Some error occurred, please check log.');
      done(err);
    }
  });
});

task('plugin-test', (done) => {
  const childProcess = spawn('npm', ['run', 'plugin_test'], {
    stdio: 'pipe',
    cwd: paths.tests
  });

  let stdout = '';

  childProcess.stderr.pipe(process.stderr);
  childProcess.stdout.pipe(process.stdout);

  childProcess.stderr.on('data', (data) => {
    stdout += data + '';
  });

  childProcess.stdout.on('data', (data) => {
    stdout += data + '';
  });

  childProcess.on('error', (error) => {
    done(error);
  });

  childProcess.on('close', (code) => {
    let dartErrorMatch = matchError(stdout);
    if (dartErrorMatch) {
      let error = new Error('UnExpected Flutter Assert Failed.');
      done(error);
      return;
    }

    if (code === 0) {
      done();
    } else {
      // TODO: collect error message from stdout.
      const err = new Error('Some error occurred, please check log.');
      done(err);
    }
  });
});

task('unit-test', (done) => {
  const childProcess = spawn('flutter', ['test', '--coverage'], {
    stdio: 'pipe',
    cwd: paths.webf
  });

  let stdout = '';

  childProcess.stderr.pipe(process.stderr);
  childProcess.stdout.pipe(process.stdout);

  childProcess.stderr.on('data', (data) => {
    stdout += data + '';
  });

  childProcess.stdout.on('data', (data) => {
    stdout += data + '';
  });

  childProcess.on('error', (error) => {
    done(error);
  });

  childProcess.on('close', (code) => {
    let dartErrorMatch = matchError(stdout);
    if (dartErrorMatch) {
      let error = new Error('UnExpected Flutter Assert Failed.');
      done(error);
      return;
    }

    if (code === 0) {
      done();
    } else {
      done(new Error('Some error occurred, please check log.'));
    }
  });
});

task('unit-test-coverage-reporter', (done) => {
  const childProcess = spawn('npm', ['run', 'test:unit:report'], {
    stdio: 'inherit',
    cwd: WEBF_ROOT,
  });
  childProcess.on('exit', () => {
    done();
  });
});

task('sdk-clean', (done) => {
  execSync(`rm -rf ${paths.sdk}/build`, { stdio: 'inherit' });
  done();
});

function insertStringSlice(code, position, slice) {
  let leftHalf = code.substring(0, position);
  let rightHalf = code.substring(position);

  return leftHalf + slice + rightHalf;
}

function patchiOSFrameworkPList(frameworkPath) {
  const pListPath = path.join(frameworkPath, 'Info.plist');
  let pListString = fs.readFileSync(pListPath, {encoding: 'utf-8'});
  let versionIndex = pListString.indexOf('CFBundleVersion');
  if (versionIndex != -1) {
    let versionStringLast = pListString.indexOf('</string>', versionIndex) + '</string>'.length;

    pListString = insertStringSlice(pListString, versionStringLast, `
        <key>MinimumOSVersion</key>
        <string>11.0</string>`);
    fs.writeFileSync(pListPath, pListString);
  }
}

task(`build-ios-webf-lib`, (done) => {
  const buildType = (buildMode == 'Release' || buildMode === 'RelWithDebInfo')  ? 'RelWithDebInfo' : 'Debug';
  let externCmakeArgs = [];

  if (process.env.ENABLE_ASAN === 'true') {
    externCmakeArgs.push('-DENABLE_ASAN=true');
  }

  // Bundle quickjs into webf.
  if (program.staticQuickjs) {
    externCmakeArgs.push('-DSTATIC_QUICKJS=true');
  }

  if (process.env.USE_SYSTEM_MALLOC === 'true') {
    externCmakeArgs.push('-DUSE_SYSTEM_MALLOC=true');
  }

  if (program.enableLog) {
    externCmakeArgs.push('-DENABLE_LOG=true');
  }

  // generate build scripts for simulator
  execSync(`cmake -DCMAKE_BUILD_TYPE=${buildType} \
    -DCMAKE_TOOLCHAIN_FILE=${paths.bridge}/cmake/ios.toolchain.cmake \
    -DPLATFORM=SIMULATOR64 \
    -DDEPLOYMENT_TARGET=11.0 \
    -DIS_IOS=TRUE \
    ${isProfile ? '-DENABLE_PROFILE=TRUE \\' : '\\'}
    ${externCmakeArgs.join(' ')} \
    -DENABLE_BITCODE=FALSE -G "Unix Makefiles" -B ${paths.bridge}/cmake-build-ios-simulator-x86 -S ${paths.bridge}`, {
    cwd: paths.bridge,
    stdio: 'inherit',
    env: {
      ...process.env,
      WEBF_JS_ENGINE: targetJSEngine,
      LIBRARY_OUTPUT_DIR: path.join(paths.bridge, 'build/ios/lib/simulator_x86')
    }
  });
  // genereate build scripts for simulator arm64
  execSync(`cmake -DCMAKE_BUILD_TYPE=${buildType} \
    -DCMAKE_TOOLCHAIN_FILE=${paths.bridge}/cmake/ios.toolchain.cmake \
    -DPLATFORM=SIMULATORARM64 \
    -DDEPLOYMENT_TARGET=11.0 \
    -DIS_IOS=TRUE \
    ${isProfile ? '-DENABLE_PROFILE=TRUE \\' : '\\'}
    ${externCmakeArgs.join(' ')} \
    -DENABLE_BITCODE=FALSE -G "Unix Makefiles" -B ${paths.bridge}/cmake-build-ios-simulator-arm64 -S ${paths.bridge}`, {
    cwd: paths.bridge,
    stdio: 'inherit',
    env: {
      ...process.env,
      WEBF_JS_ENGINE: targetJSEngine,
      LIBRARY_OUTPUT_DIR: path.join(paths.bridge, 'build/ios/lib/simulator_arm64')
    }
  });

  let cpus = os.cpus();

  // build for simulator x86
  execSync(`cmake --build ${paths.bridge}/cmake-build-ios-simulator-x86 --target webf -- -j ${cpus.length}`, {
    stdio: 'inherit'
  });

  // build for simulator arm64
  execSync(`cmake --build ${paths.bridge}/cmake-build-ios-simulator-arm64 --target webf -- -j ${cpus.length}`, {
    stdio: 'inherit'
  });

  // Generate builds scripts for ARM64
  execSync(`cmake -DCMAKE_BUILD_TYPE=${buildType} \
    -DCMAKE_TOOLCHAIN_FILE=${paths.bridge}/cmake/ios.toolchain.cmake \
    -DPLATFORM=OS64 \
    -DDEPLOYMENT_TARGET=11.0 \
    -DIS_IOS=TRUE \
    ${externCmakeArgs.join(' ')} \
    ${isProfile ? '-DENABLE_PROFILE=TRUE \\' : '\\'}
    -DENABLE_BITCODE=FALSE -G "Unix Makefiles" -B ${paths.bridge}/cmake-build-ios-arm64 -S ${paths.bridge}`, {
    cwd: paths.bridge,
    stdio: 'inherit',
    env: {
      ...process.env,
      WEBF_JS_ENGINE: targetJSEngine,
      LIBRARY_OUTPUT_DIR: path.join(paths.bridge, 'build/ios/lib/arm64')
    }
  });

  // Build for ARM64
  execSync(`cmake --build ${paths.bridge}/cmake-build-ios-arm64 --target webf -- -j ${cpus.length}`, {
    stdio: 'inherit'
  });

  const targetSourceFrameworks = ['webf_bridge'];

  // If quickjs is not static, there will be another framework called quickjs.framework.
  if (!program.staticQuickjs) {
    targetSourceFrameworks.push('quickjs');
  }

  targetSourceFrameworks.forEach(target => {
    const arm64DynamicSDKPath = path.join(paths.bridge, `build/ios/lib/arm64/${target}.framework`);
    const simulatorX64DynamicSDKPath = path.join(paths.bridge, `build/ios/lib/simulator_x86/${target}.framework`);
    const simulatorArm64DynamicSDKPath = path.join(paths.bridge, `build/ios/lib/simulator_arm64/${target}.framework`);

    // Create flat simulator frameworks with multiple archs.
    execSync(`lipo -create ${simulatorX64DynamicSDKPath}/${target} ${simulatorArm64DynamicSDKPath}/${target} -output ${simulatorX64DynamicSDKPath}/${target}`, {
      stdio: 'inherit'
    });

    // CMake generated iOS frameworks does not contains <MinimumOSVersion> key in Info.plist.
    patchiOSFrameworkPList(simulatorX64DynamicSDKPath);;
    patchiOSFrameworkPList(arm64DynamicSDKPath);

    const targetDynamicSDKPath = `${paths.bridge}/build/ios/framework`;
    const frameworkPath = `${targetDynamicSDKPath}/${target}.xcframework`;
    mkdirp.sync(targetDynamicSDKPath);

    // dSYM file are located at /path/to/webf/build/ios/lib/${arch}/target.dSYM.
    // Create dSYM for simulator.
    execSync(`dsymutil ${simulatorX64DynamicSDKPath}/${target} --out ${simulatorX64DynamicSDKPath}/../${target}.dSYM`, { stdio: 'inherit' });
    // Create dSYM for arm64,armv7.
    execSync(`dsymutil ${arm64DynamicSDKPath}/${target} --out ${arm64DynamicSDKPath}/../${target}.dSYM`, { stdio: 'inherit' });

    // Generated xcframework at located at /path/to/webf/build/ios/framework/${target}.xcframework.
    // Generate xcframework with dSYM.
    if (buildMode === 'RelWithDebInfo') {
      execSync(`xcodebuild -create-xcframework \
        -framework ${simulatorX64DynamicSDKPath} -debug-symbols ${simulatorX64DynamicSDKPath}/../${target}.dSYM \
        -framework ${arm64DynamicSDKPath} -debug-symbols ${arm64DynamicSDKPath}/../${target}.dSYM -output ${frameworkPath}`, {
        stdio: 'inherit'
      });
    } else {
      execSync(`xcodebuild -create-xcframework \
        -framework ${simulatorX64DynamicSDKPath} \
        -framework ${arm64DynamicSDKPath} -output ${frameworkPath}`, {
        stdio: 'inherit'
      });
    }
  });
  done();
});

task('build-linux-webf-lib', (done) => {
  const buildType = buildMode == 'Release' ? 'Release' : 'RelWithDebInfo';
  const cmakeGeneratorTemplate = platform == 'win32' ? 'Ninja' : 'Unix Makefiles';

  let externCmakeArgs = [];

  if (process.env.USE_SYSTEM_MALLOC === 'true') {
    externCmakeArgs.push('-DUSE_SYSTEM_MALLOC=true');
  }

  if (program.enableLog) {
    externCmakeArgs.push('-DENABLE_LOG=true');
  }

  const soBinaryDirectory = path.join(paths.bridge, `build/linux/lib/`);
  const bridgeCmakeDir = path.join(paths.bridge, 'cmake-build-linux');
  // generate project
  execSync(`cmake -DCMAKE_BUILD_TYPE=${buildType} \
  ${externCmakeArgs.join(' ')} \
  ${isProfile ? '-DENABLE_PROFILE=TRUE \\' : '\\'}
  ${'-DENABLE_TEST=true \\'}
  -G "${cmakeGeneratorTemplate}" \
  -B ${paths.bridge}/cmake-build-linux -S ${paths.bridge}`,
    {
      cwd: paths.bridge,
      stdio: 'inherit',
      env: {
        ...process.env,
        WEBF_JS_ENGINE: targetJSEngine,
        LIBRARY_OUTPUT_DIR: soBinaryDirectory
      }
    });

  // build
  execSync(`cmake --build ${bridgeCmakeDir} --target webf ${buildMode != 'Release' ? 'webf_test' : ''} webf_unit_test -- -j 12`, {
    stdio: 'inherit'
  });

  const libs = [
    'libwebf.so'
  ];

  if (buildMode != 'Release') {
    libs.push('libwebf_test.so');
  }

  libs.forEach(lib => {
    const libkrakenPath = path.join(paths.bridge, `build/linux/lib/${lib}`);
    // Patch libkraken.so's runtime path.
    execSync(`chrpath --replace \\$ORIGIN ${libkrakenPath}`, { stdio: 'inherit' });
  });

  done();
});

task('generate-bindings-code', (done) => {
  if (!fs.existsSync(path.join(paths.codeGen, 'node_modules'))) {
    spawnSync(NPM, ['install'], {
      cwd: paths.codeGen,
      stdio: 'inherit'  
    });
  }

  let buildResult = spawnSync(NPM, ['run', 'build'], {
    cwd: paths.codeGen,
    env: {
      ...process.env,
    },
    shell: true,
    stdio: 'inherit'
  });

  if (buildResult.status !== 0) {
    return done(buildResult.status);
  }

  let compileResult = spawnSync('node', ['bin/code_generator', '-s', '../../core', '-d', '../../out', '-p', targetJSEngine], {
    cwd: paths.codeGen,
    env: {
      ...process.env,
    },
    shell: true,
    stdio: 'inherit'
  });

  if (compileResult.status !== 0) {
    return done(compileResult.status);
  }

  done();
});

task('build-window-webf-lib', (done) => {
  const buildType = buildMode == 'Release' ? 'RelWithDebInfo' : 'Debug';

  let externCmakeArgs = [];

  if (process.env.USE_SYSTEM_MALLOC === 'true') {
    externCmakeArgs.push('-DUSE_SYSTEM_MALLOC=true');
  }

  if (program.enableLog) {
    externCmakeArgs.push('-DENABLE_LOG=true');
  }

  const soBinaryDirectory = path.join(paths.bridge, `build/windows/lib/`);
  const bridgeCmakeDir = path.join(paths.bridge, 'cmake-build-windows');
  // generate project
  execSync(`cmake -DCMAKE_BUILD_TYPE=${buildType} ${externCmakeArgs.join(' ')} -B ${bridgeCmakeDir} -S ${paths.bridge}`,
    {
      cwd: paths.bridge,
      stdio: 'inherit',
      env: {
        ...process.env,
        WEBF_JS_ENGINE: targetJSEngine,
        LIBRARY_OUTPUT_DIR: soBinaryDirectory
      }
    });

  const webfTargets = ['webf'];

  // build
  execSync(`cmake --build ${bridgeCmakeDir} --target ${webfTargets.join(' ')} --verbose --config ${buildType}`, {
    stdio: 'inherit'
  });

  // Fix the output path
  const outputDir = path.join(paths.bridge, `build/windows/lib/${buildMode === 'Release' ? 'RelWithDebInfo' : 'Debug'}`);
  execSync(`copy ${outputDir}\\*.dll ${outputDir}\\..\\`);

  done();
});

task('build-android-webf-lib', (done) => {
  let ndkDir = '';

  // If ANDROID_NDK_HOME env defined, use it.
  if (process.env.ANDROID_NDK_HOME) {
    ndkDir = process.env.ANDROID_NDK_HOME;
  } else {
    let androidHome;
    if (process.env.ANDROID_HOME) {
      androidHome = process.env.ANDROID_HOME;
    } else if (platform == 'win32') {
      androidHome = path.join(process.env.LOCALAPPDATA, 'Android\\Sdk');
    } else if (platform == 'darwin') {
      androidHome = path.join(process.env.HOME, 'Library/Android/sdk')
    } else if (platform == 'linux') {
      androidHome = path.join(process.env.HOME, 'Android/Sdk');
    }
    const ndkVersion = '22.1.7171670';
    ndkDir = path.join(androidHome, 'ndk', ndkVersion);

    if (!fs.existsSync(ndkDir)) {
      throw new Error(`Android NDK version (${ndkVersion}) not installed.`);
    }
  }

  const archs = ['arm64-v8a', 'armeabi-v7a', 'x86'];
  const toolChainMap = {
    'arm64-v8a': 'aarch64-linux-android',
    'armeabi-v7a': 'arm-linux-androideabi',
    'x86': 'i686-linux-android'
  };
  const buildType = (buildMode === 'Release' || buildMode == 'RelWithDebInfo') ? 'RelWithDebInfo' : 'Debug';
  let externCmakeArgs = [];

  if (process.env.ENABLE_ASAN === 'true') {
    externCmakeArgs.push('-DENABLE_ASAN=true');
  }

  if (program.enableLog) {
    externCmakeArgs.push('-DENABLE_LOG=true');
  }

  if (process.env.USE_SYSTEM_MALLOC === 'true') {
    externCmakeArgs.push('-DUSE_SYSTEM_MALLOC=true');
  }

  // Bundle quickjs into webf.
  if (program.staticQuickjs) {
    externCmakeArgs.push('-DSTATIC_QUICKJS=true');
  }

  const soFileNames = [
    'libwebf',
    'libc++_shared'
  ];

  // If quickjs is not static, there will be another so called libquickjs.so.
  if (!program.staticQuickjs) {
    soFileNames.push('libquickjs');
  }

  const cmakeGeneratorTemplate = platform == 'win32' ? 'Ninja' : 'Unix Makefiles';
  archs.forEach(arch => {
    const soBinaryDirectory = path.join(paths.bridge, `build/android/lib/${arch}`);
    const bridgeCmakeDir = path.join(paths.bridge, 'cmake-build-android-' + arch);
    // generate project
    execSync(`cmake -DCMAKE_BUILD_TYPE=${buildType} \
    -DCMAKE_TOOLCHAIN_FILE=${path.join(ndkDir, '/build/cmake/android.toolchain.cmake')} \
    -DANDROID_NDK=${ndkDir} \
    -DIS_ANDROID=TRUE \
    -DANDROID_ABI="${arch}" \
    ${isProfile ? '-DENABLE_PROFILE=TRUE \\' : '\\'}
    ${externCmakeArgs.join(' ')} \
    -DANDROID_PLATFORM="android-18" \
    -DANDROID_STL=c++_shared \
    -G "${cmakeGeneratorTemplate}" \
    -B ${paths.bridge}/cmake-build-android-${arch} -S ${paths.bridge}`,
      {
        cwd: paths.bridge,
        stdio: 'inherit',
        env: {
          ...process.env,
          WEBF_JS_ENGINE: targetJSEngine,
          LIBRARY_OUTPUT_DIR: soBinaryDirectory
        }
      });

    // build
    execSync(`cmake --build ${bridgeCmakeDir} --target webf -- -j 12`, {
      stdio: 'inherit'
    });

    // Copy libc++_shared.so to dist from NDK.
    const libcppSharedPath = path.join(ndkDir, `./toolchains/llvm/prebuilt/${os.platform()}-x86_64/sysroot/usr/lib/${toolChainMap[arch]}/libc++_shared.so`);
    execSync(`cp ${libcppSharedPath} ${soBinaryDirectory}`);

    // Strip release binary in release mode.
    if (buildMode === 'Release' || buildMode === 'RelWithDebInfo') {
      const strip = path.join(ndkDir, `./toolchains/llvm/prebuilt/${os.platform()}-x86_64/bin/llvm-strip`);
      const objcopy = path.join(ndkDir, `./toolchains/llvm/prebuilt/${os.platform()}-x86_64/bin/llvm-objcopy`);

      for (let soFileName of soFileNames) {
        const soBinaryFile = path.join(soBinaryDirectory, soFileName + '.so');
        execSync(`${objcopy} --only-keep-debug "${soBinaryFile}" "${soBinaryDirectory}/${soFileName}.debug"`);
        execSync(`${strip} --strip-debug --strip-unneeded "${soBinaryFile}"`)
      }
    }
  });

  done();
});

task('android-so-clean', (done) => {
  execSync(`rm -rf ${paths.bridge}/build/android`, { stdio: 'inherit' });
  done();
});

task('ios-framework-clean', (done) => {
  execSync(`rm -rf ${paths.bridge}/build/ios`, { stdio: 'inherit' });
  done();
});

task('macos-dylib-clean', (done) => {
  execSync(`rm -rf ${paths.bridge}/build/macos`, { stdio: 'inherit' });
  done();
});

task('android-so-clean', (done) => {
  execSync(`rm -rf ${paths.bridge}/build/android`, { stdio: 'inherit', shell: winShell });
  done();
});

task('build-benchmark-app', async (done) => {
  execSync('npm install', { cwd: path.join(paths.performanceTests, '/benchmark') });
  const result = spawnSync('npm', ['run', 'build'], {
    cwd: path.join(paths.performanceTests, '/benchmark')
  });

  if (result.status !== 0) {
    return done(result.status);
  }

  done();
})

task('run-benchmark', async (done) => {
  const serverPort = '8892';

  const childProcess = spawn('http-server', ['./', `-p ${serverPort}`], {
    stdio: 'pipe',
    cwd: path.join(paths.performanceTests, '/benchmark/build')
  })

  let serverIpAddress;
  let interfaces = os.networkInterfaces();
  for (let devName in interfaces) {
    interfaces[devName].forEach((item) => {
      if (item.family === 'IPv4' && !item.internal && item.address !== '127.0.0.1') {
        serverIpAddress = item.address;
      }
    })
  }

  if (!serverIpAddress) {
    const err = new Error('The IP address was not found.');
    done(err);
  }

  let androidDevices = getDevicesInfo();

  let performanceInfos = execSync(
    `flutter run -d ${androidDevices[0].id} --profile --dart-define="SERVER=${serverIpAddress}:${serverPort}" | grep Performance`,
    {
      cwd: paths.performanceTests
    }
  ).toString().split(/\n/);

  const KrakenPerformancePath = 'kraken-performance';
  for (let item in performanceInfos) {
    let info = performanceInfos[item];
    const match = /\[(\s?\d,?)+\]/.exec(info);
    if (match) {
      const viewType = item == 0 ? 'kraken' : 'web';
      try {
        let performanceDatas = JSON.parse(match[0]);
        // Remove the top and the bottom five from the final numbers to eliminate fluctuations, and calculate the average.
        performanceDatas = performanceDatas.sort().slice(5, performanceDatas.length - 5);

        // Save performance list to file and upload to OSS.
        const listFile = path.join(__dirname, `${viewType}-load-time-list.js`);
        fs.writeFileSync(listFile, `performanceCallback('${viewType}LoadtimeList', [${performanceDatas.toString()}]);`);

        let WebviewPerformanceOSSPath = `${KrakenPerformancePath}/${viewType}-loadtimeList.js`;
        await uploader(WebviewPerformanceOSSPath, listFile).then(() => {
          console.log(`Performance Upload Success: https://kraken.oss-cn-hangzhou.aliyuncs.com/${WebviewPerformanceOSSPath}`);
        }).catch(err => done(err));
        // Save performance data of Webview with kraken version.
        let WebviewPerformanceWithVersionOSSPath = `${KrakenPerformancePath}/${viewType}-${pkgVersion}-loadtimeList.js`;
        await uploader(WebviewPerformanceWithVersionOSSPath, listFile).then(() => {
          console.log(`Performance Upload Success: https://kraken.oss-cn-hangzhou.aliyuncs.com/${WebviewPerformanceWithVersionOSSPath}`);
        }).catch(err => done(err));
      } catch {
        const err = new Error('The performance info parse exception.');
        done(err);
      }
    }
  }

  execSync('adb uninstall com.example.performance_tests');

  done();
});

task('migrate_to_v8', async (done) => {



  done();
});

function getDevicesInfo() {
  let output = JSON.parse(execSync('flutter devices --machine', {stdio: 'pipe', encoding: 'utf-8'}));
  let androidDevices = output.filter(device => {
    return device.sdk.indexOf('Android') >= 0;
  });
  if (androidDevices.length == 0) {
    throw new Error('Can not find android benchmark devices.');
  }
  return androidDevices;
}
