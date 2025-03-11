#!/bin/bash
# Arguments: zoom
# -zoom: zoom factor

zoom=$1

$bin/build/reeb -z $zoom input_0.png reeb.png
$bin/bspline "$zoom 0 0; 0 $zoom 0; 0 0 1" input_0.png zoom1.png 1 const 6 1 auto
