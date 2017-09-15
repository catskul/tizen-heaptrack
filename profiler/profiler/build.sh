#!/bin/bash

usage()
{
	echo "Usage: $0 [CoreCLRDevelRPM] [RootFS]"
	echo "CoreCLRDevelRPM - path to coreclr-devel rpm package, which can be downloaded from download.tizen.org"
	echo "RootFS - (optional) path to target's rootfs directory. For native builds, leave empty"
}

coreclr_devel_rpm=$(readlink -f $1)
rootfs=$(readlink -f $2)

if [ ! -f $coreclr_devel_rpm ]; then
	usage
	exit 1
fi

arch=$(echo $coreclr_devel_rpm | sed -r 's/.*\.([^\.]*)\.rpm/\1/g')
cmake_arch=x64
if [[ $arch == 'armv7l' ]]; then
	cmake_arch='armel'
elif [[ $arch == 'x86_64' ]]; then
	cmake_arch='x64'
else
	echo "Unsupported architecture. Only 'armv7l' and 'x86_64' are currently supported."
	exit 1
fi

toolchain_file=""
if [ ! -z "$rootfs" ] && [ -d $rootfs ]; then
	if [[ $arch == 'armv7l' ]]; then
		toolchain_file=$(pwd)/cross/armel/toolchain.cmake
	else
		echo "Unsupported architecture. Cross-compilation is only supported for armel targets."
		exit 1
	fi
fi

rm -rf clr
mkdir clr
cd clr
clr_dir=$(pwd)
rpm2cpio $coreclr_devel_rpm | cpio -idmv

cd ..

rm -rf build
mkdir build
cd build
if [ ! -z $toolchain_file ]; then
	CC=clang CXX=clang++ ROOTFS_DIR=$rootfs cmake $(dirname $0)/.. -DCLR_ARCH=$cmake_arch \
					-DCLR_BIN_DIR=$clr_dir/usr/share/dotnet/shared/Microsoft.NETCore.App/2.0.0/ \
					-DCLR_SRC_DIR=$clr_dir/usr/share/dotnet/shared/Microsoft.NETCore.App/2.0.0/ \
					-DCMAKE_TOOLCHAIN_FILE=$toolchain_file
else
	CC=clang CXX=clang++ ROOTFS_DIR=$rootfs cmake $(dirname $0)/.. -DCLR_ARCH=$cmake_arch \
				-DCLR_BIN_DIR=$clr_dir/usr/share/dotnet/shared/Microsoft.NETCore.App/2.0.0/ \
				-DCLR_SRC_DIR=$clr_dir/usr/share/dotnet/shared/Microsoft.NETCore.App/2.0.0/
fi
make
cd ..
