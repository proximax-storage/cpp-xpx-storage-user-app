FROM ubuntu:20.04
ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC
RUN apt-get update && \
    apt-get -y install python3 python3-simplejson build-essential software-properties-common unzip libssl-dev python3-six ca-certificates gettext-base \
    manpages-dev git lzip libtool-bin python-mako python3-mako python3-pip libgl1-mesa-dev libglu1-mesa-dev software-properties-common
WORKDIR /home/windows
RUN git clone https://github.com/mxe/mxe.git && cd mxe
WORKDIR /home/windows/mxe
RUN chmod a+x ./tools/install-deps && ./tools/install-deps
RUN make gcc MXE_TARGETS='x86_64-w64-mingw32.shared' -j$(nproc)
RUN make openssl MXE_TARGETS='x86_64-w64-mingw32.shared' -j$(nproc)
RUN make boost MXE_TARGETS='x86_64-w64-mingw32.shared' -j$(nproc)
RUN make qt6 MXE_TARGETS='x86_64-w64-mingw32.shared' -j$(nproc)
RUN ln -s /home/windows/mxe/usr/x86_64-w64-mingw32.shared/include/ws2tcpip.h /home/windows/mxe/usr/x86_64-w64-mingw32.shared/include/Ws2tcpip.h
RUN ln -s /home/windows/mxe/usr/x86_64-w64-mingw32.shared/include/winsock.h /home/windows/mxe/usr/x86_64-w64-mingw32.shared/include/Winsock.h
RUN ln -s /home/windows/mxe/usr/x86_64-w64-mingw32.shared/include/winsock2.h /home/windows/mxe/usr/x86_64-w64-mingw32.shared/include/Winsock2.h
RUN ln -s /home/windows/mxe/usr/x86_64-w64-mingw32.shared/include/windows.h /home/windows/mxe/usr/x86_64-w64-mingw32.shared/include/Windows.h
RUN cp -a /home/windows/mxe/usr/x86_64-w64-mingw32.shared/bin/. /home/windows/mxe/usr/x86_64-w64-mingw32.shared/lib
# Copy sources
COPY . /app-src
RUN mkdir _build && cd _build
ENV BUILD_DIR /app-src/_build
ENV PATH="/home/windows/mxe/usr/bin:$PATH"
WORKDIR $BUILD_DIR
RUN x86_64-w64-mingw32.shared-cmake -DSKIP_GRPC=ON -DXPX_STORAGE_SDK_NOT_BUILD_EXAMPLES=ON -DCMAKE_BUILD_TYPE=Release ..
RUN x86_64-w64-mingw32.shared-cmake --build . --target StorageClientApp -j$(nproc)
ENV DESTDIR /storage-tool
WORKDIR $DESTDIR
RUN cp $BUILD_DIR/UserApp/StorageClientApp.exe $DESTDIR
RUN cp $BUILD_DIR/cpp-xpx-chain-sdk/libxpxchaincpp.dll $DESTDIR
RUN cp -r $BUILD_DIR/bin/* $DESTDIR
RUN cp -r /app-src/resources $DESTDIR
RUN cp -r /home/windows/mxe/usr/x86_64-w64-mingw32.shared/qt6/plugins $DESTDIR
RUN cp /home/windows/mxe/usr/x86_64-w64-mingw32.shared/qt6/bin/Qt6Widgets.dll $DESTDIR
RUN cp /home/windows/mxe/usr/x86_64-w64-mingw32.shared/qt6/bin/Qt6Gui.dll $DESTDIR
RUN cp /home/windows/mxe/usr/x86_64-w64-mingw32.shared/qt6/bin/Qt6Core.dll $DESTDIR
RUN find /home/windows/mxe/usr/x86_64-w64-mingw32.shared/lib -name "*.dll" -type f | xargs cp -vt $DESTDIR
RUN zip -r storage-tool.zip /storage-tool