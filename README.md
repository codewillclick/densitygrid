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
