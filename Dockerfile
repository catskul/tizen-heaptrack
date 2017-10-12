FROM ubuntu:17.04

RUN apt-get update && apt-get -y install cmake libsparsehash-dev \
  build-essential libunwind8-dev libunwind8 libboost-dev \
  libboost-iostreams-dev libboost-program-options-dev \
  zlib1g zlib1g-dev libdwarf-dev qt5-default extra-cmake-modules \
  libkf5coreaddons-dev libkf5i18n-dev libkf5itemmodels-dev \
  libkf5threadweaver-dev libkf5configwidgets-dev libkf5kiocore5 \
  libkf5kiowidgets5 kio-dev cpio rpm2cpio clang libkchart2 \
  libkchart-dev binutils-arm-linux-gnueabi gcc-arm-linux-gnueabi gettext \
  g++-arm-linux-gnueabi wget

#prepare folder structure
RUN mkdir -p /rootfs/rpms && \
    mkdir -p /heaptrack-common/x64 && \
    mkdir -p /heaptrack-common/armel && \
    mkdir -p /etc/sudoers.d && \
    mkdir -p /heaptrack/build-x64 && \
    mkdir -p /heaptrack/build-armel

#Add new sudo user
ENV USERNAME docker-user
RUN useradd -m $USERNAME && \
        echo "$USERNAME:$USERNAME" | chpasswd && \
        usermod --shell /bin/bash $USERNAME && \
        usermod -aG sudo $USERNAME && \
        echo "$USERNAME ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers.d/$USERNAME && \
        chmod 0440 /etc/sudoers.d/$USERNAME && \
        # Replace 1000 with your user/group id
        usermod  --uid 1000 $USERNAME && \
        groupmod --gid 1000 $USERNAME

#prepare tizen rootfs
ENV ROOTFS_DIR /rootfs
ENV REPO_BASE http://download.tizen.org/snapshots/tizen/base/latest/repos/standard/packages
ENV REPO_UNIFIED http://download.tizen.org/snapshots/tizen/unified/latest/repos/standard/packages
ENV TIZEN_PACKAGES_ARMEL "gcc glibc glibc-devel libunwind libunwind-devel zlib zlib-devel libdw libdw-devel boost boost-devel boost-iostreams boost-program-options"
ENV TIZEN_PACKAGES_PATTERN "-[0-9]+\(\.[0-9]+\)+-[0-9]+\(\.[0-9]+\)+\.armv7l\.rpm"

RUN cd /rootfs/rpms && \
    echo "PKGS=\"$TIZEN_PACKAGES_ARMEL\" ; for pkg in \$PKGS; do \
      echo Downloading \$pkg... ; \
      wget -q -r -np -l 1 --accept-regex \$pkg$TIZEN_PACKAGES_PATTERN $REPO_BASE/armv7l/ ; \
      wget -q -r -np -l 1 --accept-regex \$pkg$TIZEN_PACKAGES_PATTERN $REPO_UNIFIED/armv7l/ ; \
    done && \
    find . -name \"*.rpm\" -exec mv {} . \; && \
    rm -rf download.tizen.org \
" > download.sh && /bin/bash download.sh

RUN cd /rootfs && echo "RPMS=\$(ls rpms/*.rpm) ; for rpm in \$RPMS; do rpm2cpio \$rpm | cpio -idmv 2> /dev/null ; done" > unpack.sh && /bin/bash unpack.sh

COPY ./ /heaptrack/

#build x64 heaptrack with GUI
RUN cd /heaptrack/build-x64 && cmake .. && make -j4 && cp -r bin lib \
    /heaptrack-common/x64
#build armel heaptrack collection modules without GUI
RUN cd /heaptrack/build-armel && \
    CC=arm-linux-gnueabi-gcc CXX=arm-linux-gnueabi-g++ CPLUS_INCLUDE_PATH=/rootfs/usr/lib/gcc/armv7l-tizen-linux-gnueabi/6.2.1/include/c++:/rootfs/usr/lib/gcc/armv7l-tizen-linux-gnueabi/6.2.1/include/c++/armv7l-tizen-linux-gnueabi cmake .. -DHEAPTRACK_BUILD_GUI=OFF -DCMAKE_TOOLCHAIN_FILE=/heaptrack/profiler/profiler/cross/armel/toolchain.cmake && \
    make -j4 && cp -r bin lib /heaptrack-common/armel
#build managed profiler module
RUN cd /heaptrack/scripts/docker && ./build_profiler.sh /heaptrack-common/armel

VOLUME /heaptrack-common
