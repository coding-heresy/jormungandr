#!/bin/bash

bazelisk coverage //...

genhtml --branch-coverage --output-directory .coverage_report "$(bazelisk info output_path)/_coverage/_coverage_report.dat"
