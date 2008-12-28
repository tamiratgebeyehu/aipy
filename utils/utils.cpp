/*
 * Some additional utility functions for AIPY, written in C++.  These are
 * mostly for speed-critical functions.  Right now, the only thing in here
 * is a function called add2array which behaves how you'd expect the following
 * to work: a[ind] += data.  (Note that this *doesn't* do what you expect
 * under numpy.
 *
 * Author: Aaron Parsons
 * Date: 11/20/07
 * Revisions:
 *      12/05/07 arp    Extended functionality in C++ to all array types.
 */

#include <Python.h>
#include "numpy/arrayobject.h"

#define QUOTE(s) # s

#define PNT1(a,i) (a->data + i*a->strides[0])
#define PNT2(a,i,j) (a->data+i*a->strides[0]+j*a->strides[1])
#define IND1(a,i,type) *((type *)PNT1(a,i))
#define IND2(a,i,j,type) *((type *)PNT2(a,i,j))

#define TYPE(a) a->descr->type_num
#define CHK_ARRAY_TYPE(a,type) \
    if (TYPE(a) != type) { \
        PyErr_Format(PyExc_ValueError, "type(%s) != %s", \
        QUOTE(a), QUOTE(type)); \
        return NULL; }

#define DIM(a,i) a->dimensions[i]
#define CHK_ARRAY_DIM(a,i,d) \
    if (DIM(a,i) != d) { \
        PyErr_Format(PyExc_ValueError, "dim(%s) != %s", \
        QUOTE(a), QUOTE(d)); \
        return NULL; }

#define RANK(a) a->nd
#define CHK_ARRAY_RANK(a,r) \
    if (RANK(a) != r) { \
        PyErr_Format(PyExc_ValueError, "rank(%s) != %s", \
        QUOTE(a), QUOTE(r)); \
        return NULL; }

#define ADD(ptr0,ptr1,type) \
    *(type *)ptr0 += *(type *)ptr1;

#define CADD(ptr0,ptr1,type) \
    *(type *)ptr0 += *(type *)ptr1; \
    *(type *)(ptr0 + sizeof(type)) += *(type *)(ptr1 + sizeof(type));

// A template for implementing addition loops for different data types
template<typename T, NPY_TYPES NT> struct AddStuff {
    // Adds data to a at indices specified in ind.  Assumes arrays are safe.
    static void addloop(PyArrayObject *a, PyArrayObject *ind, 
            PyArrayObject *data) {
        char *index = NULL;
        for (int i=0; i < DIM(ind,0); i++) {
            index = a->data;
            for (int j=0; j < RANK(a); j++) {
                index += IND2(ind,i,j,long) * a->strides[j];
            }
            ADD(index,PNT1(data,i),T);
        }
    }
    // CAdds data to a at indices specified in ind.  Assumes arrays are safe.
    static void caddloop(PyArrayObject *a, PyArrayObject *ind, 
            PyArrayObject *data) {
        char *index = NULL;
        for (int i=0; i < DIM(ind,0); i++) {
            index = a->data;
            for (int j=0; j < RANK(a); j++) {
                index += IND2(ind,i,j,long) * a->strides[j];
            }
            CADD(index,PNT1(data,i),T);
        }
    }
};

// Adds data to a at indicies specified in ind.  Checks safety of arrays input.
PyObject *add2array(PyObject *self, PyObject *args) {
    PyArrayObject *a, *ind, *data;
    // Parse arguments and perform sanity check
    if (!PyArg_ParseTuple(args, "OOO", &a, &ind, &data)) return NULL;
    CHK_ARRAY_RANK(ind, 2);
    CHK_ARRAY_RANK(data, 1);
    CHK_ARRAY_DIM(ind, 0, DIM(data,0));
    CHK_ARRAY_DIM(ind, 1, RANK(a));
    CHK_ARRAY_TYPE(ind, NPY_LONG);
    if (TYPE(a) != TYPE(data)) {
        printf("%d %d\n", TYPE(a), TYPE(data));
        PyErr_Format(PyExc_ValueError, "type(%s) != type(%s)",
        QUOTE(a), QUOTE(data));
        return NULL;
    }
    // Use template to implement data loops for all data types
    if (TYPE(a) == NPY_BOOL) {
        AddStuff<bool,NPY_BOOL>::addloop(a,ind,data);
    } else if (TYPE(a) == NPY_BYTE) {
        AddStuff<char,NPY_BYTE>::addloop(a,ind,data);
    } else if (TYPE(a) == NPY_UBYTE) {
        AddStuff<unsigned char,NPY_BYTE>::addloop(a,ind,data);
    } else if (TYPE(a) == NPY_SHORT) {
        AddStuff<short,NPY_SHORT>::addloop(a,ind,data);
    } else if (TYPE(a) == NPY_USHORT) {
        AddStuff<unsigned short,NPY_SHORT>::addloop(a,ind,data);
    } else if (TYPE(a) == NPY_INT) {
        AddStuff<int,NPY_INT>::addloop(a,ind,data);
    } else if (TYPE(a) == NPY_UINT) {
        AddStuff<unsigned int,NPY_UINT>::addloop(a,ind,data);
    } else if (TYPE(a) == NPY_LONG) {
        AddStuff<long,NPY_LONG>::addloop(a,ind,data);
    } else if (TYPE(a) == NPY_ULONG) {
        AddStuff<unsigned long,NPY_ULONG>::addloop(a,ind,data);
    } else if (TYPE(a) == NPY_LONGLONG) {
        AddStuff<long long,NPY_LONGLONG>::addloop(a,ind,data);
    } else if (TYPE(a) == NPY_ULONGLONG) {
        AddStuff<unsigned long long,NPY_ULONGLONG>::addloop(a,ind,data);
    } else if (TYPE(a) == NPY_FLOAT) {
        AddStuff<float,NPY_FLOAT>::addloop(a,ind,data);
    } else if (TYPE(a) == NPY_DOUBLE) {
        AddStuff<double,NPY_DOUBLE>::addloop(a,ind,data);
    } else if (TYPE(a) == NPY_LONGDOUBLE) {
        AddStuff<long double,NPY_LONGDOUBLE>::addloop(a,ind,data);
    } else if (TYPE(a) == NPY_CFLOAT) {
        AddStuff<float,NPY_FLOAT>::caddloop(a,ind,data);
    } else if (TYPE(a) == NPY_CDOUBLE) {
        AddStuff<double,NPY_DOUBLE>::caddloop(a,ind,data);
    } else if (TYPE(a) == NPY_CLONGDOUBLE) {
        AddStuff<long double,NPY_CLONGDOUBLE>::caddloop(a,ind,data);
    } else {
        PyErr_Format(PyExc_ValueError, "Unsupported data type.");
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

// Wrap function into module
static PyMethodDef UtilsMethods[] = {
    {"add2array", (PyCFunction)add2array, METH_VARARGS,
        "Add 'data' to 'a' at the indices specified in 'ind'.  'data' must be 1 dimensional, 'ind' must have 1st axis same as 'data' and 2nd axis equal to number of dimensions in 'a'.  Data types of 'a' and 'data' must match."},
    {NULL, NULL}
};

PyMODINIT_FUNC initutils(void) {
    (void) Py_InitModule("utils", UtilsMethods);
    import_array();
};
