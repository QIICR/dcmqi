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

dicom3tools_version=dicom3tools_1.00.snapshot.20160721064406

#
# Targets
#

image: dcmqi dcmqi.test

# Pull dockcross image
prereq.pull_dockcross:
	$(DOCKER) pull dockcross/manylinux-x64
	$(DOCKER) run --rm  dockcross/manylinux-x64 > $(TMP)/dockcross
	chmod u+x $(TMP)/dockcross

# Download and build dicom3tools
prereq.build_dicom3tools: prereq.pull_dockcross
	mkdir -p $(ROOT_DIR)/$(BUILD_DIR)
	cd $(ROOT_DIR)/$(BUILD_DIR) \
	&& [ ! -e $(dicom3tools_version) ] \
	&& wget --no-check-certificate http://slicer.kitware.com/midas3/download/item/270310/dicom3tools_1.00.snapshot.20160721064406.tar.bz2 -O dicom3tools.tar.bz2 \
	&& tar xjf dicom3tools.tar.bz2 \
	&& cd $(dicom3tools_version)/ \
	&& $(TMP)/dockcross ./Configure \
	&& $(TMP)/dockcross imake -I./config -DUseQIICRID \
	&& ( $(TMP)/dockcross make -j$(grep -c processor /proc/cpuinfo) World > /dev/null 2>&1 )\
	|| echo "Reusing "

# Download and install "ajv" and "jsonlint" tools
prereq.install_npm_packages: prereq.pull_dockcross
	mkdir -p $(ROOT_DIR)/$(BUILD_DIR)
	# If needed, download node
	cd $(ROOT_DIR)/$(BUILD_DIR) && \
	[ ! -e node-v6.9.5-linux-x64 ] && \
  wget --no-check-certificate https://nodejs.org/dist/v6.9.5/node-v6.9.5-linux-x64.tar.xz && \
	tar xf node-v6.9.5-linux-x64.tar.xz || true
	# Install tools required to run DCMQI "doc" tests
	cd $(ROOT_DIR) && \
	$(TMP)/dockcross bash -c "export PATH=/work/build/node-v6.9.5-linux-x64/bin:$$PATH && npm install jsonlint -g && npm install ajv-cli -g"

# Configure, build and package
dcmqi.generate_package: prereq.pull_dockcross
	cd $(ROOT_DIR) && \
	$(TMP)/dockcross cmake -B$(BUILD_DIR) -H. -GNinja -DCMAKE_BUILD_TYPE:STRING=Release && \
	$(TMP)/dockcross bash -c "export PATH=/work/build/node-v6.9.5-linux-x64/bin:/work/build/$(dicom3tools_version)/appsrc/dcfile:$$PATH && ninja -C$(BUILD_DIR) -v" && \
	$(TMP)/dockcross ninja -C$(BUILD_DIR)/dcmqi-build package

# Build "dcmqi" docker image
dcmqi.docker_image:
	tar -xzvf $(ROOT_DIR)/$(BUILD_DIR)/dcmqi-build/dcmqi-linux.tar.gz -C $(DOCKER_DIR)/dcmqi
	cd $(DOCKER_DIR) && \
	docker build -t $(ORG)/dcmqi:`git describe --exact-match 2> /dev/null || echo "latest"` \
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
