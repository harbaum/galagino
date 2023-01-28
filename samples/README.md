# Samples

Some acade machines contained special hardware to generate certains
sounds. Emulating this hardware is rather tricky and the flash
memory size of the ESP32 allows to store some of these as
digital sound samples.

This directory contains one sample needed for the explosion sound
in Galaga and a few more samples for Donkey Kong.

These have to be converted into C source code using the
(romconv.py)[../romconv/romconv.py] tool.

```
../romconv/romconv.py galaga_sample_boom ./galaga_boom.s8 ../galagino/galaga_sample_boom.h
```

The Donkey Kong sounds are not complete, yet. 
```
../romconv/romconv.py dkong_sample_intro ./dkong_intro.s8 ../galagino/dkong_sample_intro.h
../romconv/romconv.py dkong_sample_stomp ./dkong_stomp.s8 ../galagino/dkong_sample_stomp.h
../romconv/romconv.py dkong_sample_roar  ./dkong_roar.s8 ../galagino/dkong_sample_roar.h
../romconv/romconv.py dkong_sample_howhigh ./dkong_howhigh.s8 ../galagino/dkong_sample_howhigh.h
../romconv/romconv.py dkong_sample_bgmus  ./dkong_bgmus.s8 ../galagino/dkong_sample_bgmus.h
../romconv/romconv.py dkong_sample_spring ./dkong_spring.s8 ../galagino/dkong_sample_spring.h
```
