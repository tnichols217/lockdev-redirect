#!/bin/bash

set -e

TESTS="tests/rxtx tests/lockdev tests/custom"


# If we run as "root", then we have write access to /var/lock even without
# lockdev-redirect working as expected --> Tests are useless
if [ $(id -u) -eq 0 ]; then
  echo "Don't run tests as root!"
  exit 1
fi

# Test /var/lock write access without LD_PRELOAD set. If this works without
# lockdev-redirect, then we can cancel at this point.
TESTFILE="/var/lock/lockdev-redirect-test$$.lck"
if touch "$TESTFILE"; then
  echo "You are on a distribution which allows users to write to /var/lock!"
  echo "This means that lockdev-redirect is not of much use for you!"
  echo "Tests have been cancelled as all tests would succeed even if something"
  echo "went wrong with building lockdev-redirect."
  rm -f "$TESTFILE"
  exit 1
fi

# Build all tests
for testdir in $TESTS; do make -C "$testdir"; done


# Now export LD_PRELOAD with our library
export LD_PRELOAD=$PWD/lockdev-redirect.so

# Run all tests
for testdir in $TESTS; do make -C "$testdir" test; done


# Some generic success message
echo
echo "All tests ran successfully!"
