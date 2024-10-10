TARGETS=build/examples/usb_microphone/Audio.uf2
all: $(TARGETS)

CURRENT_DIR := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
PICO_SDK_PATH := $(CURRENT_DIR)pico-sdk

$(TARGETS): pico-sdk
	mkdir -p build
	cd build && \
            cmake -DPICO_SDK_PATH=$(PICO_SDK_PATH) .. && \
            make -j


pico-sdk: pico-sdk/README.md

pico-sdk/README.md:
	rm -rf pico-sdk
	git clone https://github.com/raspberrypi/pico-sdk.git
	cd pico-sdk/lib/tinyusb && git submodule update --init

clean:
	rm -rf build pico-sdk

#stages:
#  - build
#
#variables:
#      GIT_SUBMODULE_STRATEGY: normal
#
#build:
#    image: ubuntu:22.04
#    stage: build
#    variables:
#       BUILD_DIR: ${CI_PROJECT_DIR}/build
#       RELEASE_SLUG: ${CI_COMMIT_TAG}_${CI_COMMIT_BRANCH}_${CI_COMMIT_SHORT_SHA}
#       RELEASE_DIR: ${CI_PROJECT_DIR}/Release_${RELEASE_SLUG}
#       RELEASE_FILES: ${BUILD_DIR}/examples/usb_microphone/*.elf ${BUILD_DIR}/examples/usb_microphone/*.bin ${BUILD_DIR}/examples/usb_microphone/*.hex ${BUILD_DIR}/examples/usb_microphone/*.map ${BUILD_DIR}/examples/usb_microphone/*.uf2
#    script:
#      - echo "Building PDM microphone in ${BUILD_DIR}"
#      - git submodule update --init
#      - pushd pico-sdk/lib/tinyusb && git submodule update --init && popd
#      - rm -rf ${BUILD_DIR}
#      - mkdir -p ${BUILD_DIR}
#      - pushd ${BUILD_DIR}
#      - export PICO_SDK_PATH=${CI_PROJECT_DIR}/pico-sdk
#      - 
#      - make -j
#      - popd
#      - mkdir -p ${RELEASE_DIR}
#      - ls -l build
#      - cp ${RELEASE_FILES} ${RELEASE_DIR}
#    artifacts:
#      name: pdm_mic-${CI_COMMIT_TAG}_${CI_COMMIT_BRANCH}_${CI_COMMIT_SHORT_SHA}_RELEASE
#      paths:
#        - ${RELEASE_DIR}
#
