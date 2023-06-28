FROM debian:bullseye AS BUILDER

RUN apt-get update && apt-get install --no-install-recommends -y \
    libfuse2 \
    curl \
    build-essential \
    cmake \
    pkg-config \
    python3-pip \
    python3-dev \
    gnutls-bin \
    libgtest-dev \
    libgcrypt20-dev \
    libssl-dev \
    libboost-dev \
    libboost-atomic-dev \
    libboost-date-time-dev \
    libboost-regex-dev \
    libboost-system-dev \
    libboost-timer-dev \
    libboost-chrono-dev \
    libboost-log-dev \
    libboost-thread-dev \
    libboost-filesystem-dev \
    libboost-program-options-dev \
    libboost-stacktrace-dev \
    libboost-random-dev \
    libgl1-mesa-dev \
    libpulse-dev \
    libxcb-glx0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-render0 \
    libxcb-shape0 \
    libxcb-shm0 \
    libxcb-sync1 \
    libxcb-util1 \
    libxcb-xfixes0 \
    libxcb-xinerama0 \
    libxcb1 \
    libxkbcommon-dev \
    libxkbcommon-x11-0 \
    libxcb-cursor-dev \
    libxcb-xkb-dev \
    libfontconfig \
    cereal

RUN pip install conan
RUN pip install aqtinstall

WORKDIR /linuxdeployqt
RUN curl -fsSL https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage -o linuxdeployqt-continuous-x86_64.AppImage
RUN chmod a+x ./linuxdeployqt-continuous-x86_64.AppImage

ARG QT=6.5.0
ARG QT_MODULES="qtscxml "
ARG QT_ARCHIVES="qttools qtbase icu"
ARG QT_HOST=linux
ARG QT_TARGET=desktop
ARG QT_ARCH=gcc_64
RUN aqt install-qt -O /opt/qt ${QT_HOST} ${QT_TARGET} ${QT} ${QT_ARCH} -m ${QT_MODULES} --archives ${QT_ARCHIVES} 

ENV PATH /opt/qt/${QT}/gcc_64/bin:$PATH
ENV QT_PLUGIN_PATH /opt/qt/${QT}/gcc_64/plugins/
ENV QML_IMPORT_PATH /opt/qt/${QT}/gcc_64/qml/
ENV QML2_IMPORT_PATH /opt/qt/${QT}/gcc_64/qml/

ENV BUILD_DIR /usr/sirius

# FROM sirius-so-app:1 AS BUILDER

WORKDIR $BUILD_DIR
COPY . /app-src
RUN cmake -DCMAKE_BUILD_TYPE=Debug -DSIRIUS_DRIVE_MULTI=ON /app-src \
   && cmake --build $BUILD_DIR --target StorageClientApp -j$(nproc)\
   && make install

ENV LD_LIBRARY_PATH=/opt/qt/${QT}/gcc_64/lib:$BUILD_DIR/bin:$BUILD_DIR/cpp-xpx-chain-sdk:"$LD_LIBRARY_PATH"

RUN echo -e "[Desktop Entry]\n\
Type=Application\n\
Name=Storaget Tool - ProximaX\n\
Icon=Storage\n\
Comment=storage\n\
Exec=StorageClientApp\n\
Categories=Network;\n\
X-AppImage-Version=" > $BUILD_DIR/UserApp/tool.desktop
RUN /linuxdeployqt/linuxdeployqt-continuous-x86_64.AppImage --appimage-extract-and-run $BUILD_DIR/UserApp/tool.desktop -verbose=1 -bundle-non-qt-libs

ENV DESTDIR /storage-tool
WORKDIR $DESTDIR
RUN cp $BUILD_DIR/UserApp/StorageClientApp $DESTDIR
RUN cp -r $BUILD_DIR/UserApp/resources $DESTDIR
RUN cp -r $BUILD_DIR/translations $DESTDIR
RUN cp -r $BUILD_DIR/lib $DESTDIR
RUN cp -r $BUILD_DIR/plugins $DESTDIR

RUN cp $BUILD_DIR/bin/*.so $DESTDIR/lib/
RUN cp $BUILD_DIR/cpp-xpx-chain-sdk/*.so $DESTDIR/lib
RUN chmod a+x $DESTDIR/StorageClientApp


# FROM debian:bullseye-slim

# WORKDIR /sirius
# COPY --from=BUILDER /sirius/UserApp/StorageClientApp /sirius
# COPY --from=BUILDER /deps /
# COPY --from=BUILDER /opt/qt /opt/qt

# RUN apt-get update && apt-get install --no-install-recommends -y \
#     xauth \
#     '^libxcb.*-dev' \
#     libx11-xcb-dev \
#     libglu1-mesa-dev \
#     libxrender-dev \
#     libxi-dev \
#     libxkbcommon-dev \
#     libxkbcommon-x11-dev \
#     libfontconfig

# ENV PATH /opt/qt/${QT}/gcc_64/bin:$PATH