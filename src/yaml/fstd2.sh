#!/bin/bash
set -x
export CC=${CC:-gcc}
make && yamllint fstd2.yaml && ./scan2 ./fstd2.output.yaml  <fstd2.yaml | tee fstd2.listing.txt  && yamllint --no-warnings ./fstd2.output.yaml
