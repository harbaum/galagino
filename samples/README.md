# Samples

Some acade machines contained special hardware to generate certain
sounds. Emulating this hardware may be tricky and the flash memory
size of the ESP32 allows to store some of these as digital sound
samples.

This directory contains one sample needed for the explosion sound
in Galaga and three more samples for Donkey Kong.

These have to be converted into C source code using the
(romconv.py)[../romconv/romconv.py] tool.

```
../romconv/romconv.py galaga_sample_boom ./galaga_boom.s8 ../galagino/galaga_sample_boom.h
```

Donkey Kong does most audio via its MB8884/I8048 audio CPU. But three sounds
are generated via discrete logic. These are still regenerated using samples.


```
../romconv/romconv.py dkong_sample_walk ./dkong_walk.s8 ../galagino/dkong_sample_walk.h
../romconv/romconv.py dkong_sample_jump ./dkong_jump.s8 ../galagino/dkong_sample_jump.h
../romconv/romconv.py dkong_sample_stomp ./dkong_stomp.s8 ../galagino/dkong_sample_stomp.h
```
