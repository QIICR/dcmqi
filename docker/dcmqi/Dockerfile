FROM centos:5
MAINTAINER http://github.com/QIICR

COPY imagefiles/docker_entry.sh /dcmqi/
COPY dcmqi-linux/bin/* /usr/bin/

WORKDIR /work
ENTRYPOINT ["/bin/bash","/dcmqi/docker_entry.sh"]

# Build-time metadata as defined at http://label-schema.org
ARG BUILD_DATE
ARG IMAGE
ARG VCS_REF
ARG VCS_URL
LABEL org.label-schema.build-date=$BUILD_DATE \
      org.label-schema.name=$IMAGE \
      org.label-schema.vcs-ref=$VCS_REF \
      org.label-schema.vcs-url=$VCS_URL \
      org.label-schema.schema-version="1.0"
