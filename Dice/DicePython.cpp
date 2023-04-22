#include "DicePython.h"
#ifdef DICE_PYTHON
#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include <stdio.h>
#include "DiceConsole.h"
#include "EncodingConvert.h"
#include "DiceAttrVar.h"
#include "DiceSelfData.h"
#include "CharacterCard.h"
#include "DiceMod.h"
#include "DDAPI.h"
std::unique_ptr<PyGlobal> py;
constexpr auto DiceModuleName = "dicemaid";
const char* empty = "";
const wchar_t* wempty = L"";
string py_args_to_gbstring(PyObject* o) {
	auto s = wempty;
	return (o && PyArg_ParseTuple(o, "u", &s)) ? UtoGBK(s) : empty;
}
string py_print_error() {
	PyObject* ptype{ nullptr }, * val{ nullptr }, * tb{ nullptr };
	PyErr_Fetch(&ptype, &val, &tb);
	PyErr_NormalizeException(&ptype, &val, &tb);
	if (tb)PyException_SetTraceback(val, tb);
	return PyUnicode_AsUTF8(PyObject_Str(val));
}
string py_to_gbstring(PyObject* o) {
	return Py_IS_TYPE(o, &PyUnicode_Type) ? UtoGBK(PyUnicode_AsUnicode(o))
		: Py_IS_TYPE(o, &PyTuple_Type) ? py_to_gbstring(PyTuple_GetItem(o, 0))
		: empty;
}
string py_to_native_string(PyObject* o) {
#ifdef _WIN32
	return Py_IS_TYPE(o, &PyUnicode_Type) ? UtoGBK(PyUnicode_AsUnicode(o)) : empty;
#else
	return Py_IS_TYPE(o, &PyUnicode_Type) ? PyUnicode_AsUTF8(o) : empty;
#endif
}
PyObject* py_from_gbstring(const string& strGBK) {
#ifdef _WIN32
	const int len = MultiByteToWideChar(54936, 0, strGBK.c_str(), -1, nullptr, 0);
	auto* const strUnicode = new wchar_t[len];
	MultiByteToWideChar(54936, 0, strGBK.c_str(), -1, strUnicode, len);
	auto o{ PyUnicode_FromUnicode(strUnicode,len - 1) };
	delete[] strUnicode;
	return o;
#else
	auto s{ ConvertEncoding<wchar_t, char>(strGBK, "gb18030", "utf-32le") };
	return PyUnicode_FromUnicode(s.c_str(), s.length());
#endif
}
AttrVar py_to_attr(PyObject* o) {
	if (!o)return {};
	else if (Py_IS_TYPE(o, &PyBool_Type))return bool(Py_IsTrue(o));
	else if (Py_IS_TYPE(o, &PyLong_Type)) {
		auto i{ PyLong_AsLongLong(o) };
		return (i == (int)i) ? (int)i : i;
	}
	else if (Py_IS_TYPE(o, &PyFloat_Type))return PyFloat_AsDouble(o);
	else if (Py_IS_TYPE(o, &PyUnicode_Type))return UtoGBK(PyUnicode_AsUnicode(o));
	else if (Py_IS_TYPE(o, &PyBytes_Type))return UTF8toGBK(PyBytes_AsString(o));
	else if (Py_IS_TYPE(o, &PyDict_Type)) {
		AttrVars tab;
		int len = PyMapping_Length(o);
		if (auto items{ PyMapping_Items(o) };items && len) {
			PyObject* item = nullptr;
			auto key = wempty;
			PyObject* val = nullptr;
			for (int i = 0; i < len; ++i) {
				item = PySequence_GetItem(items, i);
				PyArg_ParseTuple(item, "uO", key, val);
				tab[UtoGBK(key)] = py_to_attr(val);
				Py_DECREF(item);
			}
		}
		return AttrObject(tab);
	}
	else if (Py_IS_TYPE(o, &PyList_Type)) {
		VarArray ary;
		if (auto len = PySequence_Size(o)) {
			PyObject* item = nullptr;
			for (int i = 0; i < len; ++i) {
				item = PySequence_GetItem(o, i);
				ary.emplace_back(py_to_attr(item));
				Py_DECREF(item);
			}
		}
		return AttrObject(ary);
	}
	else if (Py_IS_TYPE(o, &PyTuple_Type)) {
		VarArray ary;
		if (auto len = PyTuple_Size(o)) {
			PyObject* item = nullptr;
			for (int i = 0; i < len; ++i) {
				item = PyTuple_GetItem(o, i);
				ary.emplace_back(py_to_attr(item));
				Py_DECREF(item);
			}
		}
		return AttrObject(ary);
	}
	console.log("py type: " + string(o->ob_type->tp_name), 0);
	return {};
}
PyObject* py_build_attr(const AttrVar& var) {
	switch (var.type){
	case AttrVar::AttrType::Boolean:
		if (var.bit) Py_RETURN_TRUE;
		else Py_RETURN_FALSE;
		break;
	case AttrVar::AttrType::Integer:
		return PyLong_FromSsize_t(var.attr);
		break;
	case AttrVar::AttrType::Number:
		return PyFloat_FromDouble(var.number);
		break;
	case AttrVar::AttrType::Text:
		return PyUnicode_FromString(GBKtoUTF8(var.text).c_str());
		break;
	case AttrVar::AttrType::Table:
		if (!var.table.to_dict()->empty()) {
			auto dict = PyDict_New();
			if (var.table.to_list()) {
				long idx{ 0 };
				for (auto& val : *var.table.to_list()) {
					PyDict_SetItem(dict, PyLong_FromLong(idx++), py_build_attr(val));
				}
			}
			for (auto& [key, val] : *var.table.to_dict()) {
				PyDict_SetItem(dict, py_from_gbstring(key), py_build_attr(val));
			}
			return dict;
		}
		else if (var.table.to_list()) {
			auto ary = PyList_New(var.table.to_list()->size());
			Py_ssize_t i{ 0 };
			for (auto& val : *var.table.to_list()) {
				PyList_SetItem(ary, i++, py_build_attr(val));
			}
			return ary;
		}
		break;
	case AttrVar::AttrType::ID:
		return PyLong_FromLongLong(var.id);
		break;
	case AttrVar::AttrType::Nil:
	default:
		break;
	}
	return Py_BuildValue("");
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
	return PyUnicode_FromString(GBKtoUTF8(fmt->format(UTF8toGBK(msg), obj)).c_str());
}
static PyObject* pyContext_inc(PyObject* self, PyObject* args) {
	AttrObject& obj{ ((PyContextObject*)self)->obj };
	const char* field = empty;
	int cnt = 0;
	if (!PyArg_ParseTuple(args, "s|i", &field, &cnt)) {
		return nullptr;
	}
	return PyLong_FromSsize_t(obj.inc(UTF8toGBK(field, cnt)));
}
static PyMethodDef ContextMethods[] = {
	{"echo", pyContext_echo, METH_VARARGS, "echo message"},
	{"format", pyContext_format, METH_VARARGS, "format text"},
	{"inc", pyContext_inc, METH_VARARGS, "attr auto increase"},
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
	nullptr,                                    /* tp_members */
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
AttrObject py_to_obj(PyObject* o) {
	if (Py_IS_TYPE(o, &PyDict_Type)) {
		AttrVars tab;
		int len = PyMapping_Length(o);
		if (auto items{ PyMapping_Items(o) }; items && len) {
			PyObject* item = nullptr;
			const char* key = empty;
			PyObject* val = nullptr;
			for (int i = 0; i < len; ++i) {
				item = PySequence_GetItem(items, i);
				PyArg_ParseTuple(item, "sO", key, val);
				tab[UTF8toGBK(key)] = py_to_attr(val);
				Py_DECREF(item);
			}
		}
		return AttrObject(tab);
	}
	else if (Py_IS_TYPE(o, &PyList_Type)) {
		VarArray ary;
		if (auto len = PySequence_Size(o)) {
			PyObject* item = nullptr;
			for (int i = 0; i < len; ++i) {
				item = PySequence_GetItem(o, i);
				ary.emplace_back(py_to_attr(item));
				Py_DECREF(item);
			}
		}
		return AttrObject(ary);
	}
	else if (Py_IS_TYPE(o, &PyTuple_Type)) {
		VarArray ary;
		if (auto len = PyTuple_Size(o)) {
			PyObject* item = nullptr;
			for (int i = 0; i < len; ++i) {
				item = PyTuple_GetItem(o, i);
				ary.emplace_back(py_to_attr(item));
				Py_DECREF(item);
			}
		}
		return AttrObject(ary);
	}
	else if (Py_IS_TYPE(o, &PyContextType)) {
		return ((PyContextObject*)o)->obj;
	}
	return {};
}
#define PYDEF(name) static PyObject* py_##name(PyObject* self)
#define PYDEFARG(name) static PyObject* py_##name(PyObject* self, PyObject* args)
#define PYDEFKEY(name) static PyObject* py_##name(PyObject* self, PyObject* args, PyObject* keys)
PYDEFARG(log) {
	int lv[10] = { -1 };
	if (PyObject* info{ nullptr }; PyArg_ParseTuple(args, "U|iiiiiiiiii", &info, lv, lv + 1, lv + 2, lv + 3, lv + 4, lv + 5, lv + 6, lv + 7, lv + 8, lv + 9)) {
		int note_lv{ 0 };
		for (int i = 0; i < 10; ++i) {
			if (lv[i] < 0 || lv[i] > 9)continue;
			else note_lv |= (1 << lv[i]);
		}
		console.log(py_to_gbstring(info), note_lv);
		return Py_BuildValue("");
	}
	else return NULL;
}
PYDEF(getDiceID) {
	return PyLong_FromLongLong(console.DiceMaid);
}
PYDEF(getDiceDir) {
	auto dir{ DiceDir.wstring() };
	return PyUnicode_FromUnicode(dir.c_str(), (Py_ssize_t)dir.length());
}
typedef struct {
	PyObject_HEAD
	ptr<SelfData> p;
} PySelfDataObject;

static PyObject* PySelfData_new(PyTypeObject* tp, PyObject* args, PyObject* kwds) {
	PySelfDataObject* self = (PySelfDataObject*)tp->tp_alloc(tp, 0);
	new(&self->p) ptr<SelfData>();
	return (PyObject*)self;
}
void PySelfData_dealloc(PyObject* o) {
	delete& ((PySelfDataObject*)o)->p;
	Py_TYPE(o)->tp_free(o);
}
PyObject* PySelfData_getattr(PyObject* self, char* attr) {
	auto& data{ ((PySelfDataObject*)self)->p->data };
	return data.is_table() ? py_build_attr(data.table.get(UTF8toGBK(attr))) : Py_BuildValue("");
}
int PySelfData_setattr(PyObject* self, char* attr, PyObject* val) {
	auto& data{ ((PySelfDataObject*)self)->p->data };
	if (data.is_table())data.table.set(UTF8toGBK(attr), py_to_attr(val));
	return 0;
}
PyObject* PySelfData_getattro(PyObject* self, PyObject* attr) {
	auto& data{ ((PySelfDataObject*)self)->p };
	if (!attr)return py_build_attr(data->data);
	string key{ py_to_gbstring(attr) };
	return data && data->data.is_table() ? py_build_attr(data->data.table.get(key)) : Py_BuildValue("");
}
int PySelfData_setattro(PyObject* self, PyObject* attr, PyObject* val) {
	auto& data{ ((PySelfDataObject*)self)->p->data };
	string key{ py_to_gbstring(attr) };
	if (data.is_table())data.table.set(key, py_to_attr(val));
	return 0;
}
static Py_ssize_t pySelfData_size(PyObject* self) {
	auto& p{ ((PySelfDataObject*)self)->p };
	return p && p->data.is_table() ? (Py_ssize_t)p->data.table->size() : 0;
}
PyObject* PySelfData_set(PyObject* self, PyObject* args) {
	auto data{ ((PySelfDataObject*)self)->p };
	PyObject* item{ nullptr }, * val{ nullptr };
	if (PyArg_ParseTuple(args, "O|O", &item, &val)) {
		if (Py_IS_TYPE(item, &PyUnicode_Type) && data->data.is_table()) {
			data->data.table.set(py_to_gbstring(item), py_to_attr(val));
			data->save();
		}
		else if (!val) {
			data->data = py_to_attr(item);
			data->save();
		}
		return Py_BuildValue("");
	}
	return NULL;
}
static PyMethodDef SelfDataMethods[] = {
	{"set", PySelfData_set, METH_VARARGS, "set SelfData item"},
	{"get", PySelfData_getattro, METH_VARARGS, "get SelfData item"},
	{"__getattr__", PySelfData_getattro, METH_VARARGS, "get SelfData item"},
	{NULL, NULL, 0, NULL},
};
static PyMappingMethods SelfDataMappingMethods = {
	pySelfData_size, PySelfData_getattro, PySelfData_setattro,
};
static PyTypeObject PySelfData_Type = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"dicemaid.SelfData",                                 /* tp_name */
	sizeof(PySelfDataObject),                            /* tp_basicsize */
	0,                                                 /* tp_itemsize */
	PySelfData_dealloc,                      /* tp_dealloc */
	0,                                           /* tp_print */
	nullptr,                                 /* tp_getattr */
	PySelfData_setattr,                                 /* tp_setattr */
	nullptr,                                           /* tp_reserved */
	nullptr,                                           /* tp_repr */
	nullptr,                                            /* tp_as_number */
	nullptr,                                           /* tp_as_sequence */
	&SelfDataMappingMethods,                             /* tp_as_mapping */
	nullptr,                                           /* tp_hash  */
	nullptr,                                           /* tp_call */
	nullptr,                                          /* tp_str */
	nullptr,                                           /* tp_getattro */
	nullptr,                                          /* tp_setattro */
	nullptr,                                           /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,						          /* tp_flags */
	PyDoc_STR("Custom Dice SelfData."),                 /* tp_doc */
	nullptr,                                           /* tp_traverse */
	nullptr,                                           /* tp_clear */
	nullptr,                                           /* tp_richcompare */
	0,                                                 /* tp_weaklistoffset */
	nullptr,                                           /* tp_iter */
	nullptr,                                           /* tp_iternext */
	SelfDataMethods,                                    /* tp_methods */
	nullptr,                                    /* tp_members */
	nullptr,                                          /* tp_getset */
	nullptr,                                           /* tp_base */
	nullptr,                                           /* tp_dict */
	nullptr,                                           /* tp_descr_get */
	nullptr,                                           /* tp_descr_set */
	0,                                                 /* tp_dictoffset */
	nullptr,                                           /* tp_init */
	nullptr,                                           /* tp_alloc */
	PySelfData_new,                                     /* tp_new */
};
PYDEFARG(getSelfData) {
	if (PyObject* s{ nullptr }; PyArg_ParseTuple(args, "U", &s)){
		auto file = py_to_native_string(s);
		if (!selfdata_byFile.count(file)) {
			auto& data{ selfdata_byFile[file] = std::make_shared<SelfData>(DiceDir / "selfdata" / file) };
			if (string name{ cut_stem(file) }; !selfdata_byStem.count(name)) {
				selfdata_byStem[name] = data;
			}
		}
		auto data = (PySelfDataObject*)PySelfData_Type.tp_alloc(&PySelfData_Type, 0);
		Py_INCREF(data);
		new(&data->p) ptr<SelfData>(selfdata_byStem[file]);
		return (PyObject*)data;
	}
	return NULL;
}
PYDEFKEY(getGroupAttr) {
	static const char* kwlist[] = { "id","attr","sub", NULL };
	PyObject *id{ nullptr }, *sub{ nullptr };
	auto attr = wempty;
	if (!PyArg_ParseTupleAndKeywords(args, keys, "|OuO", (char**)kwlist, &id, &attr, &sub))return NULL;
	string item;
	if (attr && (item = UtoGBK(attr))[0] == '&')item = fmt->format(item);
	if (!id) {
		if (item.empty()) {
			PyErr_SetString(PyExc_TypeError,"neither id and attr is none/empty");
			return NULL;
		}
		auto items{ PyDict_New() };
		for (auto& [gid, data] : ChatList) {
			if (data.confs.has(item))PyDict_SetItem(items, PyLong_FromLongLong(gid), py_build_attr(data.confs.get(item)));
		}
		return items;
	}
	long long gid{ PyLong_AsLongLong(id) };
	if (!gid) {
		PyErr_SetString(PyExc_ValueError, "group id can't be zero");
		return NULL;
	}
	if (item.empty()) return py_newContext(chat(gid).confs);
	else if (item == "members") {
		if (Py_IS_TYPE(sub, &PyUnicode_Type)) {
			string subitem{ py_to_gbstring(sub)};
			auto items{ PyDict_New() };
			if (subitem == "card") {
				for (auto uid : DD::getGroupMemberList(gid)) {
					PyDict_SetItem(items, PyLong_FromLongLong(uid), py_from_gbstring(DD::getGroupNick(gid, uid)));
				}
			}
			else if (subitem == "lst") {
				for (auto uid : DD::getGroupMemberList(gid)) {
					if (auto lst{ DD::getGroupLastMsg(gid, uid) }) {
						PyDict_SetItem(items, PyLong_FromLongLong(uid), PyLong_FromLongLong(lst));
					}
					else PyDict_SetItem(items, PyLong_FromLongLong(uid), Py_BuildValue("p", false));
				}
			}
			else if (subitem == "auth") {
				for (auto uid : DD::getGroupMemberList(gid)) {
					if (auto auth{ DD::getGroupAuth(gid, uid, 0) }) {
						PyDict_SetItem(items, PyLong_FromLongLong(uid), PyLong_FromSsize_t(auth));
					}
					else PyDict_SetItem(items, PyLong_FromLongLong(uid), Py_BuildValue("p", false));
				}
			}
			return items;
		}
		else {
			auto items{ PySet_New(NULL) };
			for (auto uid : DD::getGroupMemberList(gid)) {
				PySet_Add(items, PyLong_FromLongLong(uid));
			}
			return items;
		}
	}
	else if (item == "admins") {
		auto items{ PySet_New(NULL) };
		for (auto uid : DD::getGroupAdminList(gid)) {
			PySet_Add(items, PyLong_FromLongLong(uid));
		}
		return items;
	}
	if (auto val{ getGroupItem(gid,item) }; !val.is_null())return py_build_attr(val);
	else if (sub) return sub;
	return Py_BuildValue("");
}
PYDEFARG(setGroupAttr) {
	long long id = 0;
	auto attr = wempty;
	PyObject* val{ nullptr };
	if (!PyArg_ParseTuple(args, "Lu|O", &id, &attr, &val))return NULL;
	if (!id) {
		PyErr_SetString(PyExc_ValueError, "group id can't be zero");
		return NULL;
	}
	string item{ UtoGBK(attr) };
	if (item[0] == '&')item = fmt->format(item);
	if (item.empty())return 0;
	Chat& grp{ chat(id) };
	if (item.find("card#") == 0) {
		long long uid{ 0 };
		if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
			uid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
		}
		string card{ py_to_gbstring(val) };
		DD::setGroupCard(id, uid, card);
	}
	else if (!val) {
		grp.reset(item);
	}
	else grp.set(item, py_to_attr(val));
	return Py_BuildValue("");
}
PYDEFKEY(getUserAttr) {
	static const char* kwlist[] = { "id","attr","sub", NULL };
	PyObject* id{ nullptr }, * sub{ nullptr };
	auto attr = wempty;
	if (!PyArg_ParseTupleAndKeywords(args, keys, "|OuO", (char**)kwlist, &id, &attr, &sub))return NULL;
	string item;
	if (attr && (item = UtoGBK(attr))[0] == '&')item = fmt->format(item);
	if (!id) {
		if (item.empty()) {
			PyErr_SetString(PyExc_TypeError, "neither id and attr is none/empty");
			return NULL;
		}
		auto items{ PyDict_New() };
		for (auto& [uid, data] : UserList) {
			if (data.confs.has(item))PyDict_SetItem(items, PyLong_FromLongLong(uid), py_build_attr(data.confs.get(item)));
		}
		return items;
	}
	long long uid{ PyLong_AsLongLong(id) };
	if (!uid) {
		PyErr_SetString(PyExc_ValueError, "uid can't be zero");
		return NULL;
	}
	if (item.empty()) return py_newContext(getUser(uid).confs);
	if (auto val{ getUserItem(uid,item) }; !val.is_null())return py_build_attr(val);
	else return sub ? sub : Py_BuildValue("");
}
PYDEFARG(setUserAttr) {
	long long id = 0;
	auto attr = wempty;
	PyObject* val{ nullptr };
	if (!PyArg_ParseTuple(args, "Lu|O", &id, &attr, &val))return NULL;
	if (!id) {
		PyErr_SetString(PyExc_ValueError, "User id can't be zero");
		return NULL;
	}
	string item{ UtoGBK(attr) };
	if (item[0] == '&')item = fmt->format(item);
	if (item.empty())return 0;
	if (item == "trust") {
		int trust{ (int)PyLong_AsLongLong(val) };
		User& user{ getUser(id) };
		if (trust < 5 && trust >= 0 && user.nTrust < 5) {
			user.trust(trust);
		}
		else {
			PyErr_SetString(PyExc_ValueError, "User trust invalid");
			return NULL;
		}
	}
	else if (item.find("nn#") == 0) {
		long long gid{ 0 };
		if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
			gid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
		}
		if (!val) {
			getUser(id).rmNick(gid);
		}
		else {
			getUser(id).setNick(gid, py_to_attr(val));
		}
	}
	else if (!val) {
		getUser(id).rmConf(item);
	}
	else getUser(id).setConf(item, py_to_attr(val));
	return Py_BuildValue("");
}
PYDEFKEY(getUserToday) {
	static const char* kwlist[] = { "id","attr","sub", NULL };
	PyObject* id{ nullptr }, * sub{ nullptr };
	auto attr = wempty;
	if (!PyArg_ParseTupleAndKeywords(args, keys, "|OuO", (char**)kwlist, &id, &attr, &sub))return NULL;
	string item;
	if (attr && (item = UtoGBK(attr))[0] == '&')item = fmt->format(item);
	if (!id) {
		if (item.empty()) {
			PyErr_SetString(PyExc_TypeError, "neither id and attr is none/empty");
			return NULL;
		}
		auto items{ PyDict_New() };
		for (auto& [uid, data] : today->getUserInfo()) {
			if (data.has(item))PyDict_SetItem(items, PyLong_FromLongLong(uid), py_build_attr(data.get(item)));
		}
		return items;
	}
	long long uid{ PyLong_AsLongLong(id) };
	if (!uid) {
		PyErr_SetString(PyExc_ValueError, "uid can't be zero");
		return NULL;
	}
	if (item.empty()) return py_newContext(today->get(uid));
	else if (item == "jrrp")
		return py_build_attr(today->getJrrp(uid));
	else if (auto p{ today->get_if(uid, item) })
		return py_build_attr(*p);
	else if (sub) return sub;
	return Py_BuildValue("");
}
PYDEFARG(setUserToday) {
	long long id = 0;
	auto attr = wempty;
	PyObject* val{ nullptr };
	if (!PyArg_ParseTuple(args, "Lu|O", &id, &attr, &val))return NULL;
	if (!id) {
		PyErr_SetString(PyExc_ValueError, "User id can't be zero");
		return NULL;
	}
	string item{ UtoGBK(attr) };
	if (item[0] == '&')item = fmt->format(item);
	if (item.empty())return 0;
	else if (!val) {
		today->set(id, item, {});
	}
	else
		today->set(id, item, py_to_attr(val));
	return Py_BuildValue("");
}
typedef struct {
	PyObject_HEAD
	PC p;
} PyActorObject;
#define PY2PC(self) PC pc{((PyActorObject*)self)->p}
static PyObject* PyActor_new(PyTypeObject* tp, PyObject* args, PyObject* kwds) {
	static const char* kwlist[] = { "name","type", NULL }; 
	auto n = wempty, t = wempty;
	if (PyArg_ParseTuple(args, "u|u", &n, &t)) {
		PyActorObject* self = (PyActorObject*)tp->tp_alloc(tp, 0);
		string name{ UtoGBK(n) };
		new(&self->p) PC(t[0]
			? std::make_shared<CharaCard>(name, UtoGBK(t))
			: std::make_shared<CharaCard>(name));
		return (PyObject*)self;
	}
	else return NULL;
}
void PyActor_dealloc(PyObject* o) {
	delete& ((PyActorObject*)o)->p;
	Py_TYPE(o)->tp_free(o);
}
PyObject* PyActor_getattr(PyObject* self, char* attr) {
	PY2PC(self);
	return pc ? py_build_attr(pc->get(UTF8toGBK(attr))) : Py_BuildValue("");
}
int PyActor_setattr(PyObject* self, char* attr, PyObject* val) {
	PY2PC(self);
	if(pc)pc->set(UTF8toGBK(attr), py_to_attr(val));
	return 0;
}
PyObject* PyActor_getattro(PyObject* self, PyObject* attr) {
	PY2PC(self);
	if (!attr && pc)return py_build_attr(pc->Attr);
	string key{ py_to_gbstring(attr) };
	return pc ? py_build_attr(pc->get(key)) : Py_BuildValue("");
}
int PyActor_setattro(PyObject* self, PyObject* attr, PyObject* val) {
	PY2PC(self);
	string key{ py_to_gbstring(attr) };
	if (pc)pc->set(key, py_to_attr(val));
	return 0;
}
static Py_ssize_t pyActor_size(PyObject* self) {
	PY2PC(self);
	return pc ? (Py_ssize_t)pc->Attr->size() : 0;
}
PyObject* PyActor_set(PyObject* self, PyObject* args) {
	PY2PC(self);
	PyObject* item{ nullptr }, * val{ nullptr };
	if (PyArg_ParseTuple(args, "O|O", &item, &val) && pc) {
		if (Py_IS_TYPE(item, &PyUnicode_Type) && val) {
			return pc->set(py_to_gbstring(item), py_to_attr(val))
				? Py_BuildValue("") :PyLong_FromSize_t(1);
		}
		else if (Py_IS_TYPE(item, &PyDict_Type)) {
			size_t cnt = 0;
			if (int len = PyMapping_Length(item)) {
				auto items{ PyMapping_Items(item) };
				auto key = wempty;
				for (int i = 0; i < len; ++i) {
					item = PySequence_GetItem(items, i);
					PyArg_ParseTuple(item, "sO", &key, &val);
					if(!pc->set(UtoGBK(key), py_to_attr(val)))++cnt;
					Py_DECREF(item);
				}
				Py_DECREF(items);
			}
			return PyLong_FromSize_t(cnt);
		}
	}
	return NULL;
}

static PyObject* PyActor_rollDice(PyObject* self, PyObject* args) {
	auto res = PyDict_New();
	PC& pc{ ((PyActorObject*)self)->p };
	PyObject* arg = PyTuple_GetItem(args, 1);
	string exp{ Py_IS_TYPE(arg, &PyUnicode_Type) ? py_args_to_gbstring(args)
		: pc->available("__DefaultDiceExp") ? pc->get("__DefaultDiceExp").to_str()
		: "D" };
	int diceFace{ pc->get("__DefaultDice").to_int() };
	RD rd{ py_args_to_gbstring(args), diceFace ? diceFace : 100 };
	PyDict_SetItem(res, PyUnicode_FromString("expr"), py_from_gbstring(rd.strDice));
	if (int_errno err = rd.Roll(); !err) {
		PyDict_SetItem(res, PyUnicode_FromString("sum"), PyLong_FromSsize_t(rd.intTotal));
		PyDict_SetItem(res, PyUnicode_FromString("expansion"), py_from_gbstring(rd.FormCompleteString()));
	}
	else {
		PyDict_SetItem(res, PyUnicode_FromString("error"), PyLong_FromSsize_t((int32_t)err));
	}
	return res;
}
static PyMethodDef ActorMethods[] = {
	{"set", PyActor_set, METH_VARARGS, "set PC attr"},
	{"get", PyActor_getattro, METH_VARARGS, "get PC item"},
	{"__getattr__", PyActor_getattro, METH_VARARGS, "get PC item"},
	{"rollDice", PyActor_rollDice, METH_VARARGS, "PC roll dice expression"},
	{NULL, NULL, 0, NULL},
};
static PyMappingMethods ActorMappingMethods = {
	pyActor_size, PyActor_getattro, PyActor_setattro,
};
static PyTypeObject PyActor_Type = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"dicemaid.Actor",                                 /* tp_name */
	sizeof(PyActorObject),                            /* tp_basicsize */
	0,                                                 /* tp_itemsize */
	PyActor_dealloc,                      /* tp_dealloc */
	0,                                           /* tp_print */
	nullptr,                                 /* tp_getattr */
	PyActor_setattr,                                 /* tp_setattr */
	nullptr,                                           /* tp_reserved */
	nullptr,                                           /* tp_repr */
	nullptr,                                            /* tp_as_number */
	nullptr,                                           /* tp_as_sequence */
	&ActorMappingMethods,                             /* tp_as_mapping */
	nullptr,                                           /* tp_hash  */
	nullptr,                                           /* tp_call */
	nullptr,                                          /* tp_str */
	nullptr,                                           /* tp_getattro */
	nullptr,                                          /* tp_setattro */
	nullptr,                                           /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,						          /* tp_flags */
	PyDoc_STR("Dice Player Card."),                 /* tp_doc */
	nullptr,                                           /* tp_traverse */
	nullptr,                                           /* tp_clear */
	nullptr,                                           /* tp_richcompare */
	0,                                                 /* tp_weaklistoffset */
	nullptr,                                           /* tp_iter */
	nullptr,                                           /* tp_iternext */
	ActorMethods,                                    /* tp_methods */
	nullptr,                                    /* tp_members */
	nullptr,                                          /* tp_getset */
	nullptr,                                           /* tp_base */
	nullptr,                                           /* tp_dict */
	nullptr,                                           /* tp_descr_get */
	nullptr,                                           /* tp_descr_set */
	0,                                                 /* tp_dictoffset */
	nullptr,                                           /* tp_init */
	nullptr,                                           /* tp_alloc */
	PyActor_new,                                     /* tp_new */
};
PYDEFKEY(getPlayerCard) {
	static const char* kwlist[] = { "uid","gid","name", NULL };
	long long uid = 0, gid = 0;
	auto name = wempty;
	if (!PyArg_ParseTupleAndKeywords(args, keys, "L|LO", (char**)kwlist, &uid, &gid, &name))return NULL;
	if (uid) {
		Player& pl{ getPlayer(uid) };
		auto pc = (PyActorObject*)PyActor_Type.tp_alloc(&PyActor_Type, 0);
		Py_INCREF(pc);
		if (gid) {
			new(&pc->p) PC(pl[gid]);
		}
		else if (name[0]) {
			new(&pc->p) PC(pl[UtoGBK(name)]);
		}
		else {
			new(&pc->p) PC(pl[0]);
		}
		return (PyObject*)pc;
	}
	PyErr_SetString(PyExc_ValueError, "uid cannot be zero");
	return NULL;
}
PYDEFKEY(sendMsg) {
	PyObject* msg = nullptr;
	long long uid = 0, gid = 0, chid = 0;
	static const char* kwlist[] = { "msg","uid","gid","chid", NULL };
	/* Parse arguments */
	if (!PyArg_ParseTupleAndKeywords(args, keys, "O|LLL", (char**)kwlist, &msg, &uid, &gid, &chid))return NULL;
	AttrObject chat;
	if (Py_IS_TYPE(msg, &PyUnicode_Type)) {
		chat = py_to_obj(msg);
		AddMsgToQueue(fmt->format(chat.get_str("fwdMsg"), chat), chatInfo{ chat.get_ll("uid") ,chat.get_ll("gid") ,chat.get_ll("chid") });
	}
	else {
		if (!gid && !uid) {
			PyErr_SetString(PyExc_ValueError, "chat id can't be zero");
			return NULL;
		}
		if (uid)chat["uid"] = uid;
		if (gid)chat["gid"] = gid;
		if (chid)chat["chid"] = chid;
		AddMsgToQueue(fmt->format(py_to_gbstring(msg), chat), { uid,gid,chid });
	}
	return Py_BuildValue("");
}
PYDEFKEY(eventMsg) {
	PyObject* msg = nullptr;
	long long uid = 0, gid = 0, chid = 0;
	static const char* kwlist[] = { "msg","uid","gid","chid", NULL };
	/* Parse arguments */
	if (!PyArg_ParseTupleAndKeywords(args, keys, "O|LLL", (char**)kwlist, &msg, &uid, &gid, &chid))return NULL;
	if (!msg) {
		PyErr_SetString(PyExc_TypeError, "msg can't be none");
		return NULL;
	}
	AttrObject eve;
	if (Py_IS_TYPE(msg, &PyUnicode_Type)) {
		eve = py_to_obj(msg);
	}
	else {
		string fromMsg{ py_to_gbstring(msg)};
		eve = gid
			? AttrVars{ {"fromMsg",fromMsg},{"gid",gid}, {"uid", uid} }
		: AttrVars{ {"fromMsg",fromMsg}, {"uid", uid} };
	}
	std::thread th([=]() {
		DiceEvent msg(eve);
		msg.virtualCall();
		});
	th.detach();
	return Py_BuildValue("");
}
#define REG(name) #name,(PyCFunction)py_##name
static PyMethodDef DiceMethods[] = {
	{REG(log), METH_VARARGS, "output log to command&notice"},
	{REG(getDiceID), METH_NOARGS, "return dicemaid account"},
	{REG(getDiceDir), METH_NOARGS, "return Dice data dir"},
	{REG(getSelfData), METH_VARARGS, "return selfdata by filename"},
	{REG(getGroupAttr), METH_VARARGS | METH_KEYWORDS, NULL},
	{REG(setGroupAttr), METH_VARARGS, NULL},
	{REG(getUserAttr), METH_VARARGS | METH_KEYWORDS, NULL},
	{REG(setUserAttr), METH_VARARGS, NULL},
	{REG(getUserToday), METH_VARARGS | METH_KEYWORDS, NULL},
	{REG(setUserToday), METH_VARARGS, NULL},
	{REG(getPlayerCard), METH_VARARGS | METH_KEYWORDS, "return player card by uid and gid/name"},
	{REG(sendMsg), METH_VARARGS | METH_KEYWORDS, NULL},
	{REG(eventMsg), METH_VARARGS | METH_KEYWORDS, NULL},
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
	auto mod = PyModule_Create(&DiceMaidDef);
	if (PyType_Ready(&PyContextType) || 
		PyModule_AddObject(mod, "Context", (PyObject*)&PyContextType) < 0) {
		console.log("注册dicemaid.Context失败!", 0b1000);
		Py_DECREF(&PyContextType);
		Py_DECREF(mod);
		return NULL;
	}
	else if (PyType_Ready(&PyActor_Type) ||
		PyModule_AddObject(mod, "Actor", (PyObject*)&PyActor_Type) < 0) {
		console.log("注册dicemaid.Actor失败!", 0b1000);
		Py_DECREF(&PyActor_Type);
		Py_DECREF(mod);
		return NULL;
	}
	else if (PyType_Ready(&PySelfData_Type) || 
		PyModule_AddObject(mod, "SelfData", (PyObject*)&PySelfData_Type) < 0) {
		console.log("注册dicemaid.SelfData失败!", 0b1000);
		Py_DECREF(&PySelfData_Type);
		Py_DECREF(mod);
		return NULL;
	}
	Py_INCREF(&PyContextType);
	Py_INCREF(&PyActor_Type);
	Py_INCREF(&PySelfData_Type);
	return mod;
}
PyGlobal::PyGlobal() {
	if (std::filesystem::path dirPy{ dirExe / "bin" };
		std::filesystem::exists(dirPy / "python3.dll")
		|| std::filesystem::exists(dirPy / "python3.so")
		|| std::filesystem::exists(dirPy = dirExe / "py310")
		|| std::filesystem::exists(dirPy = dirExe / "python")) {
		Py_SetPythonHome(dirPy.wstring().c_str());
		Py_SetPath((dirPy / "python310.zip").wstring().c_str());
	}
	else if (std::filesystem::exists(dirExe / "python3.dll")) {
		Py_SetPythonHome(dirExe.wstring().c_str());
		Py_SetPath((dirExe / "python310.zip").wstring().c_str());
	}
	//Py_SetProgramName(L"DiceMaid");
	try {
		static auto import_dice = PyImport_AppendInittab(DiceModuleName, PyInit_DiceMaid);
		if (import_dice) {
			console.log("预载dicemaid模块失败!", 0b1000);
		}
		if (!Py_IsInitialized())Py_Initialize();
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
	if(Py_IsInitialized())Py_Finalize();
	console.log("Python.Finalized", 0);
}
int PyGlobal::runFile(const std::filesystem::path& p) {
	return 0;
}
bool PyGlobal::runFile(const std::string& s, const AttrObject& context) {
	FILE* fp{ fopen(s.c_str(),"r") };
	PyObject* mainModule = PyImport_ImportModule("__main__");
	PyObject* global = PyModule_GetDict(mainModule);
	auto res{ PyRun_File(fp, s.c_str(), Py_file_input, global, py_newContext(context)) };
	fclose(fp);
	if (!res) {
		console.log(getMsg("self") + "运行" + s + "异常!" + py_print_error(), 0b10);
		PyErr_Clear();
		return false;
	}
	return true;
}
int PyGlobal::runString(const std::string& s) {
	return PyRun_SimpleString(GBKtoUTF8(s).c_str());
}
bool PyGlobal::execString(const std::string& s, const AttrObject& context) {
	try {
		PyObject* mainModule = PyImport_ImportModule("__main__");
		PyObject* dict = PyModule_GetDict(mainModule);
		if (auto ret{ PyRun_String(GBKtoUTF8(s).c_str(), Py_eval_input, dict, py_newContext(context)) }; !ret) {
			console.log(getMsg("self") + "运算python语句异常!" + py_print_error(), 0b10);
			PyErr_Clear();
		}
		return true;
	}
	catch (std::exception& e) {
		console.log(getMsg("self") + "运行python脚本异常!" + e.what(), 0b10);
	}
	return false;
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
bool PyGlobal::call_reply(DiceEvent* msg, const AttrVar& action) {
	try {
		PyObject* mainModule = PyImport_ImportModule("__main__");
		PyObject* dict = PyModule_GetDict(mainModule);
		PyObject* locals = PyDict_New();
		PyDict_SetItem(locals, PyUnicode_FromString("msg"), py_newContext(*msg));
		string pyScript{ action };
		if (fmt->has_py(pyScript)) {
			FILE* fp{ fopen(fmt->py_path(pyScript).c_str(),"r") };
			if (auto res{ PyRun_File(fp, fmt->py_path(pyScript).c_str(), Py_file_input, dict, locals) }; !res) {
				console.log(getMsg("self") + "运行" + pyScript + "异常!" + py_print_error(), 0b10);
				PyErr_Clear();
				fclose(fp);
				msg->set("lang", "Python");
				msg->reply(getMsg("strScriptRunErr"));
				return false;
			}
			fclose(fp);
		}
		else {
			if (auto res{ PyRun_String(GBKtoUTF8(pyScript).c_str(), Py_file_input, dict, locals) };!res) {
				console.log(getMsg("self") + "运行python语句异常!" + py_print_error(), 0b10);
				PyErr_Clear();
				msg->set("lang", "Python");
				msg->reply(getMsg("strScriptRunErr"));
				return false;
			}
		}
		return true;
	}
	catch (std::exception& e) {
		console.log(getMsg("self") + "运行python脚本异常!" + e.what(), 0b10);
		msg->set("lang", "Python");
		msg->reply(getMsg("strScriptRunErr"));
		return false;
	}
}
bool py_call_event(AttrObject eve, const AttrVar& action) {
	if (Enabled && py) {
		string script{ action.to_str() };
		bool isFile{ action.is_character() && fmt->has_py(script) };
		return isFile ? py->runFile(fmt->py_path(script), eve) : py->execString(script, eve);
	}
	return false;
}
#endif // #ifdef DICE_PYTHON