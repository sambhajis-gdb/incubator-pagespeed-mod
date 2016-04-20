#!/bin/sh
#
# Checks that an nginx release builds and passes tests.  This ensures that the
# PSOL tarball is good, and that it's compatible with the nginx code we intend
# to release.
#
# Usage:
#
#   verify_nginx_release.sh [version] [binary tarball]
#   verify_nginx_release.sh 1.10.33.6 /path/to/1.10.33.6.tar.gz
#
# To get the binary tarball, run build_psol_tarball.sh

set -e  # exit script if any command returns an error
set -u  # exit the script if any variable is uninitialized

if [ $# != 2 ]; then
  echo "Usage: $0 version /path/to/psol-binary-tarball"
  exit 1
fi

VERSION="$1"
TARBALL="$2"

if [ ! -f "$TARBALL" ]; then
  echo "$TARBALL should be a file"
  exit 1
fi

function die() {
  echo "verify_nginx_release.sh: $@"
  cd
  rm -rf "$WORKDIR"
  exit 1
}

WORKDIR=$(mktemp -d)
cd "$WORKDIR"

mkdir mod_pagespeed
cd mod_pagespeed
git clone https://github.com/pagespeed/mod_pagespeed.git src/
cd src/
git checkout $VERSION


cd $WORKDIR
git clone https://github.com/pagespeed/ngx_pagespeed.git
cd ngx_pagespeed
git checkout release-$VERSION-beta
tar -xzf "$TARBALL"

cd $WORKDIR
git clone https://github.com/FRiCKLE/ngx_cache_purge.git

wget https://openresty.org/download/openresty-1.9.7.3.tar.gz
tar xzvf openresty-*.tar.gz
cd openresty-*/
./configure --with-luajit
make

cd $WORKDIR
wget http://nginx.org/download/nginx-1.9.12.tar.gz

for is_debug in debug release; do
  cd $WORKDIR
  if [ -d nginx ]; then
    rm -rf nginx/
  fi
  tar -xzf nginx-1.9.12.tar.gz
  mv nginx-1.9.12 nginx
  cd nginx/

  cd $WORKDIR
  wget http://nginx.org/download/nginx-1.9.12.tar.gz
  tar -xzf nginx-1.9.12.tar.gz
  mv nginx-1.9.12 nginx
  cd nginx/
  extra_args=""
  if [ "$is_debug" == "debug" ]; then
    extra_args="--with-debug"
  fi

  nginx_root="$WORKDIR/$is_debug/"
  ./configure \
    --prefix="$nginx_root" \
    --add-module="$WORKDIR/ngx_pagespeed" \
    --add-module="$WORKDIR/ngx_cache_purge" \
    --add-module="$WORKDIR/openresty-*/build/ngx_devel_kit-*/" \
    --add-module="$WORKDIR/openresty-*/build/set-misc-nginx-*/" \
    --add-module="$WORKDIR/openresty-*/build/headers-more-nginx-module-*/" \
    --with-ipv6 \
    --with-http_v2_module \
    $extra_args

  make install

  cd "$WORKDIR"
  USE_VALGRIND=false \
    TEST_NATIVE_FETCHER=false \
    TEST_SERF_FETCHER=true \
    ngx_pagespeed/test/run_tests.sh 8060 8061 \
    $WORKDIR/mod_pagespeed \
    $nginx_root/sbin/nginx \
    modpagespeed.com

  cd
done

rm -rf "$WORKDIR"

echo "builds and tests completed successfully for both debug and release"