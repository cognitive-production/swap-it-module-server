#!/bin/bash

MODEL_XML="Pfdl.Model.xml"
TMP_OUTPUT_DIR="tmp_output"


# Pull latest container
docker pull ghcr.io/opcfoundation/ua-modelcompiler:latest

# Change to script dir
cd "$(dirname "$0")"

# (Re)create tmp output dir
rm -rf "$TMP_OUTPUT_DIR"
mkdir -p "$TMP_OUTPUT_DIR"

# Compile model
docker run -v ${PWD}:/data --rm ghcr.io/opcfoundation/ua-modelcompiler:latest compile -d2 /data/"$MODEL_XML" -cg /data/"$TMP_OUTPUT_DIR"/"$MODEL_XML".csv -o2 /data/"$TMP_OUTPUT_DIR" 

# Move necessary files to script dir
mv -f "$TMP_OUTPUT_DIR"/*.Types.bsd .
mv -f "$TMP_OUTPUT_DIR"/*.NodeIds.csv .
mv -f "$TMP_OUTPUT_DIR"/*.NodeSet2.xml .

# Remove tmp output dir
rm -rf "$TMP_OUTPUT_DIR"