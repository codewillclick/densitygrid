# densitygrid
Density grid for plotting massive amounts of points, written in C.

### Setup
```
git clone <url>
git submodule update --recurse
```
### Design

Callback functions.
- grid cells
  - alloc (`func_0`)
  - delete (`del_func`)
- data translation
  - `func_value_to_pairs`
  - `func_coord_translation`
  - `func_value_interact`
