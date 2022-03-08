SignalR-Qt
==========

This is a fork from the official version of this project.  That source maybe found at: https://github.com/p3root/signalr-qt

SignalR-Qt is a C++ library used to create client apps to be paired with servers that employ Microsoft's ASP.Net SignalR (https://github.com/SignalR) protocol.  While MS directly offers an official C++ library for this purpose, that does not provide mobile platform compatibility.  This solution, however, works everywhere Qt does, including on iOS and Android!  

The intended consumers of this particular fork (who shall remain anonymous...) already have an integration with this library setup within another project.  That other project contains the "public" headers along with pre-built .dll, .lib, .so, and .a files produced from this source.  

Instructions
============

1. To begin with, clone this repository *recursively* to ensure the submodules are also cloned.

```
git clone --recursive https://github.com/QDroneDev/signalr-qt.git
```

2. To rebuild the **SignalRClient** library, for a given platform, first rebuild its **dependencies**, found in the nested projects.

Open each of the following NESTED projects in Qt Creator.  Configure them to build (i.e. define the .pro.user) as needed per context. 

- `ThirdParty\QtExtJson\QtExtJson.pro`

- `ThirdParty\QHttpServer\src\src.pro`

- `ThirdParty\QtWebSockets\src\websockets\websockets.pro`

3. Build each in DEBUG mode and RELEASE mode, both STATIC and SHARED versions of each.  

> Note: By default, the QMake is setup for shared library builds.  To switch to static, comment out the `CONFIG += sharedlib` line in each .pro, and uncomment the `CONFIG += staticlib` line.

4. After each build, copy the target file produced (e.g. the .dll, .a, etc.) to another directory, and then delete the "shadow build" folder.  On platforms which stamp a version number onto .so files, you'll want to save the symbolic links produced as well.

5. To now build the **SignalRClient** files, open and build: `SignalRLibraries\SignalRClient\SignalRClient.pro`.  For that to succeed, the dependencies need to be copied to their corresponding source directory (i.e. placed adjacent to the .pro files).  You MUST match the DEBUG/RELEASE/SHARED/STATIC detail for each corresponding target build for the SignalRClient!  

6.  Repeat all these steps, with the given tiny tweaks you'll need, oh about 50 billion times if you ever modify this source and have to then update the prebuilts for every context.  Here are the *dimensions*, for which you may need to produce a different version of all these interdependent libraries, in every possible combination:

- PLATFORM (WINDOWS, MAC, LINUX, iOS, ANDROID, CUSTOM (embedded) OSes...) 

- CPU ARCHITECTURE

- DEBUG vs RELEASE

- SHARED vs STATIC

> Note: It seems that some build contexts will allow you to produce pure static builds of your final target executable program, and others need all shared libraries (which must then be distributed along side the main program), and other contexts require some mixed bag of that...  

LICENSE
=======

Copyright (c) 2013-2014, p3root - Patrik Pfaffenbauer (patrik.pfaffenbauer@p3.co.at)
All rights reserved.
 
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

  Redistributions in binary form must reproduce the above copyright notice, this
  list of conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.

  Neither the name of the {organization} nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

