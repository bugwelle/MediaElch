# Spellchecking

We support spellchecking using KDE Sonnet.

Use `-DENABLE_SPELLCHECK=ON` with CMake to enable it.

## Install Sonnet

Install cmake-extra-modules and Sonnet.

Sonnet:

```sh
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_DESIGNERPLUGIN=OFF
```

