# Tizen .NET Memory Profiler for Microsoft Visual Studio

## Introduction

Some time ago Milian Wolff created Heaptrack, an open-source memory profiler for C/C++ Linux applications (see <http://milianw.de/tag/heaptrack> and <https://github.com/KDE/heaptrack>). The profiler includes a utility which collects memory profiling data and saves them to a file and two tools for analyzing the resulting data: a command-line one and GUI-based one (heaptrack_gui). The latter application provides different views on the collected data: text-based views (a summary, different lists and tables) and graphical ones (a so-called flame graph, several charts, and an allocation histogram).
Original KDE Heaptrack's README can be found [here](docs/HEAPTRACK_README.md).

Later Samsung developers modified Heaptrack to support Samsung Tizen OS to enable profiling the memory consumption of managed .NET CoreCLR applications running under Tizen. The official name for this modification of heaptrack is Tizen .NET Memory Profiler. This tool can run under Linux only.
Original README for Tizen .NET Memory Profiler can be found [here](docs/TIZEN_HEAPTRACK_ORIGINAL_README.md).

The next step in evolution of Tizen .NET memory profiler is, porting its GUI to Windows platform and then integration into Visual Studio Tools for Tizen extension. Further in this document the Tizen .NET memory profiler for MS Visual Studio is described in more details.


## Prerequisites

### Prerequisites for building

Linux and Windows hosts will be needed in order to build all components of the Memory Profiler.

#### Windows host
In order to build the Memory profiler GUI, any Microsoft Windows 64-bit operating system starting from Windows 7 can be used.

Microsoft Visual Studio 2017 (<https://www.visualstudio.com/downloads>) or higher is required (Community Edition can be used). The components “VC++ 2017 v141 toolset (x86, x64)”, “Visual Studio extension development“ and “MSBuild” must be installed (select them in Visual Studio installer).

Tizen SDK is required.

The following additional software and libraries are required:

1. Microsoft Visual Studio 2017;
2. boost C++ libraries;
3. Qt 5 framework including some libraries, tools (*qmake*), and optionally IDE (*Qt Creator*);
4. QWT library and any SVN client to get it from its repository;
5. ThreadWeaver library and *git* software to get it from its repository.

Please see [here](WINDOWS_GUI_BUILD.md) for instructions about how to obtain and install these components.

#### Linux host
All build procedures were tested on the [Ubuntu 16.04](http://releases.ubuntu.com/16.04/)

The GBS build system must be installed to the Linux system. For more info about GBS installation please refer [here](https://source.tizen.org/documentation/developer-guide/getting-started-guide/installing-development-tools)


### Prerequisites for running
- VS Tools for Tizen. A compartible version of VS Tools for Tizen must be used which has the user interface for launching the Memory profiler.
- dotnet-launcher. A compartible version of dotnet-launcher must be used which has support for profiler launching.


## Build

The Tizen .NET Memory Profiler for Visual Studio consists of two components:
1) target-specific part (running on target side);
2) GUI (running on host side)


### Building target-specific part
The target specific part is built on Linux host  with GBS built system installed.

You should use compatible GBS config files to build the target-specific part for supported targets.

By default GBS store resulted RPMs and SRPMs at
`~/GBS-ROOT/local/repos/<distro>/<arch>` directories.

In the following examples option `-c` is used to specify path to the GBS config file suitable for target.

#### Build for ARM Tizen TM1 target

```console
~/heaptrack$ gbs -c ../gbs-config/gbs-tm1.conf build -A armv7l
```

#### Build for x86 Tizen emulator

```console
~/heaptrack$ gbs -c ../gbs-config/gbs-emul32.conf build -A i586
```

### Building GUI

The Memory profiler GUI is built on Windows, detailed instructions are provided [here](WINDOWS_GUI_BUILD.md).

## Install procedure

### Installing target-specific part

After successfull build on the Linux host, resulting target-specific RPMs will be located in the corresponding directories `~/GBS-ROOT/local/repos/<distro>/<arch>/RPMS`

The resulting RPMs shall be copied to the Windows host into the following specific directory:

`SDK\platforms\tizen-4.0\common\on-demand`

The RPMs will be auhomatically installed by on demand installation procedure.

Alternatively manual installation procedure can be also used:
```
> sdb push RPMS /tmp
> sdb root on
> sdb shell
sh-3.2# export TERM=windows
sh-3.2#  cd /tmp
sh-3.2# mount -o rw,remount /
sh-3.2# rpm --force -i <rpm_packet_name>.rpm
```


### Installing GUI
After successfull build on the Windows host, the following binaries will be available in the build directory:

```
TizenMemoryProfiler.exe
Qt5Core.dll
Qt5Gui.dll
Qt5OpenGL.dll
Qt5Svg.dll
Qt5Widgets.dll
qwt.dll
threadweaver.dll
imageformats\qjpeg.dll
platforms\qwindows.dll
styles\qwindowsvistastyle.dll
```

These files shall be copied to the following directory:
`SDK\tools\memory-profiler`

##Running 
