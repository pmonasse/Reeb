#!/bin/bash
# Arguments: zoom
# -zoom: zoom factor

zoom=$1

$bin/build/reeb -z $zoom input_0.png reeb.png
