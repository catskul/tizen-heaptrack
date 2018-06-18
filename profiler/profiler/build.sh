#!/bin/bash

usage()
{
	echo "Usage: $0 [CoreCLRDevelRPM]"
	echo "CoreCLRDevelRPM - (optional) path to coreclr-devel rpm package, which can be downloaded from download.tizen.org"
	echo "For manual CoreCLR builds, set \$CORECLR_SRC_DIR, \$CORECLR_BIN_DIR, and, optionally, \$ARCH and \
\$ROOTFS_DIR environment variables"
}

coreclr_devel_rpm=$(readlink -f "$1")

determine_coreclr_source() {
	if [ -d "$CORECLR_SRC_DIR" ] && [ -d "$CORECLR_BIN_DIR" ]; then
		coreclr_source="manual"
	elif [ -f "$coreclr_devel_rpm" ]; then
		coreclr_source="rpm"
	else
		usage
		exit 1
	fi
}

determine_cmake_arch() {
	supported_cmake_archs=("armel" "x64")
	supported_rpm_archs=("armv7l" "x86_64")
	default_arch="x64"
	if [ "$coreclr_source" == "manual" ]; then
		# determine architecture from env variable
		if [ -z "$ARCH" ]; then
			cmake_arch="$default_arch"
		else
			for supported_cmake_arch in $supported_cmake_archs; do
				if [ "$supported_cmake_arch" == "$ARCH" ]; then
					cmake_arch="$supported_cmake_arch"
					return
				fi
			done

			echo "Unsupported architecture '$ARCH'. Must be one of ${supported_cmake_archs[@]}."
		fi
	elif [ "$coreclr_source" == "rpm" ]; then
		# determine architecture from RPM package
		rpm_arch=$(echo $coreclr_devel_rpm | sed -r 's/.*\.([^\.]*)\.rpm/\1/g')
		arch_index=0
		for supported_rpm_arch in $supported_rpm_archs; do
			if [ "$rpm_arch" == "$supported_rpm_arch" ]; then
		    	cmake_arch=${supported_cmake_archs[arch_index]}
				return
			fi
			let arch_index=arch_index+1
		done
		echo "Unsupported architecture '$rpm_arch'. Must be one of ${supported_rpm_archs[@]}."
		exit 1
	else
		usage
		exit 1
	fi
}

prepare_coreclr() {
	if [ "$coreclr_source" == "rpm" ]; then
		rm -rf clr
		mkdir clr
		cd clr
		clr_dir=$(pwd)
		rpm2cpio $coreclr_devel_rpm | cpio -idmv
		CORECLR_BIN_DIR=$clr_dir/usr/share/dotnet/shared/Microsoft.NETCore.App/2.0.0/
		CORECLR_SRC_DIR=$clr_dir/usr/share/dotnet/shared/Microsoft.NETCore.App/2.0.0/
		cd ..
	fi
}

determine_coreclr_source
determine_cmake_arch
prepare_coreclr

toolchain_file=""
if [ ! -z "$ROOTFS_DIR" ] && [ -d $ROOTFS_DIR ]; then
	if [[ $cmake_arch == 'armel' ]]; then
		toolchain_file=$(pwd)/cross/armel/toolchain.cmake
	else
		echo "Unsupported architecture. Cross-compilation is only supported for armel targets."
		exit 1
	fi
fi

rm -rf build
mkdir build
cd build
if [ ! -z $toolchain_file ]; then
	CC=clang CXX=clang++ ROOTFS_DIR=$ROOTFS_DIR cmake $(dirname $0)/.. -DCLR_ARCH=$cmake_arch \
					-DCLR_BIN_DIR=$CORECLR_BIN_DIR \
					-DCLR_SRC_DIR=$CORECLR_SRC_DIR \
					-DCMAKE_TOOLCHAIN_FILE=$toolchain_file
else
	CC=clang CXX=clang++ ROOTFS_DIR=$ROOTFS_DIR cmake $(dirname $0)/.. -DCLR_ARCH=$cmake_arch \
				-DCLR_BIN_DIR=$CORECLR_BIN_DIR \
				-DCLR_SRC_DIR=$CORECLR_SRC_DIR
fi
make
cd ..
