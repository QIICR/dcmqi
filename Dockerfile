FROM fedorov/docker-centos-build
MAINTAINER http://github.com/QIICR

# Build and install dcmqi
WORKDIR /usr/src
RUN git clone https://github.com/QIICR/dcmqi.git && \
  mkdir dcmqi-superbuild && \
  cd dcmqi-superbuild && \
  cmake -DCMAKE_INSTALL_PREFIX=/usr ../dcmqi && \
  make -j$(grep -c processor /proc/cpuinfo)

WORKDIR /usr/src
ENTRYPOINT ["/bin/bash","/usr/src/dcmqi/docker_entry.sh"]
