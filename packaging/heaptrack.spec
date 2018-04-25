Name:       heaptrack
Summary:    heaptrack for dotnet apps
Version:    1.1.0
Release:    0.1
Group:      Application Framework/Application State Management
License:    GPL
Source0:    %{name}-%{version}.tar.gz
Source1001: heaptrack.manifest
Source1002: 0001-Target-build
%define	heaptrack_src heaptrack-%{version}
%define	heaptrack_build build-%{_target_platform}
AutoReqProv: no

BuildRequires: cmake
BuildRequires: gcc
BuildRequires: clang
BuildRequires: glibc-devel
BuildRequires: libdw-devel
BuildRequires: libunwind-devel
BuildRequires: boost-devel
BuildRequires: boost-iostreams
BuildRequires: boost-program-options
BuildRequires: pkgconfig(zlib)
BuildRequires: coreclr-devel

# .NET Core Runtime
%define dotnet_version  2.0.0
%define dotnetdir       dotnet
%define netshareddir    %{dotnetdir}/shared
%define netcoreappdir   %{netshareddir}/Microsoft.NETCore.App/%{dotnet_version}

%description
Heaptrack for Tizen applications

%prep
%setup -q
# 0001-Target-build.patch
cp %{SOURCE1001} .
cp %{SOURCE1002} .
# Gbp-Patch-Macros
patch -p2 < 0001-Target-build

%build

echo _target _target_cpu _target_arch _target_os _target_platform
echo %{_target}
echo %{_target_cpu}
echo %{_target_arch}
echo %{_target_os}
echo %{_target_platform}

export CFLAGS="--target=%{_host}"
export CXXFLAGS="--target=%{_host}"

%define _heaptrack_build_conf RelWithDebInfo
%define _coreclr_devel_directory /usr/share/%{netcoreappdir}

cmake \
  -DCMAKE_INSTALL_PREFIX=%{_prefix} \
  -DCMAKE_BUILD_TYPE=%{_heaptrack_build_conf} \
  -DHEAPTRACK_BUILD_GUI=OFF \
	.

cd profiler; 
	ROOTFS_DIR=/ \
	CC=clang CXX=clang++ \
	cmake \
	-DCMAKE_TOOLCHAIN_FILE=profiler/cross/armel/toolchain.cmake \
	-DCLR_BIN_DIR=%{_coreclr_devel_directory} \
	-DCLR_SRC_DIR=%{_coreclr_devel_directory} \
	-DCLR_ARCH=%{_target_cpu} \
	profiler \
	; \
	make
cd -

make %{?jobs:-j%jobs} VERBOSE=1

%install
rm -rf %{buildroot}
%make_install

#mkdir -p %{buildroot}%{_native_lib_dir}
#ln -sf %{_libdir}/libheaptrack_preload.so.1 %{buildroot}%{_native_lib_dir}/libheaptrack_preload.so
#ln -sf %{_libdir}/libheaptrack_inject.so.1 %{buildroot}%{_native_lib_dir}/libheaptrack_inject.so

pwd
cp profiler/src/libprofiler.so %{buildroot}%{_prefix}/lib/heaptrack/libprofiler.so
echo %{buildroot}
ls %{buildroot}
echo %{_prefix}

%files
%manifest heaptrack.manifest
%{_prefix}/lib/heaptrack/libheaptrack_preload.so*
%{_prefix}/lib/heaptrack/libheaptrack_inject.so*
%{_prefix}/lib/heaptrack/libprofiler.so
%{_prefix}/lib/heaptrack/libexec/heaptrack_interpret

