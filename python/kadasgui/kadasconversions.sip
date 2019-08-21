// QMap<int, double> is implemented as a Python dictionary.
%MappedType QMap<int, double>
{
%TypeHeaderCode
#include <qmap.h>
%End

%ConvertFromTypeCode
    // Create the dictionary.
    PyObject *d = PyDict_New();

    if (!d)
        return NULL;

    // Set the dictionary elements.
    QMap<int, double>::const_iterator i = sipCpp->constBegin();

    while (i != sipCpp->constEnd())
    {
        PyObject *kobj = SIPLong_FromLong(i.key());
        PyObject *tobj = PyFloat_FromDouble(i.value());
        if (kobj == NULL || tobj == NULL || PyDict_SetItem(d, kobj, tobj) < 0)
        {
            Py_DECREF(d);
            if (kobj)
            {
                Py_DECREF(kobj);
            }
            if (tobj)
            {
                Py_DECREF(tobj);
            }
            return NULL;
        }

        Py_DECREF(kobj);
        Py_DECREF(tobj);

        ++i;
    }

    return d;
%End

%ConvertToTypeCode
    PyObject *kobj, *tobj;
    SIP_SSIZE_T i = 0;

    // Check the type if that is all that is required.
    if (sipIsErr == NULL)
    {
        if (!PyDict_Check(sipPy))
            return 0;
        return 1;
    }

    QMap<int, double> *qm = new QMap<int, double>;

    while (PyDict_Next(sipPy, &i, &kobj, &tobj))
    {
        int k = SIPLong_AsLong(kobj);
        double t = PyFloat_AsDouble(tobj);

        if (*sipIsErr)
        {
            delete qm;
            return 0;
        }

        qm->insert(k, t);
    }

    *sipCppPtr = qm;

    return sipGetState(sipTransferObj);
%End
};
