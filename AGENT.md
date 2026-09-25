The goal of this project is to create an alternative firmware for a Xiaomi Rice cooker for strict local control.

Ultimately we want an ESPHome firmware. But for investigation, we choose other tools as needed. 

## Provenence
We keep a directory with chronological notes under 'research'.

## Readme file
The README reflects the current state, is an entry point for new users and developers. As such it is updated as the project progresses. 

## ESPHome
Use the projects esphome version instead of the systems that is managed by uv.

Invoke with `esphome`

## Chip tools 
TBD, probably starting with `esptool`.

Command is `esptool`

## Python
When using python scripts, use the projects uv environment instead of the sytem python.

Command `uv`

## TMP
There is a `tmp` folder in the project root that is not commited. 
Using default cache locations on the host for uv and esphome etc is okay, but we try to keep the global state to the project dir otherwise. 
