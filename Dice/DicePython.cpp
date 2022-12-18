#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "DicePython.h"
#include "DiceConsole.h"
#include "EncodingConvert.h"
#include "DiceAttrVar.h"
#include "DiceMod.h"
std::unique_ptr<PyGlobal> py;
constexpr auto DiceModuleName = "dicemaid";
char* empty = "";
string py_args_to_gbstring(PyObject* o) {
    const char* s = empty;
    return (o && PyArg_ParseTuple(o, "s", &s)) ? UTF8toGBK(s) : empty;
}
string py_print_error() {
    PyObject *ptype, *val, *tb;
    PyErr_Fetch(&ptype, &val, &tb);
    PyErr_NormalizeException(&ptype, &val, &tb);
    if (tb)PyException_SetTraceback(val, tb);
    return PyUnicode_AsUTF8(PyObject_Str(val));
}
string py_to_gbstring(PyObject* o) {
    return Py_IS_TYPE(o, &PyUnicode_Type) ? UTF8toGBK(PyUnicode_AsUTF8(o)) : empty;
}
AttrVar py_to_attr(PyObject* o) {
    if (Py_IS_TYPE(o, &PyBool_Type))return bool(Py_IsTrue(o));
    else if (Py_IS_TYPE(o, &PyLong_Type)) {
        auto i{ PyLong_AsLongLong(o) };
        return (i > 10000000 || i < -100000000) ? i : (int)i;
    }
    else if (Py_IS_TYPE(o, &PyFloat_Type))return PyFloat_AsDouble(o);
    else if (Py_IS_TYPE(o, &PyUnicode_Type))return UTF8toGBK(PyUnicode_AsUTF8(o));
    else if (Py_IS_TYPE(o, &PyBytes_Type))return UTF8toGBK(PyBytes_AsString(o));
    console.log("py type: " + string(o->ob_type->tp_name), 0);
    return {};
}
PyObject* py_build_attr(const AttrVar& var) {
    switch (var.type){
    case AttrVar::AttrType::Boolean:
        return Py_BuildValue("p", var.bit);
        break;
    case AttrVar::AttrType::Integer:
        return PyLong_FromLong((long)var.attr);
        break;
    case AttrVar::AttrType::Number:
        return PyFloat_FromDouble(var.number);
        break;
    case AttrVar::AttrType::Text:
        return Py_BuildValue("s", GBKtoUTF8(var.text).c_str());
        break;
    case AttrVar::AttrType::ID:
        return PyLong_FromLongLong(var.id);
        break;
    case AttrVar::AttrType::Nil:
    default:
        return Py_BuildValue("");
        break;
    }
}

typedef struct{
	PyObject_HEAD
	AttrObject obj;
} PyContextObject;
static PyObject* PyContext_new(PyTypeObject* tp, PyObject* args, PyObject* kwds) {
    PyContextObject* self = (PyContextObject*)tp->tp_alloc(tp, 0);
    new(&self->obj) AttrObject();
    return (PyObject*)self;
}
void PyContext_dealloc(PyObject* o) {
    delete& ((PyContextObject*)o)->obj;
    Py_TYPE(o)->tp_free(o);
}
PyObject* PyContext_getattr(PyObject* self, char* attr) {
    AttrObject& obj{ ((PyContextObject*)self)->obj };
    return py_build_attr(getContextItem(obj, UTF8toGBK(attr)));
}
int PyContext_setattr(PyObject* self, char* attr, PyObject* val) {
    AttrObject& obj{ ((PyContextObject*)self)->obj };
    obj.set(UTF8toGBK(attr), py_to_attr(val));
    return 0;
}
PyObject* PyContext_getattro(PyObject* self, PyObject* attr) {
    AttrObject& obj{ ((PyContextObject*)self)->obj };
    string key{ py_to_gbstring(attr) };
    return py_build_attr(obj.get(key));
}
int PyContext_setattro(PyObject* self, PyObject* attr, PyObject* val) {
    AttrObject& obj{ ((PyContextObject*)self)->obj };
    string key{ py_to_gbstring(attr) };
    obj.set(key, py_to_attr(val));
    return 0;
}
static PyObject* pyContext_echo(PyObject* self, PyObject* args) {
    AttrObject& obj{ ((PyContextObject*)self)->obj };
    const char* msg = empty;
    bool isRaw = false;
    if(!PyArg_ParseTuple(args, "s|p", &msg, &isRaw)) {
        return nullptr;
    }
    reply(obj, UTF8toGBK(msg), !isRaw);
    return Py_BuildValue("");
}
static PyObject* pyContext_format(PyObject* self, PyObject* args) {
    AttrObject& obj{ ((PyContextObject*)self)->obj };
    const char* msg = empty;
    if (!PyArg_ParseTuple(args, "s", &msg)) {
        return nullptr;
    }
    return PyUnicode_FromString(GBKtoUTF8(fmt->format(UTF8toGBK(msg), obj, true)).c_str());
}
static PyMethodDef ContextMethods[] = {
    {"echo", pyContext_echo, METH_VARARGS, "echo message"},
    {"format", pyContext_format, METH_VARARGS, "format text"},
    {"__getattr__", PyContext_getattro, METH_VARARGS, "get Context item"},
    {NULL, NULL, 0, NULL},
};
static Py_ssize_t pyContext_size(PyObject* self) {
    AttrObject& obj{ ((PyContextObject*)self)->obj };
    return (Py_ssize_t)obj->size();
}
static PyMappingMethods ContextMappingMethods = {
    pyContext_size, PyContext_getattro, PyContext_setattro,
};
static void* ContextMembers[] = {
    //{"ptr", T_INT, offsetof(PyContextObject, obj), 0, NULL},
    {NULL},
};
static PyTypeObject PyContextType = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    "dicemaid.Context",                                 /* tp_name */
    sizeof(PyContextObject),                            /* tp_basicsize */
    0,                                                 /* tp_itemsize */
    PyContext_dealloc,                      /* tp_dealloc */
    0,                                           /* tp_print */
    nullptr,                                 /* tp_getattr */
    PyContext_setattr,                                 /* tp_setattr */
    nullptr,                                           /* tp_reserved */
    nullptr,                                           /* tp_repr */
    nullptr,                                            /* tp_as_number */
    nullptr,                                           /* tp_as_sequence */
    &ContextMappingMethods,                             /* tp_as_mapping */
    nullptr,                                           /* tp_hash  */
    nullptr,                                           /* tp_call */
    nullptr,                                          /* tp_str */
    nullptr,                                           /* tp_getattro */
    nullptr,                                          /* tp_setattro */
    nullptr,                                           /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,          /* tp_flags */
    PyDoc_STR("Custom Dice Context."),                 /* tp_doc */
    nullptr,                                           /* tp_traverse */
    nullptr,                                           /* tp_clear */
    nullptr,                                           /* tp_richcompare */
    0,                                                 /* tp_weaklistoffset */
    nullptr,                                           /* tp_iter */
    nullptr,                                           /* tp_iternext */
    ContextMethods,                                    /* tp_methods */
    (PyMemberDef*)ContextMembers,                                    /* tp_members */
    nullptr,                                          /* tp_getset */
    nullptr,                                           /* tp_base */
    nullptr,                                           /* tp_dict */
    nullptr,                                           /* tp_descr_get */
    nullptr,                                           /* tp_descr_set */
    0,                                                 /* tp_dictoffset */
    nullptr,                                           /* tp_init */
    nullptr,                                           /* tp_alloc */
    PyContext_new,                                     /* tp_new */
};
PyObject* py_newContext(const AttrObject& obj) {
    PyContextObject* context = (PyContextObject*)PyContextType.tp_alloc(&PyContextType, 0);
    new(&context->obj) AttrObject(obj);
    return (PyObject*)context;
}

static PyObject* py_log(PyObject* self, PyObject* args) {
    int lv[10] = { -1 };
    if (const char* info = empty; !PyArg_ParseTuple(args, "s|iiiiiiiiii", &info, lv, lv + 1, lv + 2, lv + 3, lv + 4, lv + 5, lv + 6, lv + 7, lv + 8, lv + 9))return NULL;
    else {
        int note_lv{ 0 };
        for (int i = 0; i < 10;++i) {
            if (lv[i] < 0 || lv[i] > 9)continue;
            else note_lv |= (1 << lv[i]);
        }
        console.log(UTF8toGBK(info), note_lv);
    }
    return Py_BuildValue("");
}
static PyObject* py_sendMsg(PyObject* self, PyObject* args, PyObject* keys) {
    const char* msg = empty;
    long long uid = 0, gid = 0, chid = 0;
    static char* kwlist[] = { "msg","uid","gid","chid",NULL };
    /* Parse arguments */
    if (!PyArg_ParseTupleAndKeywords(args, keys, "s|LLL", kwlist, &msg, &uid, &gid, &chid))return NULL;
    AttrObject chat;
    if (uid)chat["uid"] = uid;
    if (gid)chat["gid"] = gid;
    if (chid)chat["chid"] = chid;
    if (!gid && !uid)return 0;
    AddMsgToQueue(fmt->format(UTF8toGBK(msg), chat), { uid,gid,chid });
    return Py_BuildValue("");
}
#define REG(name) #name,(PyCFunction)py_##name
static PyMethodDef DiceMethods[] = {
	{REG(log), METH_VARARGS, "output log to command&notice"},
    {REG(sendMsg), METH_VARARGS|METH_KEYWORDS, "send message"},
	{NULL, NULL, 0, NULL}
}; 
static PyModuleDef DiceMaidDef = {
	PyModuleDef_HEAD_INIT,
	DiceModuleName,   //导出的模块名称
	"Python interface for Dice function",      //模块文档，可以为NULL
	-1,       //一般为-1
	DiceMethods //PyMethodDef类型，导出方法
}; 
PyMODINIT_FUNC PyInit_DiceMaid(){
    if (PyType_Ready(&PyContextType)) console.log("定义dicemaid.Context失败!" + py_print_error(), 0b1000);
    Py_INCREF(&PyContextType);
	auto mod = PyModule_Create(&DiceMaidDef);
    if (PyModule_AddObject(mod, "Context", (PyObject*)&PyContextType) < 0) {
        console.log("注册dicemaid.Context失败!", 0b1000);
        Py_DECREF(&PyContextType);
        Py_DECREF(mod);
        return NULL;
    }
    return mod;
}
PyGlobal::PyGlobal() {
    if (std::filesystem::exists(dirExe / "bin" / "python3.dll") || std::filesystem::exists(dirExe / "bin" / "python3.so")) {
        Py_SetPythonHome((dirExe / "bin").wstring().c_str());
    }
    else if (std::filesystem::exists(dirExe / "python3.dll")) {
        Py_SetPythonHome(dirExe.wstring().c_str());
    }
	if (PyImport_AppendInittab(DiceModuleName, PyInit_DiceMaid)) {
		console.log("预载dicemaid模块失败!", 0b1000);
	}
	//Py_SetProgramName(L"DiceMaid");
	try {
		Py_Initialize();
		PyRun_SimpleString("import sys");
		PyRun_SimpleString(("sys.path.append('" + (DiceDir / "plugin").u8string() + "/')").c_str());
        PyRun_SimpleString(("sys.path.append('" + (dirExe / "Diceki" / "py").u8string() + "/')").c_str());
        PyRun_SimpleString("from dicemaid import *");
	}
	catch (std::exception& e) {
		console.log("python初始化失败" + string(e.what()), 0b1);
	}
	console.log("Python.Initialized", 0);
}
PyGlobal::~PyGlobal() {
	Py_Finalize();
}
int PyGlobal::runFile(const std::filesystem::path& p) {
	//PyRun_SimpleFile(p.c_str());
}
int PyGlobal::runString(const std::string& s) {
	return PyRun_SimpleString(GBKtoUTF8(s).c_str());
}
AttrVar PyGlobal::evalString(const std::string& s, const AttrObject& context) {
    try {
        PyObject* mainModule = PyImport_ImportModule("__main__");
        PyObject* dict = PyModule_GetDict(mainModule);
        if (auto ret{ PyRun_String(GBKtoUTF8(s).c_str(), Py_eval_input, dict, py_newContext(context)) }; !ret) {
            console.log(getMsg("self") + "运算python语句异常!" + py_print_error(), 0b10);
            PyErr_Clear();
        }
        else return py_to_attr(ret);
    }
    catch (std::exception& e) {
        console.log(getMsg("self") + "运行python脚本异常!" + e.what(), 0b10);
    }
    return {};
}
bool PyGlobal::call_reply(DiceEvent* msg, const AttrObject& action) {
    try {
        PyObject* mainModule = PyImport_ImportModule("__main__");
        PyObject* dict = PyModule_GetDict(mainModule);
        PyObject* locals = PyDict_New();
        PyDict_SetItem(locals, Py_BuildValue("s", "msg"), py_newContext(*msg));
        string pyScript{ action.get_str("script") };
        if (fmt->script_has(pyScript)) {
            return false;
        }
        else {
            if (auto res{ PyRun_String(GBKtoUTF8(pyScript).c_str(), Py_file_input, dict, locals) };!res) {
                console.log(getMsg("self") + "运行python脚本异常!" + py_print_error(), 0b10);
                PyErr_Clear();
            }
        }
        return true;
    }
    catch (std::exception& e) {
        console.log(getMsg("self") + "运行python脚本异常!" + e.what(), 0b10);
        return false;
    }
}