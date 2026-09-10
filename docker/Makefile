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

# dicom3tools is installed from PyPI, which publishes manylinux wheels this image can run.
# Keep this pin in sync with .github/actions/install-dicom3tools/action.yml, which does the
# same for the macOS, Windows and Linux/arm64 jobs.
dicom3tools_version=20260901

# Each dockcross invocation is a separate `docker run`, so only the mounted repository
# survives between targets: the tools have to be installed under the (cached) build
# directory rather than somewhere in the container's own filesystem.
dicom3tools_venv=/work/$(BUILD_DIR)/dicom3tools-venv

# Expands, inside the container, to the directory holding the packaged dicom3tools
# programs -- what find_program(DCIODVFY_EXECUTABLE dciodvfy) needs on PATH. The recipes
# below are expanded by make, then by the host shell, and only then handed to the
# container, so the `$` is escaped to survive the host shell: the substitution has to run
# where the venv exists, which is inside the container.
dicom3tools_bin=\$$($(dicom3tools_venv)/bin/python -c 'import dicom3tools; print(dicom3tools.bin_dir())')

#
# Targets
#

image: dcmqi dcmqi.test

# Pull dockcross image
prereq.pull_dockcross:
	$(DOCKER) pull dockcross/manylinux2014-x64
	$(DOCKER) run --rm dockcross/manylinux2014-x64 > $(TMP)/dockcross
	chmod u+x $(TMP)/dockcross

# Install dicom3tools (provides dciodvfy) from PyPI.
#
# This replaces a download-and-build of the upstream sources that never worked: it fetched
# the *Windows* .zip, unpacked it with `tar xjf`, and cd'd into a directory the flat archive
# does not contain, with the whole && chain swallowed by `|| echo "Reusing "`. The step
# reported success while find_program() found nothing, so the dciodvfy conformance tests
# were silently dropped from this job.
#
# The wheels require Python >= 3.10, so this deliberately does not use the cp38 interpreter
# the rest of this Makefile pins for the build itself.
prereq.install_dicom3tools: prereq.pull_dockcross
	mkdir -p $(ROOT_DIR)/$(BUILD_DIR)
	cd $(ROOT_DIR) && \
	$(TMP)/dockcross bash -c 'set -eu; \
	  PY=$$(ls -1d /opt/python/cp31[0-9]-cp31[0-9]/bin/python 2>/dev/null | sort -V | tail -n 1); \
	  if [ -z "$$PY" ]; then echo "No Python >= 3.10 found in the dockcross image" >&2; exit 1; fi; \
	  "$$PY" -m venv $(dicom3tools_venv); \
	  $(dicom3tools_venv)/bin/python -m pip install --disable-pip-version-check \
	    --only-binary=:all: "dicom3tools==$(dicom3tools_version)"; \
	  BIN=$$($(dicom3tools_venv)/bin/python -c "import dicom3tools; print(dicom3tools.bin_dir())"); \
	  test -x "$$BIN/dciodvfy" || { echo "dciodvfy missing from $$BIN" >&2; exit 1; }; \
	  echo "dciodvfy $(dicom3tools_version) installed at $$BIN/dciodvfy"'

# Download and install "ajv"
prereq.install_npm_packages: prereq.pull_dockcross
	mkdir -p $(ROOT_DIR)/$(BUILD_DIR)
	echo "Root dir: $(ROOT_DIR)"
	echo "Build dir: $(BUILD_DIR)"
	# If needed, download node
	cd $(ROOT_DIR)/$(BUILD_DIR) && \
	[ ! -e node-v12.19.1-linux-x64 ] && \
  wget --no-check-certificate https://nodejs.org/dist/v12.19.1/node-v12.19.1-linux-x64.tar.xz && \
	tar xf node-v12.19.1-linux-x64.tar.xz || true
	# Install tools required to run DCMQI "doc" tests
	cd $(ROOT_DIR) && \
	$(TMP)/dockcross bash -c "export PATH=/work/build/node-v12.19.1-linux-x64/bin:$$PATH && sudo npm install ajv-cli@3.3.0 -g"

# Configure, build and package
dcmqi.generate_package: prereq.pull_dockcross prereq.install_dicom3tools
	cd $(ROOT_DIR) && \
	$(TMP)/dockcross cmake -B$(BUILD_DIR) -H. -GNinja -DCMAKE_BUILD_TYPE:STRING=Release -DPYTHON_EXECUTABLE:FILEPATH=/opt/python/cp38-cp38/bin/python && \
	$(TMP)/dockcross bash -c "export PATH=/work/build/node-v6.9.5-linux-x64/bin:$(dicom3tools_bin):$$PATH && ninja -C$(BUILD_DIR) -v" && \
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

dcmqi.docker_image_test:
	$(info $$DOCKER_DIR ${DOCKER_DIR})
	cd $(DOCKER_DIR) && \
	docker run --rm $(ORG)/dcmqi:`git describe --tags --exact-match 2> /dev/null || echo "latest"` itkimage2paramap --help
	docker run --rm $(ORG)/dcmqi:`git describe --tags --exact-match 2> /dev/null || echo "latest"` paramap2itkimage --help
	docker run --rm $(ORG)/dcmqi:`git describe --tags --exact-match 2> /dev/null || echo "latest"` itkimage2segimage --help
	docker run --rm $(ORG)/dcmqi:`git describe --tags --exact-match 2> /dev/null || echo "latest"` segimage2itkimage --help
	docker run --rm $(ORG)/dcmqi:`git describe --tags --exact-match 2> /dev/null || echo "latest"` tid1500reader --help
	docker run --rm $(ORG)/dcmqi:`git describe --tags --exact-match 2> /dev/null || echo "latest"` tid1500writer --help

dcmqi: prereq.install_dicom3tools prereq.install_npm_packages dcmqi.generate_package dcmqi.docker_image dcmqi.docker_image_test

# Guard against the silent-skip failure mode: when find_program() fails to locate dciodvfy,
# CMake drops the conformance tests without failing anything, so the checks stop running
# unnoticed -- which is exactly what the broken download above did for years. `ctest -N`
# only lists what was registered at configure time, so this needs no PATH of its own.
dcmqi.test: prereq.install_dicom3tools
	cd $(ROOT_DIR) && \
	$(TMP)/dockcross bash -c "cd /work/$(BUILD_DIR)/dcmqi-build && count=\$$(ctest -N -R dciodvfy | sed -n 's/^Total Tests: //p') && echo \"dciodvfy tests registered: \$${count:-0}\" && [ \"\$${count:-0}\" -gt 0 ]"
	cd $(ROOT_DIR) && \
	$(TMP)/dockcross bash -c "export PATH=/work/build/node-v6.9.5-linux-x64/bin:$(dicom3tools_bin):/opt/python/cp38-cp38/bin/:$$PATH && cd /work/build/dcmqi-build && ctest -j4 -D ExperimentalTest -VV --no-compress-output"

dcmqi.clean:
	rm -rf $(ROOT_DIR)/$(BUILD_DIR)
	rm -rf $(DOCKER_DIR)/dcmqi/dcmqi-linux

.PHONY: image dcmqi dcmqi.% prereq.%
