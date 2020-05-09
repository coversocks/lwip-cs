lwIP for coversocks
===

### Compile
* get the lwip source code by `git clone https://git.savannah.nongnu.org/git/lwip.git`
* get the lwip-cs source code to the same folder by `git clone https://github.com/coversocks/lwip-cs.git`
* go to lwip folder and apply patch by `git am xxx/lwip-cs/0001-change-for-coversocks.patch`
* compile lwipcore

```.sh
cd xx/lwip/
mkdir build
cmake ..
make -j5 lwipcore
```

* compile lwipcs

```.sh
cd xx/lwip-cs/
mkdir build
cmake ..
make -j5 lwipcs
```
* the liblwipcs.a will be in src
