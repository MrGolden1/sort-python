# SORT-PYTHON

Python module bindings for SORT algorithm (Simple, Online, and Realtime Tracking) implemented in C++, with the goal of being fast and easy to use. Tracker is based on this [repo](https://github.com/yasenh/sort-cpp).

# Installation

Before you can install the package, you need to install the following dependencies:

```bash
$ sudo apt install libopencv-dev
$ sudo apt install libeigen3-dev
```
Make sure pip is upgraded to last version:
```bash
$ pip install pip --upgrade
```

Then you can install the package using:

```bash
$ pip install sort-tracker
```

or
    
```bash
$ git clone https://github.com/MrGolden1/sort-python.git
$ cd sort-python
$ pip install .
```

# Usage

    
```python
import sort
# Create a tracker with max_coast_cycles = 5 and min_hits = 3
# Default values are max_coast_cycles = 3 and min_hits = 1
tracker = sort.SORT(5,3)
```

## Methods:

Two main methods are available named `run` and `get_tracks` and you can specify format of input and output as follows:

```python
# format (int):
#    0: [xmin, ymin, w, h]
#    1: [xcenter, ycenter, w, h]
#    2: [xmin, ymin, xmax, ymax]
```

defaulf format is 0.

### `run`

`run` method takes a list of bounding boxes and format, and then performs tracking.

```python
# Input:
#   bounding_boxes: list of bounding boxes [n, 4]
#   format: format of bounding boxes (int)
import numpy as np
bounding_boxes = np.array([[10, 10, 20, 20], [30, 30, 40, 40]])
tracker.run(bounding_boxes, 0)
```

### `get_tracks`

`get_tracks` method returns a list of tracks.

```python
# Input:
#   format: format of bounding boxes (int)
# Output:
#   tracks: list of tracks [n, 5] where n is the number of tracks
#       and 5 is (id, ..., ..., ..., ...) where id is the track id and ... is the bounding box in the specified format
tracks = tracker.get_tracks(0)
```

# Demo

![demo](demo.gif)

`Author: MrGolden1`

`Email: ali.zarrinzadeh@gmail.com`