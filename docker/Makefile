#
# Parameters
#

# Name of the docker executable
DOCKER = docker

# Docker organization to pull the images from
ORG = qiicr

# Directories
ROOT_DIR = $(shell pwd)/..
BUILD_DIR = build
DOCKER_DIR = $(ROOT_DIR)/docker
TMP = /tmp

# Shell
SHELL = /bin/bash

dicom3tools_version=dicom3tools_1.00.snapshot.20200512090031

#
# Targets
#

image: dcmqi dcmqi.test

# Pull dockcross image
prereq.pull_dockcross:
	$(DOCKER) pull dockcross/manylinux-x64
	$(DOCKER) run --rm dockcross/manylinux-x64 > $(TMP)/dockcross
	chmod u+x $(TMP)/dockcross

# Download and build dicom3tools
prereq.build_dicom3tools: prereq.pull_dockcross
	mkdir -p $(ROOT_DIR)/$(BUILD_DIR)
	cd $(ROOT_DIR)/$(BUILD_DIR) \
	&& [ ! -e $(dicom3tools_version) ] \
	&& wget --no-check-certificate  https://github.com/QIICR/dicom3tools/releases/download/20200512090031/dicom3tools_winexe_1.00.snapshot.20200512090031.zip -O dicom3tools.zip \
	&& tar xjf dicom3tools.zip \
	&& cd $(dicom3tools_version)/ \
	&& $(TMP)/dockcross ./Configure \
	&& $(TMP)/dockcross imake -I./config -DUseQIICRID \
	&& ( $(TMP)/dockcross make -j$(grep -c processor /proc/cpuinfo) World > /dev/null 2>&1 )\
	|| echo "Reusing "

# Download and install "ajv" 
prereq.install_npm_packages: prereq.pull_dockcross
	mkdir -p $(ROOT_DIR)/$(BUILD_DIR)
	# If needed, download node
	cd $(ROOT_DIR)/$(BUILD_DIR) && \
	[ ! -e node-v6.9.5-linux-x64 ] && \
  wget --no-check-certificate https://nodejs.org/dist/v6.9.5/node-v6.9.5-linux-x64.tar.xz && \
	tar xf node-v6.9.5-linux-x64.tar.xz || true
	# Install tools required to run DCMQI "doc" tests
	cd $(ROOT_DIR) && \
	$(TMP)/dockcross bash -c "export PATH=/work/build/node-v6.9.5-linux-x64/bin:$$PATH && sudo npm install ajv-cli@3.3.0 -g"

# Configure, build and package
dcmqi.generate_package: prereq.pull_dockcross
	cd $(ROOT_DIR) && \
	$(TMP)/dockcross cmake -B$(BUILD_DIR) -H. -GNinja -DCMAKE_BUILD_TYPE:STRING=Release -DPYTHON_EXECUTABLE:FILEPATH=/opt/python/cp27-cp27mu/bin/python && \
	$(TMP)/dockcross bash -c "export PATH=/work/build/node-v6.9.5-linux-x64/bin:/work/build/$(dicom3tools_version)/appsrc/dcfile:$$PATH && ninja -C$(BUILD_DIR) -v" && \
	$(TMP)/dockcross ninja -C$(BUILD_DIR)/dcmqi-build package

# Build "dcmqi" docker image
#
# Docker expects package content to be in directory dcmqi-linux. As a
# workaround, the content of the cpack-generated package, stored in
# dcmqi-<version>-linux-<date>-<hash> directory is copied to dcmqi-linux.
#
dcmqi.docker_image:
	$(info $$ROOT_DIR ${ROOT_DIR})
	$(info $$BUILD_DIR ${BUILD_DIR})
	$(eval PACKAGE_FILE_NAME := $(shell ls $(ROOT_DIR)/build/dcmqi-build/_CPack_Packages/linux/TGZ/dcmqi-*-linux*.tar.gz))
	tar -xzvf $(PACKAGE_FILE_NAME) -C $(DOCKER_DIR)/dcmqi
	$(eval PACKAGE_PATH_NAME := $(shell find $(ROOT_DIR)/build/dcmqi-build/_CPack_Packages/linux/TGZ/ -mindepth 1 -maxdepth 1 -type d))
	$(info $$PACKAGE_FILE_NAME ${PACKAGE_FILE_NAME})
	$(info $$PACKAGE_PATH_NAME ${PACKAGE_PATH_NAME})
	$(info $$DOCKER_DIR ${DOCKER_DIR})
	$(shell cp -r $(PACKAGE_PATH_NAME) $(DOCKER_DIR)/dcmqi/dcmqi-linux)
	cd $(DOCKER_DIR) && \
	docker build -t $(ORG)/dcmqi:`git describe --tags --exact-match 2> /dev/null || echo "latest"` \
		--build-arg IMAGE=$(ORG)/dcmqi \
		--build-arg VCS_REF=`git describe --exact-match 2> /dev/null || git rev-parse --short HEAD` \
		--build-arg VCS_URL=`git config --get remote.origin.url` \
		--build-arg BUILD_DATE=`date -u +"%Y-%m-%dT%H:%M:%SZ"` \
	dcmqi

dcmqi: prereq.build_dicom3tools prereq.install_npm_packages dcmqi.generate_package dcmqi.docker_image

dcmqi.test:
	cd $(ROOT_DIR) && \
	$(TMP)/dockcross bash -c "export PATH=/work/build/node-v6.9.5-linux-x64/bin:/work/build/$(dicom3tools_version)/appsrc/dcfile:/opt/python/cp27-cp27mu/bin/:$$PATH && cd /work/build/dcmqi-build && ctest -j4 -D ExperimentalTest -VV --no-compress-output"

dcmqi.clean:
	rm -rf $(ROOT_DIR)/$(BUILD_DIR)
	rm -rf $(DOCKER_DIR)/dcmqi/dcmqi-linux

.PHONY: image dcmqi dcmqi.% prereq.%
