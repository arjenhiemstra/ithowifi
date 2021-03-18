#!/bin/bash
#
VERSION=$(git describe --dirty --broken --tags --always)

FILE='NRG_itho_wifi/NRG_itho_wifi.ino'
if [[ -f "$FILE" ]]; then
    echo "$FILE exists."
    sed -i "s/\#define FWVERSION.*/\#define FWVERSION \"${VERSION}\"/" ${FILE}

fi