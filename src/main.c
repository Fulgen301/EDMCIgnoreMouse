#include <Windows.h>
#include <commctrl.h>

#include <Python.h>

#define WM_FOUND_HANDLE (WM_USER + 2)

static BOOL SetWindowTransparent(HWND hWnd, BOOL transparent)
{
	LONG exstyle = GetWindowLong(hWnd, GWL_EXSTYLE);

	if (exstyle & WS_EX_LAYERED)
	{
		if (transparent)
		{
			SetWindowLong(hWnd, GWL_EXSTYLE, exstyle & ~WS_EX_TRANSPARENT);
		}
		else
		{
			SetWindowLong(hWnd, GWL_EXSTYLE, exstyle | WS_EX_TRANSPARENT);
		}

		return TRUE;
	}

	return FALSE;
}

static LRESULT WINAPI MainSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	if (uMsg == WM_ACTIVATEAPP)
	{
		if (SetWindowTransparent(hWnd, wParam))
		{
			return 0;
		}
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

static LRESULT WINAPI ThreadResultSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	if (uMsg == WM_FOUND_HANDLE)
	{
		HWND foundWindow = (HWND) lParam;

		if (GetForegroundWindow() == foundWindow)
		{
			SetWindowTransparent(foundWindow, TRUE);
		}

		SetWindowSubclass((HWND) lParam, &MainSubclassProc, 1, 0);
		RemoveWindowSubclass(hWnd, &ThreadResultSubclassProc, 1);
		return 0;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

static DWORD WINAPI ThreadProc(LPVOID parameter)
{
	HWND targetWindow = ((void **) parameter)[0];
	wchar_t *windowName = ((void **) parameter)[1];

	for (;;)
	{
		HWND window = FindWindowW(NULL, windowName);
		if (window)
		{
			PostMessage(targetWindow, WM_FOUND_HANDLE, 0, (LONG_PTR) window);
			PyMem_FREE(windowName);
			free(parameter);
			ExitThread(0);
		}

		Sleep(100);
	}
}

static PyObject *ImportAppLongName()
{
	PyObject *importMembers = Py_BuildValue("[s]", "applongname");
	if (!importMembers)
	{
		PyErr_SetString(PyExc_RuntimeError, "Failed to create import members list");
		return NULL;
	}

	PyObject *constants = PyImport_ImportModuleEx("constants", NULL, NULL, importMembers);
	Py_DECREF(importMembers);

	if (!constants)
	{
		return NULL;
	}

	PyObject *applongname = PyObject_GetAttrString(constants, "applongname");
	if (!applongname)
	{
		return NULL;
	}

	Py_INCREF(applongname);
	Py_DECREF(constants);

	return applongname;
}

PyObject *plugin_app(PyObject *self, PyObject *args)
{
	Py_ssize_t windowId;
	if (!PyArg_ParseTuple(args, "n", &windowId))
	{
		return NULL;
	}

	HWND targetWindow = (HWND) windowId;

	if (!IsWindow(targetWindow))
	{
		PyErr_SetString(PyExc_ValueError, "Argument \"window\" is not a valid window handle");
		return NULL;
	}

	PyObject *applongname = ImportAppLongName();
	if (!applongname)
	{
		return NULL;
	}

	wchar_t *windowName = PyUnicode_AsWideCharString(applongname, NULL);
	if (!windowName)
	{
		goto cleanup_applongname;
	}

	if (!SetWindowSubclass(targetWindow, &ThreadResultSubclassProc, 1, 0))
	{
		PyErr_SetFromWindowsErr(0);
		goto cleanup_windowName;
	}

	void **parameter = malloc(2 * sizeof(void *));
	if (!parameter)
	{
		PyErr_NoMemory();
		goto cleanup_windowName;
	}

	parameter[0] = targetWindow;
	parameter[1] = windowName;

	if (!CreateThread(NULL, 0, &ThreadProc, parameter, 0, NULL))
	{
		PyErr_SetFromWindowsErr(0);
		goto cleanup_parameter;
	}

	Py_DECREF(applongname);
	Py_RETURN_NONE;

cleanup_parameter:
	free(parameter);
cleanup_windowName:
	PyMem_FREE(windowName);
cleanup_applongname:
	Py_DECREF(applongname);

	Py_RETURN_NONE;
}

PyObject *plugin_start3(PyObject *self, PyObject *args)
{
	return Py_BuildValue("s", "IgnoreMouse");
}

static PyMethodDef methods[] = {
	{"plugin_app", &plugin_app, METH_VARARGS, "Plugin app"},
	{"plugin_start3", &plugin_start3, METH_VARARGS, "Plugin init"},
	{NULL, NULL, 0, NULL}
};

static struct PyModuleDef moduleDef = {
	PyModuleDef_HEAD_INIT,
	"edmcignoremouse",
	NULL,
	-1,
	methods
};

PyMODINIT_FUNC PyInit_edmcignoremouse(void)
{
	return PyModule_Create(&moduleDef);
}
