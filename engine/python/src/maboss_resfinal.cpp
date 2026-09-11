/*
#############################################################################
#                                                                           #
# BSD 3-Clause License (see https://opensource.org/licenses/BSD-3-Clause)   #
#                                                                           #
# Copyright (c) 2011-2020 Institut Curie, 26 rue d'Ulm, Paris, France       #
# All rights reserved.                                                      #
#                                                                           #
# Redistribution and use in source and binary forms, with or without        #
# modification, are permitted provided that the following conditions are    #
# met:                                                                      #
#                                                                           #
# 1. Redistributions of source code must retain the above copyright notice, #
# this list of conditions and the following disclaimer.                     #
#                                                                           #
# 2. Redistributions in binary form must reproduce the above copyright      #
# notice, this list of conditions and the following disclaimer in the       #
# documentation and/or other materials provided with the distribution.      #
#                                                                           #
# 3. Neither the name of the copyright holder nor the names of its          #
# contributors may be used to endorse or promote products derived from this #
# software without specific prior written permission.                       #
#                                                                           #
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       #
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED #
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A           #
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER #
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,  #
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,       #
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR        #
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    #
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      #
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        #
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              #
#                                                                           #
#############################################################################

   Module:
     maboss_resfinal.cpp

   Authors:
     Vincent Noël <vincent.noel@curie.fr>
 
   Date:
     January-March 2020
*/

#include "maboss_resfinal.h"
#include "maboss_res.h"

#include <fstream>

#ifdef __GLIBC__
#include <malloc.h>
#endif

// network/runconfig/engine are C++ pointers, not PyObject*, so they must never
// be exposed through T_OBJECT_EX: Python would incref a C++ object header
PyMemberDef cMaBoSSResultFinal_members[] = {
    {(char*)"network", T_OBJECT, offsetof(cMaBoSSResultFinalObject, py_network), READONLY, (char*)"network"},
    {(char*)"start_time", T_LONG, offsetof(cMaBoSSResultFinalObject, start_time), READONLY, (char*)"start_time"},
    {(char*)"end_time", T_LONG, offsetof(cMaBoSSResultFinalObject, end_time), READONLY, (char*)"end_time"},
    // T_OBJECT yields None while the cache is still empty
    {(char*)"last_probtraj", T_OBJECT, offsetof(cMaBoSSResultFinalObject, last_probtraj), READONLY, (char*)"last_probtraj"},
    {NULL}  /* Sentinel */
};

PyMethodDef cMaBoSSResultFinal_methods[] = {
    {"get_final_time", (PyCFunction) cMaBoSSResultFinal_get_final_time, METH_NOARGS, "gets the final time of the simulation"},
    {"get_last_probtraj", (PyCFunction) cMaBoSSResultFinal_get_last_probtraj, METH_NOARGS, "gets the last probtraj of the simulation"},
    {"display_final_states", (PyCFunction) cMaBoSSResultFinal_display_final_states, METH_VARARGS, "display the final state"},
    {"get_last_nodes_probtraj", (PyCFunction) cMaBoSSResultFinal_get_last_nodes_probtraj, METH_VARARGS, "gets the last nodes probtraj of the simulation"},
    {"display_run", (PyCFunction) cMaBoSSResultFinal_display_run, METH_VARARGS, "prints the run of the simulation to a file"},
    {NULL}  /* Sentinel */
};

PyTypeObject cMaBoSSResultFinal = {
  PyVarObject_HEAD_INIT(NULL, 0)
  build_type_name("cMaBoSSResultFinalObject"),               /* tp_name */
  sizeof(cMaBoSSResultFinalObject),         /* tp_basicsize */
    0,                              /* tp_itemsize */
  (destructor) cMaBoSSResultFinal_dealloc,      /* tp_dealloc */
    0,                              /* tp_vectorcall_offset */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_as_async */
    0,                              /* tp_repr */
    0,                              /* tp_as_number */
    0,                              /* tp_as_sequence */
    0,                              /* tp_as_mapping */
    0,                              /* tp_hash */
    0,                              /* tp_call */
    0,                              /* tp_str */
    0,                              /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,         /* tp_flags */
  "cMaBoSS Result final object",                   /* tp_doc */
  (traverseproc) cMaBoSSResultFinal_traverse,  /* tp_traverse */
  (inquiry) cMaBoSSResultFinal_clear,          /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    0,                              /* tp_iter */
    0,                              /* tp_iternext */
  cMaBoSSResultFinal_methods,                              /* tp_methods */
  cMaBoSSResultFinal_members,                              /* tp_members */
    0,                              /* tp_getset */
    0,                              /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    0,                              /* tp_init */
    0,                              /* tp_alloc */
  cMaBoSSResultFinal_new,                      /* tp_new */   
};

int cMaBoSSResultFinal_traverse(cMaBoSSResultFinalObject *self, visitproc visit, void *arg)
{
  Py_VISIT(self->py_network);
  Py_VISIT(self->py_config);
  Py_VISIT(self->last_probtraj);
  return 0;
}

int cMaBoSSResultFinal_clear(cMaBoSSResultFinalObject *self)
{
  Py_CLEAR(self->last_probtraj);
  Py_CLEAR(self->py_network);
  Py_CLEAR(self->py_config);
  self->network = NULL;
  self->runconfig = NULL;
  return 0;
}

void cMaBoSSResultFinal_dealloc(cMaBoSSResultFinalObject *self)
{
  PyObject_GC_UnTrack(self);
  cMaBoSSResultFinal_clear(self);
  delete self->engine;
  self->engine = NULL;

#ifdef __GLIBC__
  malloc_trim(0);
#endif

  Py_TYPE(self)->tp_free((PyObject *) self);
}

PyObject * cMaBoSSResultFinal_new(PyTypeObject* type, PyObject *args, PyObject* kwargs)
{
  // tp_alloc zeroes the struct, so the cache starts out NULL ("not computed")
  return (PyObject*) type->tp_alloc(type, 0);
}

PyObject* cMaBoSSResultFinal_get_last_probtraj(cMaBoSSResultFinalObject* self)
{
  if (self->last_probtraj == NULL) {
    self->last_probtraj = self->engine->getNumpyLastStatesDists();
    if (self->last_probtraj == NULL) {
      return NULL;
    }
  }
  Py_INCREF(self->last_probtraj);

  return self->last_probtraj;
}

PyObject* cMaBoSSResultFinal_get_last_nodes_probtraj(cMaBoSSResultFinalObject* self, PyObject* args) {
  
  std::vector<Node*> list_nodes;
  PyObject* pList = Py_None;
  
  if (!PyArg_ParseTuple(args, "|O", &pList)) {
    PyErr_SetString(PyExc_TypeError, "Error parsing arguments");
    return NULL;
  }
  
  if (!cMaBoSSResult_parse_node_list(self->network, pList, list_nodes)) {
    return NULL;
  }

  return self->engine->getNumpyLastNodesDists(list_nodes);
}

PyObject* cMaBoSSResultFinal_display_final_states(cMaBoSSResultFinalObject* self, PyObject* args) {

  char * filename = NULL;
  int hexfloat = 0;
  if (!PyArg_ParseTuple(args, "s|i", &filename, &hexfloat))
    return NULL;

  std::ostream* output_final = new std::ofstream(filename);
  CSVFinalStateDisplayer * final_displayer = new CSVFinalStateDisplayer(
    self->network, *output_final, (bool) hexfloat
  );

  self->engine->displayFinal(final_displayer);

  ((std::ofstream*) output_final)->close();
  delete final_displayer;
  delete output_final;

  Py_RETURN_NONE;
}

PyObject* cMaBoSSResultFinal_get_final_time(cMaBoSSResultFinalObject* self) {
  return PyFloat_FromDouble(self->engine->getFinalTime());
}


PyObject* cMaBoSSResultFinal_display_run(cMaBoSSResultFinalObject* self, PyObject* args) 
{
  char * filename = NULL;
  int hexfloat = 0;
  if (!PyArg_ParseTuple(args, "s|i", &filename, &hexfloat))
    return NULL;
    
  std::ostream* output_run = new std::ofstream(filename);
  self->engine->displayRunStats(*output_run, self->start_time, self->end_time);
  ((std::ofstream*) output_run)->close();
  delete output_run;

  Py_RETURN_NONE;
}
