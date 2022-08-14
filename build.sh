#!/bin/sh

BUILDDIR=builddir-buildsh

if ! command -v meson > /dev/null
then
  echo "ERROR: meson not found"
  exit 1
fi

if ! command -v ninja > /dev/null
then
  echo "ERROR: ninja not found"
  exit 1
fi

if command -v clang > /dev/null
then
  export CC=clang
else
  echo "*** Warning: clang not found"
  echo "*** nameless works much better compiled with clang"
fi

if [ ! -d "$BUILDDIR" ]
then
  meson setup -Dprefix=/ "$BUILDDIR"
fi

DESTDIR="$PWD" ninja -C "$BUILDDIR" install
