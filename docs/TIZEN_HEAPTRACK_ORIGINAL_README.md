# Tizen .NET Memory Profiler

(Original KDE Heaptrack's README can be found [here](docs/HEAPTRACK_README.md))

## Brief contents of the Guide

### Building and launching profiler in Docker

Prerequisites:
* [Tizen SDK](https://developer.tizen.org/development/tizen-studio/download) is required to run the profiler.
* Your host machine should have Docker installed. For Ubuntu instructions, please see [the manual](https://docs.docker.com/engine/installation/linux/docker-ce/ubuntu/).

Please, keep in mind:

> If you would like to use Docker as a non-root user, you should now consider
> adding your user to the "docker" group with something like:
> 
> `sudo usermod -aG docker your-user`

1. When building in Docker, the first thing you need to do is to create a `coreclr-devel` directory and put coreclr-devel rpms for the CoreCLR versions you use on your devices in this folder. You can find out which version of CoreCLR is installed on your device using `rpm -qa` command. Alternatively, you can let docker download the latest `coreclr-devel` package from download.tizen.org, or, if you are building CoreCLR yourself, point the build script to the compiled CoreCLR source folder. If you choose one of these two options, leave the `coreclr-devel` folder empty and proceed to step 2.
2. Run 
```
sudo ./launch.sh org.tizen.example.HelloWorld.Tizen /opt/usr/home/owner/apps_rw/HelloWorld.Tizen/bin/HelloWorld.Tizen.exe
```
to build the profiler and launch profiling of HelloWorld application. If you are running the profiler for the first time, you will be asked permission to move files around on the device in order to free up some disk space. You will also be asked to point the script to the location of "debuginfo" RPMs for the native libraries in which you want to see stack traces.

### VM [[Details]](docs/DETAILED.md#download-and-unpack-vm-disk-image-from-this-page)

1\. Download and unpack VM image, start it in VirtualBox and connect with Tizen device

To unpack it on Linux, please, download the ubuntu-17.04.vdi.tar.bz2-* files to separate directory and run the following command in the directory:

```
cat ubuntu-17.04.vdi.tar.bz2-* | tar xjvf -
```



For unpacking on Windows, please see [the details](docs/DETAILED.md#download-and-unpack-vm-disk-image-from-this-page).

* [ubuntu-17.04.vdi.tar.bz2-31-08-17-aa](http://suprem.sec.samsung.net/confluence/download/attachments/81831470/ubuntu-17.04.vdi.tar.bz2-31-08-17-aa?api=v2)
* [ubuntu-17.04.vdi.tar.bz2-31-08-17-ab](http://suprem.sec.samsung.net/confluence/download/attachments/81831470/ubuntu-17.04.vdi.tar.bz2-31-08-17-ab?api=v2)
* [ubuntu-17.04.vdi.tar.bz2-31-08-17-ac](http://suprem.sec.samsung.net/confluence/download/attachments/81831470/ubuntu-17.04.vdi.tar.bz2-31-08-17-ac?api=v2)
* [ubuntu-17.04.vdi.tar.bz2-31-08-17-ad](http://suprem.sec.samsung.net/confluence/download/attachments/81831470/ubuntu-17.04.vdi.tar.bz2-31-08-17-ad?api=v2)
* [ubuntu-17.04.vdi.tar.bz2-31-08-17-ae](http://suprem.sec.samsung.net/confluence/download/attachments/81831470/ubuntu-17.04.vdi.tar.bz2-31-08-17-ae?api=v2)
* [ubuntu-17.04.vdi.tar.bz2-31-08-17-af](http://suprem.sec.samsung.net/confluence/download/attachments/81831470/ubuntu-17.04.vdi.tar.bz2-31-08-17-af?api=v2)
* [ubuntu-17.04.vdi.tar.bz2-31-08-17-ag](http://suprem.sec.samsung.net/confluence/download/attachments/81831470/ubuntu-17.04.vdi.tar.bz2-31-08-17-ag?api=v2)
* [ubuntu-17.04.vdi.tar.bz2-31-08-17-ah](http://suprem.sec.samsung.net/confluence/download/attachments/81831470/ubuntu-17.04.vdi.tar.bz2-31-08-17-ah?api=v2)
* [ubuntu-17.04.vdi.tar.bz2-31-08-17-ai](http://suprem.sec.samsung.net/confluence/download/attachments/81831470/ubuntu-17.04.vdi.tar.bz2-31-08-17-ai?api=v2)

[[SHA256 values for archive contents]](#checksums)

### Initialization of device for measurements [[Details]](docs/DETAILED.md#prepare-tizen-device-for-measurements)

2\. Put Tizen RPMs to VM's /home/ubuntu/device-rpms for:
- debuginfo packages

3\. Run /home/ubuntu/heaptrack-scripts/prepare-device.sh on VM

[Video tutorial](http://suprem.sec.samsung.net/confluence/download/attachments/81831470/How-to-prepare-Tizen-device-for-measurements.mp4?api=v2)


### Build profiler module for your CoreCLR version [[Details]](docs/DETAILED.md#build-profiler-module-for-your-coreclr-version)

By default the libprofiler.so is already built for coreclr-2.0.0.11992-11.1.armv7l, so if you use this version,
then you don't need to rebuild it.

For details, see "Build profiler module" in full contents below (section 5)

[Video tutorial](http://suprem.sec.samsung.net/confluence/download/attachments/81831470/How-to-build-managed-profiling-module.mp4?api=v2)

### Measurements [[Details]](docs/DETAILED.md#run-measurements)

8\. Make sure that "debuginfo" packages are installed for all libraries that you want to track by the profiler

9\. Run application using /home/ubuntu/heaptrack-scripts/heaptrack.sh with `[application ID]` and `[path to executable]` arguments on VM,
like the following:

```
./heaptrack-scripts/heaptrack.sh org.tizen.example.HelloWorld.Tizen /opt/usr/home/owner/apps_rw/HelloWorld.Tizen/bin/HelloWorld.Tizen.exe
```

[Video tutorial](http://suprem.sec.samsung.net/confluence/download/attachments/81831470/Profiling-memory-consumption.mp4?api=v2)

10\. Wait until a moment, in which you want to figure out the memory consumption.

Please, note that application runs significantly slower (30x to 100x slower) under profiling.

11\. Press 'c' on VM, wait for GUI to start and analyze the results
The GUI will start separately for **Malloc** part, **Managed** part, **mmap Private_Dirty** part, **mmap Private_Clean** part and **mmap Shared_Clean** part

[[Profiling results description]](#profiling-results)

[[Some screenshots]](#screenshots)

[[Solutions for possible issues]](#troubleshooting)

### Profiling results

The GUI viewer provides several views for the profiling results.

- If some functions are shown as `<unresolved function>` - most probably (with few exceptions, like libc internals) it means that "debuginfo" package for the corresponding library is missing or of wrong version.

- The functions, which are marked as `<untracked>` are the areas, for which locations is not known. This is a mark for areas, which were allocated before attaching profiler, or by ways, which are not trackable by current version of profiler (current profiler doesn't track allocations from ld-linux.so.3).

- The "leaks" in the interface are most probably not actual leaks - it is label for memory that wasn't freed when application was terminating. However, it is usual behaviour for many libraries, including CoreCLR to not free memory upon application termination, as kernel anyway frees it. So, "leaks" labels should be considered as labels for memory, which was consumed just before memory profiling stopped.

#### Summary view
Peak contributions shows top functions that consumed memory at moment of highest memory consumption.
Peak instances shows top functions that has most memory items allocated and not freed at moment of highest count of the memory items.
Largest memory leaks - top functions that allocated memory, which wasn't deallocated up to moment of memory profiling stop.
Most memory allocations - top functions that call allocation most number of times.
Most temporary allocations - the same for allocations, in which deallocation is called very close to allocation, like `p = malloc(...); simple code; free (p)`.
Most memory allocated - top functions, which allocated most memory (this doesn't account deallocations, so is not very useful for analysis of memory consumption causes).

#### Bottom-Up/Top-Down view
Memory consumption details for each detected call stack.

#### Caller / Callee view
Memory consumption statistics for each function for itself, and also inclusive (i.e. including statistics of other functions, which it called).

#### Flame Graph view
Also represents memory consumption per call-stack (either top-down or bottom-down mode)
Combobox allows to choose between different statistics (like in Summary view).
The "leaks" here also are not the memory leaks: it is memory that was consumed at moment of profiling stop.
So, the "leaks" view seems to be the most useful to investigate memory consumption details.

#### Other
Next several views are graphs for different memory consumption statistics (also, like in Summary view), as they change in time of application execution.
Here, the "consumed" is used instead of "leaks", which is more precise.
The "consumed" view shows memory consumption graph for top memory consuming function (separately, as shown in color, and also total consumption - the topmost graph).

## Troubleshooting

1\. The heaptrack is built for one of latest Tizen Unified TM1 system.
If it can't be started on your Tizen device, because of missing dependencies, please recompile it from sources (see /home/ubuntu/heaptrack-common/build-armel).
Recompilation is simple, like "mkdir build_dir; cd build_dir; cmake ..; make -j4" and should be performed on an armel system with compatible libraries or in a corresponding chroot.

2\. Managed memory consumption or managed call stacks data is missing

See [[Build profiler module]](docs/DETAILED.md#build-profiler-module-for-your-coreclr-version)

3\. Please, feel free to ask any questions, in case of the described or other issues ([https://github.sec.samsung.net/dotnet/profiler/issues/4](https://github.sec.samsung.net/dotnet/profiler/issues/4))

The profiling tool can show memory consumption of **Managed**, **Malloc** and **Mmap**-allocated regions.

## Screenshots

### malloc'ed memory over time
![malloc-Consumed-Graph.png](screenshots/malloc-Consumed-Graph.png)
*   Each color shows different function (function name is shown when mouse is over a part of graph)
*   The top part is sum of all malloc-allocated memory (for all allocating functions)
*   The graph is useful to get overall picture of how memory consumption changed as program executed
### Functions and their statistics
![malloc-Plain-Statistics.png](screenshots/malloc-Plain-Statistics.png)
*   Peak is memory consumption of particular function at overall maximum of application consumption (this considers only currently shown memory - i.e. only malloc, or mmap Private_Dirty, or mmap Private_Clean, etc.)
*   Leaked is memory consumption just at application exit (not actually, leak, as most application do not free memory at their exit - so, it is just information about memory consumption just before application exit)
*   Allocated is sum of all allocated memory sizes (doesn't account that some memory was already freed - counts only allocations)
*   "Incl." mark is for the function and all functions that it calls; "Self" mark is for the function only
### Call stacks and memory consumption at each point of call stack
![malloc-FlameGraph-Details.png](screenshots/malloc-FlameGraph-Details.png)
*   For example, "icu::Normalizer2Impl::ensureCanonIterData" calls "utrie2_enum" and "icu::CanonIterData::CanonIterData" and "utri2_freeze"
    *   The "utrie2_enum" and "icu::CanonIterData::CanonIterData" and "utri2_freeze" call other functions that in sum allocate 539, 283 and 80 kilobytes, correspondingly
    *   The consumption for "ensureCanonIterData" will be shown as 904 kB - it is sum of the three values
*   In another places (another call stacks) the functions can also be called and that memory consumption will be accounted separately for these and those call stacks
### Reversed FlameGraph (if "Bottom-Down View" is checked)
![malloc-FlameGraph-Reversed.png](screenshots/malloc-FlameGraph-Reversed.png)
*   A useful form of FlameGraph to show the call stacks reversed
    *   So, the bottom line will show the allocator functions - like malloc, calloc, realloc, etc. and lines above will be the functions that call the allocator functions
### Top-N lists of functions:
![malloc-Summary.png](screenshots/malloc-Summary.png)
*   functions that consumed most when peak of malloc memory consumption occured ("peak contributions")
*   functions that consumed most just before application exit ("largest memory leaks")
*   functions that called allocators more than others ("most memory allocations")
*   functions that called allocators more than others for temporary allocations ("most temporary allocations") - temporary is the case when malloc and free are called almost one after other (with no allocations between them)
*   functions that allocated most memory at sum ("most memory allocated") - just sum of allocations, without accounting freeing of memory
*   "peak RSS" is currently experimental and so is not precise
### Managed heap inspection
![managed-ReferenceTree.png](screenshots/managed-ReferenceTree.png)
*   Objects are grouped by their type
*   If type T2 is a child of type T1, it means that objects of type T2 reference objects of type T1
*   Shallow Size is the total size of objects in the reference chain
*   Referenced Size is the size of objects referenced by the parent object.
For example, `System.AppDomainSetup` references 76 bytes of `System.String`s in the screenshot through `System.Object[]`->`System.String[]` chain and 32 bytes of `System.String`s directly.
### mmap-allocated memory graphs
Most of the graphs listed above are also available for mmap-allocated memory.
![mmap-private-dirty-Plain-Statistics.png](screenshots/mmap-private-dirty-Plain-Statistics.png)
![mmap-private-dirty-Consumed-Graph.png](screenshots/mmap-private-dirty-Consumed-Graph.png)
*   the mmap-allocated memory is divided into four groups (as in /proc/.../smaps): Private_Dirty, Private_Clean, Shared_Clean + Shared_Dirty
    *   Private_Dirty is process-local (not shared) memory that was modified by the process
    *   Private_Clean is process-local (not shared) memory that was loaded from disk and not modified by the process
    *   Shared_Clean is shared between processes memory, not modified by the process
    *   Shared_Clean is shared between processes memory, modified by the process
    *   the groups are chosen by --private_dirty, --private_clean, --shared keys of heaptrack_gui (--malloc is for malloc memory consumption and --managed is for managed memory consumption)
*   there are two additional groups, which are accounted indirectly
    *   dlopen (memory, used for loaded libraries)
    *   sbrk (memory, used for "[heap]") - this includes:
        *   libc allocator (malloc-allocated memory, can be investigated through `heaptrack_gui --malloc` run)
        *   overhead of profiler
        *   possibly, other allocators if they use `sbrk` (this is unusual)

## Checksums

SHA256SUM for unpacked VM image

```
1afcea540149f76fae6e243839b6a21666725cc1409b4c259be82533e2a21a24  ubuntu-17.04.vdi
```

SHA256SUM for archive

```
e8de003daa38880e37d847af462b86145ccf859f0af127e9a18dcc7c7cc04deb  ubuntu-17.04.vdi.tar.bz2
```

SHA256SUM for archive parts
```
7da95143eb387f94d09c1a34a952bcdc05844cbf064ba971884b6107f2665e55  ubuntu-17.04.vdi.tar.bz2-31-08-17-aa
34bec202f4521fc3857fa57c7acb07910f02df788add8a01dc03b09bc732895f  ubuntu-17.04.vdi.tar.bz2-31-08-17-ab
a03cd105647fd225fcbc85310490032f96c23c9e55289fe88341daf8db560236  ubuntu-17.04.vdi.tar.bz2-31-08-17-ac
788790f51a237273eff96e102a6ad39fdd0008dcfcbbe537d80a1ab543cbcd7c  ubuntu-17.04.vdi.tar.bz2-31-08-17-ad
a165a9642e8d58536b4c944e12fe23bd666d77cd74b3fce1b050c01308a4b887  ubuntu-17.04.vdi.tar.bz2-31-08-17-ae
7bf1495ae705a6a32577c4b1982c18230338eaa4b7cd5079b87a376d99b77b48  ubuntu-17.04.vdi.tar.bz2-31-08-17-af
6fea617bf0833bb841317c56674e6b34c09a145fc5a95f4ce1ea202dc6f4b56a  ubuntu-17.04.vdi.tar.bz2-31-08-17-ag
21389420ce2dcc0cd86d1b2c0872cb116e18ce226e1bab111e00536ed5be17f1  ubuntu-17.04.vdi.tar.bz2-31-08-17-ah
58a3950e44c37f9b825d13ff738e75544a3a2e0bca8f5a74ce877cbbee7a141b  ubuntu-17.04.vdi.tar.bz2-31-08-17-ai
```
