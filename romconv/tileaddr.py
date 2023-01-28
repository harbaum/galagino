#!/usr/bin/env python3
import sys

if len(sys.argv) != 2:
    print("Usage:",sys.argv[0], "<outfile>")
    print("  ", sys.argv[0], "../galagino/tileaddr.h")
    sys.exit(-1)

with open(sys.argv[1], "w") as f:
  map = [[None for x in range(28)] for y in range(36)]
  
  # create mapping table from screen coordinate to memory
  for offs in range(1024):
    mx = offs % 32;
    my = offs // 32;
    
    if my <= 1:
      sx = my + 34;
      sy = mx - 2;
    elif my >= 30:
      sx = my - 30;
      sy = mx - 2;
    else:
      sx = mx + 2;
      sy = my - 2;

    if sx>= 0 and sx < 36 and sy >=0 and sy<28:
      map[sx][27-sy] = offs

  print("const unsigned short tileaddr[][28] = {", file=f)    
  rows = []
  for row in map:
    vals = []
    for val in row:
      vals.append(str(val))    
    rows.append(" {" + ", ".join(vals) + "}")

  print(",\n".join(rows), file=f)
  print("};", file=f)

