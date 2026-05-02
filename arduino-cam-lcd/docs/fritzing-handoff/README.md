# Fritzing Handoff Package (Starter)

This folder contains source tables and a checklist to quickly build the final Fritzing breadboard diagram.

Included files:
- [wiring-table.csv](wiring-table.csv)
- [parts-list.csv](parts-list.csv)
- [fritzing-build-checklist.md](fritzing-build-checklist.md)

## Why a starter package instead of a finished .fzz
A fully working `.fzz` usually needs exact Fritzing part IDs and validation in the Fritzing app environment.
This starter package gives everything needed to produce the final student-facing diagram quickly and accurately.

## Pin truth source
Receiver firmware pin map is defined in [arduino-cam-lcd/receiver/receiver.ino](../../receiver/receiver.ino).
Most critical change: display A0/DC is GPIO21.
