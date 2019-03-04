# Nvenc Session Limit Bump

When using the patch from https://github.com/keylase/nvidia-patch/ on Windows the NvEnc session limit is raised for CUDA encoding sessions, but not for other surface types. 
There is however a work-around. When you first 'enable' the sessions using CUDA surface based encoders, they will remain enabled for other types of surfaces until a reboot.

This program creates a configurable amount of sessions on a Nvidia GPU so the session limit is changed until the next reboot.

## Usage
The default operation is to enable 32 encoding sessions on all Nvidia GPU's. 
Arguments:
```
 -h, --help
      Print help and exit
 -g, --gpu
     The GPU id to bump the limit on (default: all)
 -s, --sessions
     The number of encoding sessions to unlock (default: 32)
```

## Compiling
A Visual Studio 2017 project is included in the code. Windows only. x64 only.

### Dependencies
Nvidia Cuda Toolkit needs to installed: https://developer.nvidia.com/cuda-downloads
Nvidia Video Codec SDK needs to be installed: https://developer.nvidia.com/nvidia-video-codec-sdk#Download

A nice command line parser is used for... parsing the arguments. File is include in the code, original can be found here: https://github.com/vietjtnguyen/argagg

