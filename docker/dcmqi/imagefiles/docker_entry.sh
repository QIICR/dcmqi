#!/bin/bash

case "$1" in
    itkimage2paramap|paramap2itkimage|itkimage2segimage|segimage2itkimage|tid1500reader|tid1500writer)
        $1 "${@:2}"
        ;;
    *)
        echo "ERROR: Unknown command"
        echo ""
        cat >&2 <<ENDHELP
Usage: docker run [-v <HOST_DIR>:<CONTAINER_DIR>] qiicr/dcmqi <command> [args]

Run the given dcmqi *command*.

Available commands are:
  itkimage2paramap
  paramap2itkimage
  itkimage2segimage
  segimage2itkimage
  tid1500reader
  tid1500writer

For *command* help use: docker run qiicr/dcmqi <command> --help
ENDHELP
        exit 1
        ;;
esac

