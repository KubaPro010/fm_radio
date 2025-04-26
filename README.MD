# fm-radio

fm-radio is a fork of the radio module, however this is made with the focus of FM receiving, for example, here RDS is better as now it decodes:

- PTYN
- AF
- ODA
- TP
- TA
- ECC
- LPS
- ERT
- CT
- ERT
this is just the best RDS demodulator for SDRs, i believe better than RDS Spy, that doesn't do ERT

## How to install?

Well, you have to compile it yourself, first you need to be able to compile [SDR++](https://github.com/AlexandreRouma/SDRPlusPlus) in the first place (tip: you can disable all of the modules in the CMakeLists.txt file to speed it up)

After compiling you'll need to cd into your decoder_modules folder and clone there this repository, so:

```bash
cd decoder_modules
git clone https://github.com/KubaPro010/fm-radio
```

After that you'll need to edit the CMakeLists.txt (again maybe) in the decoder modules group add the line `option(OPT_BUILD_FM_RADIO "FM Radio" ON)` thne click on one of these `OPT_BUILD` things, specifically `OPT_BUILD_RADIO`, now find a 2nd occurence of that and you'll see something like this:

```CMake
if (OPT_BUILD_RADIO)
add_subdirectory("decoder_modules/radio")
endif (OPT_BUILD_RADIO)
```

Under it just paste in this:

```CMake
if (OPT_BUILD_FM_RADIO)
add_subdirectory("decoder_modules/fm_radio")
endif (OPT_BUILD_FM_RADIO)
```

then just do the build instructions of sdrpp and you should have the receiver compiled
