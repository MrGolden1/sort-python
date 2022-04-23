#include <Python.h>
#include "tracker.h"
// include numpy object array api
#include <numpy/arrayobject.h>
#include <map>
#include <opencv2/opencv.hpp>
#include <vector>

typedef struct
{
    PyObject_HEAD
        Tracker *tracker;
    int kMinHits;
} Py_SORT;

static PyObject *Py_SORT_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int Py_SORT_init(Py_SORT *self, PyObject *args, PyObject *kwds);
static void Py_SORT_dealloc(Py_SORT *self);
static PyObject *Py_SORT_run(Py_SORT *self, PyObject *args);
static PyObject *Py_SORT_get_tracks(Py_SORT *self, PyObject *args);
static PyObject *Py_SORT_reset_id(Py_SORT *self);

/*
 * Module specification
 */
static PyMethodDef Py_SORT_methods[] = {
    {"run", (PyCFunction)Py_SORT_run, METH_VARARGS, "Run tracker"},
    {"get_tracks", (PyCFunction)Py_SORT_get_tracks, METH_VARARGS, "Get tracks"},
    {"reset_id", (PyCFunction)Py_SORT_reset_id, METH_NOARGS, "Reset ID"},
    {NULL} /* Sentinel */
};

/*
 * Module Docstring
 */
static char Py_SORT_doc[] = "Python wrapper for SORT";

/*
 * Module Definition
 */
static PyTypeObject Py_SORT_Type = {
    PyVarObject_HEAD_INIT(NULL, 0) "SORT", /* tp_name */
    sizeof(Py_SORT),                          /* tp_basicsize */
    0,                                        /* tp_itemsize */
    (destructor)Py_SORT_dealloc,              /* tp_dealloc */
    0,                                        /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_reserved */
    0,                                        /* tp_repr */
    0,                                        /* tp_as_number */
    0,                                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    0,                                        /* tp_hash */
    0,                                        /* tp_call */
    0,                                        /* tp_str */
    0,                                        /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                       /* tp_flags */
    Py_SORT_doc,                              /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    Py_SORT_methods,                          /* tp_methods */
    0,                                        /* tp_members */
    0,                                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    (initproc)Py_SORT_init,                   /* tp_init */
    0,                                        /* tp_alloc */
    Py_SORT_new                               /* tp_new */
};

static PyObject *Py_SORT_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Py_SORT *self;
    self = (Py_SORT *)type->tp_alloc(type, 0);
    return (PyObject *)self;
}

static int Py_SORT_init(Py_SORT *self, PyObject *args, PyObject *kwds)
{
    int max_age = 3;
    int min_hits = 1;
    float iou_threshold = 0.3;

    // parameters are optional
    // get from kwds
    if (kwds)
    {
        PyObject *max_age_obj = PyDict_GetItemString(kwds, "max_age");
        if (max_age_obj)
        {
            max_age = PyLong_AsLong(max_age_obj);
        }
        PyObject *min_hits_obj = PyDict_GetItemString(kwds, "min_hits");
        if (min_hits_obj)
        {
            min_hits = PyLong_AsLong(min_hits_obj);
        }
        PyObject *iou_threshold_obj = PyDict_GetItemString(kwds, "iou_threshold");
        if (iou_threshold_obj)
        {
            iou_threshold = (float)PyFloat_AsDouble(iou_threshold_obj);
        }
    }

    self->tracker = new Tracker(max_age, iou_threshold);
    self->kMinHits = min_hits;
    return 0;
}

static void Py_SORT_dealloc(Py_SORT *self)
{
    delete self->tracker;
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *Py_SORT_run(Py_SORT *self, PyObject *args)
{
    // parameters
    // 1. numpy array of any dtype(numpy.ndarray)
    // 2. format (int)
    //    0: [xmin, ymin, w, h]
    //    1: [xcenter, ycenter, w, h]
    //    2: [xmin, ymin, xmax, ymax]

    PyObject *py_array;
    int format = 0; // default format
    if (!PyArg_ParseTuple(args, "O|i", &py_array, &format))
        return NULL;

    // check if array is numpy array
    if (!PyArray_Check(py_array))
    {
        PyErr_SetString(PyExc_TypeError, "First argument must be numpy array");
        return NULL;
    }

    // check if format is valid
    if (format < 0 || format > 2)
    {
        PyErr_SetString(PyExc_ValueError, "Format must be 0, 1 or 2");
        return NULL;
    }

    // get dtype
    PyArrayObject *array = (PyArrayObject *)py_array;
    int dtype = array->descr->type_num;

    // cast any dtype to int32
    if (dtype != NPY_INT32)
    {
        PyArrayObject *array_int32 = (PyArrayObject *)PyArray_Cast(array, NPY_INT32);
        if (array_int32 == NULL)
        {
            PyErr_SetString(PyExc_TypeError, "Cannot cast numpy array to int32");
            return NULL;
        }
        py_array = (PyObject *)array_int32;
    }

    // release GIL
    Py_BEGIN_ALLOW_THREADS

    // get data
    npy_intp *shape = PyArray_DIMS(array);
    npy_intp n = shape[0];
    npy_intp m = shape[1];

    if (m < 4)
    {
        PyErr_SetString(PyExc_TypeError, "Array must have at least 4 columns");
        return NULL;
    }

    std::vector <cv::Rect> rects;
    rects.reserve(n);

    int xmin, ymin, width, height;
    switch (format)
    {
    case 0:
        for (int i = 0; i < n; i++)
        {
            xmin = *(int *)PyArray_GETPTR2(py_array, i, 0);
            ymin = *(int *)PyArray_GETPTR2(py_array, i, 1);
            width = *(int *)PyArray_GETPTR2(py_array, i, 2);
            height = *(int *)PyArray_GETPTR2(py_array, i, 3);
            rects.push_back(cv::Rect(xmin, ymin, width, height));
        }
        break;
    case 1:
        for (int i = 0; i < n; i++)
        {
            width = *(int *)PyArray_GETPTR2(py_array, i, 2);
            height = *(int *)PyArray_GETPTR2(py_array, i, 3);
            xmin = *(int *)PyArray_GETPTR2(py_array, i, 0) - width / 2;
            ymin = *(int *)PyArray_GETPTR2(py_array, i, 1) - height / 2;
            rects.push_back(cv::Rect(xmin, ymin, width, height));
        }
        break;
    case 2:
        for (int i = 0; i < n; i++)
        {
            xmin = *(int *)PyArray_GETPTR2(py_array, i, 0);
            ymin = *(int *)PyArray_GETPTR2(py_array, i, 1);
            width = *(int *)PyArray_GETPTR2(py_array, i, 2) - xmin;
            height = *(int *)PyArray_GETPTR2(py_array, i, 3) - ymin;
            rects.push_back(cv::Rect(xmin, ymin, width, height));
        }
        break;
    default:
        PyErr_SetString(PyExc_TypeError, "Format must be 0, 1 or 2");
        return NULL;
    }

    // for (int i = 0; i < n; i++)
    // {
    //     printf("%d: [%d, %d, %d, %d]\n", i, rects[i].x, rects[i].y, rects[i].width, rects[i].height);
    // }
    // run tracker
    self->tracker->Run(rects);

    // acquire GIL
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}

static PyObject *Py_SORT_get_tracks(Py_SORT *self, PyObject *args)
{
    std::map<int, Track> tracks = self->tracker->GetTracks();

    // convert tracks to numpy array [n, 5]
    // [id, x, y, w, h]

    // parameters
    // 1. format (int)
    //    0: [xmin, ymin, w, h]
    //    1: [xcenter, ycenter, w, h]
    //    2: [xmin, ymin, xmax, ymax]

    int format = 0; // default format
    if (!PyArg_ParseTuple(args, "|i", &format))
        return NULL;
    // check if format is valid
    if (format < 0 || format > 2)
    {
        PyErr_SetString(PyExc_ValueError, "Format must be 0, 1 or 2");
        return NULL;
    }
    
    // convert tracks to numpy array
    int hited = 0;
    if (self->kMinHits > 1){
        for (auto &trk : tracks)
            if (trk.second.hit_streak_ >= self->kMinHits)
                hited++;
    }
    else
        hited = tracks.size();

    int n = hited;
    int m = 5;
    npy_intp dims[2] = {n, m};
    PyArrayObject *array = (PyArrayObject *)PyArray_SimpleNew(2, dims, NPY_INT32);
    if (array == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "Cannot create numpy array");
        return NULL;
    }

    int *data = (int *)PyArray_DATA(array);
    int i = 0;
    for (auto &trk : tracks)
    {
        int id = trk.first;
        const auto &bbox = trk.second.GetStateAsBbox();
        if (trk.second.hit_streak_ < self->kMinHits)
            continue;

        switch (format)
        {
        case 0: // [xmin, ymin, w, h]
            *data++ = id;
            *data++ = bbox.x;
            *data++ = bbox.y;
            *data++ = bbox.width;
            *data++ = bbox.height;
            break;
        case 1: // [xcenter, ycenter, w, h]
            *data++ = id;
            *data++ = bbox.x + bbox.width / 2;
            *data++ = bbox.y + bbox.height / 2;
            *data++ = bbox.width;
            *data++ = bbox.height;
            break;
        case 2: // [xmin, ymin, xmax, ymax]
            *data++ = id;
            *data++ = bbox.x;
            *data++ = bbox.y;
            *data++ = bbox.x + bbox.width;
            *data++ = bbox.y + bbox.height;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "Invalid format");
            return NULL;
        }
        i++;
    }

    return (PyObject *)array;
}

static PyObject *Py_SORT_reset_id(Py_SORT *self)
{
    self->tracker->ResetID();
    Py_RETURN_NONE;
}

/*
* Module Definition
*/
static struct PyModuleDef module_def = {
    PyModuleDef_HEAD_INIT,
    "sort",
    "SORT module",
    -1,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC PyInit__sort(void)
{
    PyObject *m = PyModule_Create(&module_def);
    if (m == NULL)
        return NULL;

    // import numpy
    import_array();
    
    // SORT class
    Py_SORT_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&Py_SORT_Type) < 0)
        return NULL;
    Py_INCREF(&Py_SORT_Type);
    PyModule_AddObject(m, "SORT", (PyObject *)&Py_SORT_Type);

    // constants
    PyModule_AddIntConstant(m, "FORMAT_0", 0);
    PyModule_AddIntConstant(m, "FORMAT_1", 1);
    PyModule_AddIntConstant(m, "FORMAT_2", 2);

    return m;
}
