# densitygrid
Density grid for plotting massive amounts of points, written in C.

### Setup
Dependencies
- Linux OS or Cygwin
  - may remove Linux-dependent includes in the future

Download...
```
git clone <url>
git submodule update --recursive
```
Build...
```
make
```
Output binaries are compiled under `build/`.

### Examples
Dependencies
- [codewillclick/plotitude](https://github.com/codewillclick/plotitude)
- run from examples directory

### Design

Callback functions.
- grid cells
  - alloc (`func_0`)
  - delete (`del_func`)
- data translation
  - `func_value_to_pairs`
  - `func_coord_translation`
  - `func_value_interact`

# bmplot
This could be its own repo, but it's too closely tied to **densitygrid**.

Simple plotting tool, in the syntactical vein of gnuplot and similar.  Created for easy execution within **plotitude** scripts.

### Commands
- skip
  - `skip <# of lines to skip>`
- pipeline
  - `pipeline <new-value func> <del-value func> <coordinate/value to value pairs func> <translation func> <apply pair-value to grid cell value func>`
- render
  - `render <output filename> <render func> <grid cell value to pixel func>`
- plot
  - `plot <draw frame> <input file> <column indices>`

### Example bmplot syntax

```
skip 1
pipeline n.int del p.int t.xy ii
render r.grad.bmp bmp.black c.hsv.n
plot [0,0,100,80] thung.csv 0,1,2
```
