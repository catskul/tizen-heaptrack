#!/bin/bash -x

mount -o remount,rw /;
mv /usr/share/dotnet /opt/usr/dotnet;
mv /usr/share/dotnet.tizen /opt/usr/dotnet.tizen;
chsmack -a _ /opt/usr/dotnet; chsmack -a _ /opt/usr/dotnet.tizen;

for i in `find /opt/usr/dotnet/`; do chsmack -a _ $i; done;
for i in `find /opt/usr/dotnet.tizen/`; do chsmack -a _ $i; done;

ln -s /opt/usr/dotnet /usr/share/dotnet;
chsmack -a _ /usr/share/dotnet;
ln -s /opt/usr/dotnet.tizen /usr/share/dotnet.tizen;
chsmack -a _ /usr/share/dotnet.tizen;

mkdir /opt/usr/lib
mv /usr/lib/debug /opt/usr/lib/debug;
chsmack -a _ /opt/usr/lib/debug;

for i in `find /opt/usr/lib/debug/`; do chsmack -a _ $i; done;

ln -s /opt/usr/lib/debug /usr/lib/debug;
chsmack -a _ /usr/lib/debug;

mkdir /opt/usr/src
mv /usr/src/debug /opt/usr/src/debug;
chsmack -a _ /opt/usr/src/debug;

for i in `find /opt/usr/src/debug/`; do chsmack -a _ $i; done;

ln -s /opt/usr/src/debug /usr/src/debug;
chsmack -a _ /usr/src/debug;
