# Image loading test

```console
$ cmake -B ./build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
$ cmake --build ./build
$ timeout 5m python utils/run.py build/load build/device/image
```
