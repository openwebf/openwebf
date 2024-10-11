read_version() {
  VERSION_STR=$(cat webf.podspec | grep s.version | awk '{print $3}')
  END_POS=$(echo ${#VERSION_STR} - 2 | bc)
  export VERSION=${VERSION_STR:1:$END_POS}
}

ROOT=$(pwd)

if [ -L "libwebf.a" ]; then
  rm libwebf.a
  ln -s $ROOT/../../bridge/build/macos/lib/x86_64/libwebf.a
fi
