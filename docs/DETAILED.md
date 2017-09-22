# Detailed Guide

To perform the measurements the following steps are necessary:

## Download and unpack VM disk image from this page

The VM contains scripts and tools to prepare Tizen device for measurements, and to perform the measurements.

The disk image is archived and split into several files.

To unpack it, please, download all the files to separate directory and:

*   on Linux run the following command in the directory:
 
```
cat ubuntu-17.04.vdi.tar.bz2-* | tar xjvf -
```

*   on Windows run the following command in the directory and unpack the created ubuntu-17.04.vdi.tar.bz2 using some archiver (for example, 7-zip from [7-zip.org](http://7-zip.org/)):

```
copy /B ubuntu-17.04.vdi.tar.bz2-31-08-17-aa + ubuntu-17.04.vdi.tar.bz2-31-08-17-ab + ubuntu-17.04.vdi.tar.bz2-31-08-17-ac + ubuntu-17.04.vdi.tar.bz2-31-08-17-ad + ubuntu-17.04.vdi.tar.bz2-31-08-17-ae + ubuntu-17.04.vdi.tar.bz2-31-08-17-af + ubuntu-17.04.vdi.tar.bz2-31-08-17-ag + ubuntu-17.04.vdi.tar.bz2-31-08-17-ah + ubuntu-17.04.vdi.tar.bz2-31-08-17-ai ubuntu-17.04.vdi.tar.bz2
```


The unpacked VM image has the following checksums:

```
ubuntu-17.04.vdi sha256sum 1afcea540149f76fae6e243839b6a21666725cc1409b4c259be82533e2a21a24
```

The checksum can be checked using `sha256sum ubuntu-17.04.vdi` on Linux, and using 7-zip (right-click file -> CRC -> SHA256) on Windows

## Create an Ubuntu-64 VirtualBox VM based on this disk image and run it.

The VM image was tested using VirtualBox 5.1.26 on Linux.
Login is ubuntu, and password is the same - ubuntu.

How to create VM:

1.  Run VirtualBox
2.  Click 'New'
3.  Enter a VM name, like 'CoreCLR Memory Profiling Tool'
4.  Choose Type: 'Linux'
5.  Choose Version: 'Ubuntu (64-bit)'
6.  Click 'Next'
7.  Choose memory size (at least 2048 MB)
8.  Click 'Next'
9.  Choose 'Use an existing virtual hard disk file'
10.  Click the 'Open File' graphical button next to ComboBox element. This will open dialog for choosing ubuntu-17.04.vdi (find and choose the file)
11.  Click 'Create'
12.  Double-click the VM name in list of VMs to start it

Minimal requirements: 2048 MB memory, 1 CPU core.

Recommended: 8192 MB memory, 4 CPU core.

Other settings (network, devices, etc.): USB should be enabled to connect Tizen device through sdb

## Connect your Tizen device through USB connection and attach it to the VirtualBox

Use the following menu of VirtualBox: Devices -> USB -> Samsung TIZEN device

## Prepare Tizen device for measurements

To make measurements possible, the device additionally to default packages, which are necessary to run .NET applications, should contain:

*   "debuginfo" RPMs (debug information) for libraries, which are used in the .NET applications, like coreclr, etc.

Please, make sure that versions of the RPMs equal to versions of the corresponding libraries.

If debug package is missing or can't be decoded, the corresponding functions in measurements report will be labelled `<unresolved function>`

"debugsource" RPMs are not necessary.

To prepare device for measurements the script can be used:

```
/home/ubuntu/heaptrack-scripts/prepare-device.sh
```


Before starting the script, please, make sure to put all the necessary RPM packages to **/home/ubuntu/device-rpms** in the VM.
Please, choose the versions of the binary and debuginfo packages for your device corresponding to each other ("debuginfo" for same versions, as binary packages).

For example, if you flash [http://download.tizen.org/snapshots/tizen/unified/latest/](http://download.tizen.org/snapshots/tizen/unified/latest/) to your Tizen device,
then you need debuginfo packages from [http://download.tizen.org/snapshots/tizen/unified/latest/repos/standard/debug/](http://download.tizen.org/snapshots/tizen/unified/latest/repos/standard/debug/)
At least, the following debuginfo packages are highly recommended:
- http://download.tizen.org/snapshots/tizen/unified/latest/repos/standard/debug/coreclr-debuginfo-[...].armv7l.rpm
- http://download.tizen.org/snapshots/tizen/unified/latest/repos/standard/debug/ecore-[...]-debuginfo-[...].armv7l.rpm
- http://download.tizen.org/snapshots/tizen/unified/latest/repos/standard/debug/edje-debuginfo-[...].armv7l.rpm
- http://download.tizen.org/snapshots/tizen/unified/latest/repos/standard/debug/eet-debuginfo-[...].armv7l.rpm
- http://download.tizen.org/snapshots/tizen/unified/latest/repos/standard/debug/eina-debuginfo-[...].armv7l.rpm
- http://download.tizen.org/snapshots/tizen/unified/latest/repos/standard/debug/elementary-debuginfo-[...].armv7l.rpm
- http://download.tizen.org/snapshots/tizen/unified/latest/repos/standard/debug/security-manager-debuginfo-[...].armv7l.rpm
- http://download.tizen.org/snapshots/tizen/unified/latest/repos/standard/debug/capi-[...]-debuginfo-[...].armv7l.rpm

The prepare-device.sh script will install the packages to device, and also make changes as implemented in /home/ubuntu/heaptrack-scripts/prepare-device-internal.sh script to remount root file system into read-write mode, and to move some directories from rootfs to /opt/usr to free some disk space on rootfs for installation of other packages.

Please, use the script only once - don't run it second time until the device is reset to its initial state.

To make sure the device was prepared correctly, please, check the following:
- /usr/share/dotnet directory is moved to /opt/usr/dotnet
- /usr/shared/dotnet.tizen is moved to /opt/usr/dotnet.tizen
- /usr/lib/debug is moved to /opt/usr/lib/debug
- `ls -Z` shows "_" smack attribute for all contents of the three directories
- there are symlinks created for the directories to their initial locations (i.e. /usr/share/dotnet symlink to /opt/usr/dotnet etc.)
- `rpm -qa | grep debuginfo` shows all the debuginfo packages that you planned to install

## Build profiler module for your CoreCLR version

By default the libprofiler.so is already built for coreclr-2.0.0.11992-11.1.armv7l, so if you use this version,
then you don't need to rebuild it.

**WARNING:**

**If the libprofiler.so doesn't match the CoreCLR version, then managed memory and managed call stacks will not be profiled.**

1.  Find out version of CoreCLR package on your Tizen device, like coreclr-2.0.0.11992-11.1.armv7l
2.  Download corresponding "devel" package, like coreclr-devel-2.0.0.11992-11.1.armv7l.rpm
3.  Use the "devel" package to build profiler module (on armel system or under armel rootfs), using contents of the directory from the VM:
    1.  /home/ubuntu/heaptrack-common/build-profiler-armel (see build.sh in the directory)
4.  Put the compiled libprofiler.so (build-profiler-armel/build/src/libprofiler.so) module to /home/ubuntu/heaptrack-common/build-armel/libprofiler.so

## Run measurements

Setup completed! Now, we can run memory consumption measurements.

**Note: This way of measurements assumes that .NET application starts based on dotnet-launcher.**

To start measurements, run application (application will start and works as usual, although noticeably/significantly (30x to 100x) slower because of profiling)

```
/home/ubuntu/heaptrack-scripts/heaptrack.sh [application ID] [application path]
```

like the following:

```
 ./heaptrack-scripts/heaptrack.sh org.tizen.example.HelloWorld.Tizen /opt/usr/home/owner/apps_rw/HelloWorld.Tizen/bin/HelloWorld.Tizen.exe
```


After this, at any moment, you can stop application to get the memory profiling results for the moment of stopping.
To stop and get profiling results, press "c".

Upon stop, application is terminated, and profiling results are downloaded to VM.
When profiling results are downloaded, the measurements report is shown in GUI viewer.

The GUI will start separately for Malloc part, Managed part, mmap Private_Dirty part, mmap Private_Clean part and mmap Shared_Clean part.

Each part will be opened after the previous is closed.
