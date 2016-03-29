/***************************************************************************/
/*                                                                         */
/* play.c - mruby testing                                                  */
/* Copyright (C) 2015 Paolo Bosetti and Matteo Ragni,                      */
/* paolo[dot]bosetti[at]unitn.it and matteo[dot]ragni[at]unitn.it          */
/* Department of Industrial Engineering, University of Trento              */
/*                                                                         */
/* This library is free software.  You can redistribute it and/or          */
/* modify it under the terms of the GNU GENERAL PUBLIC LICENSE 2.0.        */
/*                                                                         */
/* This library is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* Artistic License 2.0 for more details.                                  */
/*                                                                         */
/* See the file LICENSE                                                    */
/*                                                                         */
/***************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include "mruby.h"
#include "mruby/variable.h"
#include "mruby/string.h"
#include "mruby/data.h"
#include "mruby/class.h"
#include "mruby/value.h"
#include "mruby/array.h"
#include "mruby/hash.h"
#include "mruby/numeric.h"
#include "mruby/compile.h"

#define MRB_IPC_ERROR (mrb_class_get(mrb, "IPCError"))
#define MRB_IPC_PIPE_ERROR (mrb_class_get(mrb, "IPCPipeError"))
#define DEFAULT_BUFSIZE 1024
#define CHECK   printf(">>> check %d\n", __LINE__)

typedef struct {
  int readpipe[2];
  int writepipe[2];
  int *write_p, *read_p;
  pid_t pid;
} ipc_context;

static void ipc_free(mrb_state *mrb, void *p) {
  if (p != NULL) {
    ipc_context *ipc = (ipc_context *)p;
    if (ipc->readpipe[0]) close(ipc->readpipe[0]);
    if (ipc->readpipe[1]) close(ipc->readpipe[1]);
    if (ipc->writepipe[0]) close(ipc->writepipe[0]);
    if (ipc->writepipe[1]) close(ipc->writepipe[1]);
    mrb_free(mrb, p);
  }
}

static struct mrb_data_type mrb_ipc_ctx_type = {"IPCContext", ipc_free};

static mrb_value mrb_ipc_init(mrb_state *mrb, mrb_value self) {
  ipc_context *ipc;
  ipc = mrb_calloc(mrb, 1, sizeof(ipc));
  
  if (pipe(ipc->writepipe) == -1) {
    mrb_value err_desc = mrb_str_new_cstr(mrb, strerror(errno));
    mrb_raisef(mrb, MRB_IPC_ERROR, "Error creating write pipe: %S", err_desc);
  }
  
  if (pipe(ipc->readpipe) == -1) {
    mrb_value err_desc = mrb_str_new_cstr(mrb, strerror(errno));
    mrb_raisef(mrb, MRB_IPC_ERROR, "Error creating read pipe: %S", err_desc);
  }

  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@forked"), mrb_false_value());
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@role"), mrb_str_new_cstr(mrb, "none"));
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@separator"), mrb_str_new_cstr(mrb, "\n"));
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@bufsize"), mrb_fixnum_value(DEFAULT_BUFSIZE));
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@last_message"), mrb_nil_value());
  mrb_data_init(self, ipc, &mrb_ipc_ctx_type);
  return self;
}

static mrb_value mrb_ipc_fork(mrb_state *mrb, mrb_value self) {
  ipc_context *ipc;
  mrb_value forked = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@forked"));
  if (mrb_test(forked)) {
    mrb_raise(mrb, MRB_IPC_ERROR, "Already forked!");
  }
  ipc = DATA_GET_PTR(mrb, self, &mrb_ipc_ctx_type, ipc_context);
  
  ipc->pid = fork();
  if (ipc->pid < 0) {
    mrb_value err_desc = mrb_str_new_cstr(mrb, strerror(errno));
    mrb_raisef(mrb, MRB_IPC_ERROR, "Can't fork: %S", err_desc);
  }
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@forked"), mrb_true_value());
  return mrb_fixnum_value(ipc->pid);
}

mrb_value mrb_ipc_is_forked(mrb_state *mrb, mrb_value self) {
  return mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@forked"));
}


// read end is parent[0], write end is parent[1]
static mrb_value mrb_ipc_as_child(mrb_state *mrb, mrb_value self) {
  ipc_context *ipc;
  mrb_value forked = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@forked"));
  mrb_value role = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@role"));
  if (! mrb_test(forked)) {
    mrb_raise(mrb, MRB_IPC_ERROR, "Not yet forked!");
  }
  if (mrb_eql(mrb, role, mrb_str_new_cstr(mrb, "child"))) {
    mrb_raise(mrb, MRB_IPC_ERROR, "I am child already!");
  }
  ipc = DATA_GET_PTR(mrb, self, &mrb_ipc_ctx_type, ipc_context);
  close(ipc->readpipe[1]);
  ipc->readpipe[1] = 0;
  close(ipc->writepipe[0]);
  ipc->writepipe[0] = 0;
  ipc->write_p = &ipc->writepipe[1];
  ipc->read_p = &ipc->readpipe[0];
  fcntl(*ipc->read_p, F_SETFL, O_NONBLOCK);
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@role"), mrb_str_new_cstr(mrb, "child"));
  return mrb_nil_value();
}

static mrb_value mrb_ipc_as_parent(mrb_state *mrb, mrb_value self) {
  ipc_context *ipc;
  mrb_value forked = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@forked"));
  mrb_value role = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@role"));
  if (! mrb_test(forked)) {
    mrb_raise(mrb, MRB_IPC_ERROR, "Not yet forked!");
  }
  if (mrb_eql(mrb, role, mrb_str_new_cstr(mrb, "parent"))) {
    mrb_raise(mrb, MRB_IPC_ERROR, "I am parent already!");
  }
  ipc = DATA_GET_PTR(mrb, self, &mrb_ipc_ctx_type, ipc_context);
  close(ipc->readpipe[0]);
  ipc->readpipe[0] = 0;
  close(ipc->writepipe[1]);
  ipc->writepipe[1] = 0;
  ipc->write_p = &ipc->readpipe[1];
  ipc->read_p = &ipc->writepipe[0];
  fcntl(*ipc->read_p, F_SETFL, O_NONBLOCK);
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@role"), mrb_str_new_cstr(mrb, "parent"));
  return mrb_nil_value();
}

static mrb_value mrb_ipc_fdes(mrb_state *mrb, mrb_value self) {
  ipc_context *ipc;
  mrb_value result;
  ipc = DATA_GET_PTR(mrb, self, &mrb_ipc_ctx_type, ipc_context);
  result = mrb_ary_new(mrb);
  mrb_ary_push(mrb, result, mrb_fixnum_value(ipc->writepipe[0]));
  mrb_ary_push(mrb, result, mrb_fixnum_value(ipc->writepipe[1]));
  mrb_ary_push(mrb, result, mrb_fixnum_value(ipc->readpipe[0]));
  mrb_ary_push(mrb, result, mrb_fixnum_value(ipc->readpipe[1]));
  return result;
}

static mrb_value mrb_ipc_readpipe(mrb_state *mrb, mrb_value self) {
  ipc_context *ipc;
  ipc = DATA_GET_PTR(mrb, self, &mrb_ipc_ctx_type, ipc_context);
  return mrb_fixnum_value(*ipc->read_p);
}

static mrb_value mrb_ipc_writepipe(mrb_state *mrb, mrb_value self) {
  ipc_context *ipc;
  ipc = DATA_GET_PTR(mrb, self, &mrb_ipc_ctx_type, ipc_context);
  return mrb_fixnum_value(*ipc->write_p);
}

static mrb_value mrb_ipc_pid(mrb_state *mrb, mrb_value self) {
  ipc_context *ipc;
  ipc = DATA_GET_PTR(mrb, self, &mrb_ipc_ctx_type, ipc_context);
  return mrb_fixnum_value(ipc->pid);
}

static mrb_value mrb_ipc_send(mrb_state *mrb, mrb_value self) {
  ipc_context *ipc;
  char *data = NULL;
  mrb_int len;
  mrb_get_args(mrb, "s", &data, &len);
  ipc = DATA_GET_PTR(mrb, self, &mrb_ipc_ctx_type, ipc_context);
  while (1) {
    if (write(*ipc->write_p, data, len) >= 0) {
      break;
    }
    else {
      mrb_value err_desc = mrb_str_new_cstr(mrb, strerror(errno));
      mrb_raisef(mrb, MRB_IPC_PIPE_ERROR, "Error writing to pipe: %S", err_desc);
    }
  }
  return mrb_fixnum_value(len);
}

static mrb_value mrb_ipc_receive(mrb_state *mrb, mrb_value self) {
  ipc_context *ipc;
  mrb_int bufsize, res;
  mrb_value result;
  char *data;
  
  if (mrb_get_args(mrb, "|i", &bufsize) != 1) {
    bufsize = mrb_int(mrb, mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@bufsize")));
  }

  data   = calloc(bufsize, sizeof(char));
  result = mrb_str_buf_new(mrb, bufsize);
  ipc    = DATA_GET_PTR(mrb, self, &mrb_ipc_ctx_type, ipc_context);

  if ((res = read(*ipc->read_p, data, bufsize)) > 0) {
    result = mrb_str_cat(mrb, result, data, bufsize);
  }
  else if (errno == EAGAIN) {
    result = mrb_nil_value();
  }
  else {
    mrb_value err_desc = mrb_str_new_cstr(mrb, strerror(errno));
    mrb_raisef(mrb, MRB_IPC_PIPE_ERROR, "Error reading from pipe: %S", err_desc);
  }
  free(data);
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@last_message"), result);
  return result;
}

static mrb_value mrb_ipc_close(mrb_state *mrb, mrb_value self) {
  ipc_context *ipc;
  ipc = DATA_GET_PTR(mrb, self, &mrb_ipc_ctx_type, ipc_context);
  close(*ipc->read_p);
  close(*ipc->write_p);
  return mrb_nil_value();
}

void mrb_mruby_ipc_gem_init(mrb_state *mrb) {
  struct RClass *ipc;

  ipc = mrb_define_class(mrb, "IPC", mrb->object_class);
  MRB_SET_INSTANCE_TT(ipc, MRB_TT_DATA);
  mrb_define_method(mrb, ipc, "initialize", mrb_ipc_init, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, ipc, "as_child", mrb_ipc_as_child, MRB_ARGS_NONE());
  mrb_define_method(mrb, ipc, "as_parent", mrb_ipc_as_parent, MRB_ARGS_NONE());
  mrb_define_method(mrb, ipc, "fork", mrb_ipc_fork, MRB_ARGS_NONE());
  mrb_define_method(mrb, ipc, "forked?", mrb_ipc_is_forked, MRB_ARGS_NONE());
  mrb_define_method(mrb, ipc, "descriptors", mrb_ipc_fdes, MRB_ARGS_NONE());
  mrb_define_method(mrb, ipc, "writepipe", mrb_ipc_writepipe, MRB_ARGS_NONE());
  mrb_define_method(mrb, ipc, "readpipe", mrb_ipc_readpipe, MRB_ARGS_NONE());
  mrb_define_method(mrb, ipc, "pid", mrb_ipc_pid, MRB_ARGS_NONE());
  mrb_define_method(mrb, ipc, "send", mrb_ipc_send, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, ipc, "receive", mrb_ipc_receive, MRB_ARGS_OPT(1));
  mrb_define_method(mrb, ipc, "close", mrb_ipc_close, MRB_ARGS_NONE());
}

void mrb_mruby_ipc_gem_final(mrb_state *mrb) {}
