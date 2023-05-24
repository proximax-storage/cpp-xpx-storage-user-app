FROM debian:bullseye AS BUILDER

RUN apt-get update && apt-get install --no-install-recommends -y \
    build-essential \
    cmake \
    pkg-config \
    python3-pip \
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
    libxcb-xkb-dev \
    libfontconfig \
    cereal

RUN pip install conan
RUN pip install aqtinstall

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

# FROM sirius-so-app:1 AS BUILDER

COPY . /app-src
WORKDIR /sirius
RUN cmake -DCMAKE_BUILD_TYPE=Debug -DSIRIUS_DRIVE_MULTI=ON -DQT_DEBUG_FIND_PACKAGE=ON -DCMAKE_FIND_DEBUG_MODE=TRUE /app-src \
   && make -j$(nproc)\
   && make install

RUN /app-src/ci/copyDeps.sh /sirius/UserApp/StorageClientApp /deps


FROM debian:bullseye-slim

WORKDIR /sirius
COPY --from=BUILDER /sirius/UserApp/StorageClientApp /sirius
COPY --from=BUILDER /deps /
COPY --from=BUILDER /opt/qt /opt/qt

RUN apt-get update && apt-get install --no-install-recommends -y \
    xauth \
    '^libxcb.*-dev' \
    libx11-xcb-dev \
    libglu1-mesa-dev \
    libxrender-dev \
    libxi-dev \
    libxkbcommon-dev \
    libxkbcommon-x11-dev \
    libfontconfig

ENV PATH /opt/qt/${QT}/gcc_64/bin:$PATH