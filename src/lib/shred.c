#include <unistd.h>
#include "gwion_util.h"
#include "gwion_ast.h"
#include "gwion_env.h"
#include "gwion_thread.h"
#include "vm.h"
#include "instr.h"
#include "shreduler_private.h"
#include "gwion.h"
#include "object.h"
#include "operator.h"
#include "import.h"
#include "emit.h"
#include "specialid.h"
#include "gwi.h"

static m_int o_fork_thread, o_shred_cancel,
    o_fork_done, o_fork_ev;

#define FORK_THREAD(o)  *(THREAD_TYPE *)(o->data + o_fork_thread)

VM_Shred new_shred_base(const VM_Shred shred, const VM_Code code) {
  const VM_Shred sh = new_vm_shred(shred->info->mp, code);
  vmcode_addref(code);
  sh->base = shred->base;
  return sh;
}

M_Object new_shred(const VM_Shred shred) {
  const M_Object obj =
      new_object(shred->info->mp, shred->info->vm->gwion->type[et_shred]);
  ME(obj) = shred;
  return obj;
}

ANN static inline M_Object fork_object(const VM_Shred shred, const Type t) {
  const Gwion    gwion = shred->info->vm->gwion;
  const M_Object o     = new_object(gwion->mp, t);
  *(M_Object *)(o->data + o_fork_ev) =
      new_object(gwion->mp, gwion->type[et_event]);
  vector_init(&EV_SHREDS(*(M_Object *)(o->data + o_fork_ev)));
  return o;
}

ANN M_Object new_fork(const VM_Shred shred, const VM_Code code, const Type t) {
  VM *           parent = shred->info->vm;
  const VM_Shred sh     = new_shred_base(shred, code);
  VM *           vm     = (sh->info->vm = gwion_cpy(parent));
  vm->parent            = parent;
  const M_Object o = sh->info->me = fork_object(shred, t);
  ME(o)                           = sh;
//  ++o->ref;
  shreduler_add(vm->shreduler, sh);
  return o;
}

static MFUN(gw_shred_exit) {
  const VM_Shred s = ME(o);
  if((m_int)s->tick->prev != -1)
   shreduler_remove(s->tick->shreduler, s, true);
}

static MFUN(vm_shred_id) {
  const VM_Shred s = ME(o);
  *(m_int *)RETURN = s->tick->xid;
}

static MFUN(vm_shred_is_running) {
  const VM_Shred s  = ME(o);
  *(m_uint *)RETURN = ((m_int)s->tick->prev != -1 && (s->tick->prev || s->tick->next)) ? true : false;
}

static MFUN(vm_shred_is_done) { *(m_uint *)RETURN = !ME(o) ? true : false; }

static MFUN(shred_yield) {
  const VM_Shred  s  = ME(o);
  if((m_int)s->tick->prev == -1) return;
  const Shreduler sh = s->tick->shreduler;
  if (s != shred) shreduler_remove(sh, s, false);
  shredule(sh, s, GWION_EPSILON);
}

static SFUN(vm_shred_from_id) {
  const m_int index = *(m_int *)MEM(0);
  if (index > 0) {
    const Vector v = &shred->tick->shreduler->active_shreds;
    for (m_uint i = 0; i < vector_size(v); ++i) {
      const VM_Shred s =
          (VM_Shred)vector_at(v, i);
      if (s->tick->xid == (m_uint)index) {
        *(M_Object *)RETURN = s->info->me;
        return;
      }
    }
  }
  handle(shred, "InvalidShredRequest");
}

static MFUN(shred_args) {
  const VM_Shred s  = ME(o);
  *(m_uint *)RETURN = s->info->args.ptr ? vector_size(&s->info->args) : 0;
}

static MFUN(shred_arg) {
  const VM_Shred s   = ME(o);
  const m_int    idx = *(m_int *)MEM(SZ_INT);
  if (s->info->args.ptr && idx >= 0 && (m_uint)idx < vector_size(&s->info->args)) {
    const m_str str = (m_str)vector_at(&s->info->args, *(m_uint *)MEM(SZ_INT));
    *(M_Object *)RETURN = new_string(shred->info->vm->gwion, str);
  } else
    handle(shred, "InvalidShredArgumentRequest");
}

#ifndef BUILD_ON_WINDOWS
#define PATH_CHR '/'
#else
#define PATH_CHR '\\'
#endif

#define describe_name(name, src)                                               \
  static MFUN(shred##name##_name) {                                            \
    const VM_Shred s   = ME(o);                                                \
    const m_str    str = code_name((src), 0);                                  \
    *(m_uint *)RETURN  = (m_uint)new_string(shred->info->vm->gwion, str);      \
  }
describe_name(, s->info->orig->name) describe_name(_code, s->code->name)

#define describe_path_and_dir(name, src)                                       \
  static MFUN(shred##name##_path) {                                            \
    const VM_Shred s   = ME(o);                                                \
    const m_str    str = code_name((src), 1);                                  \
    *(m_uint *)RETURN  = (m_uint)new_string(shred->info->vm->gwion, str);      \
  }                                                                            \
  static MFUN(shred##name##_dir) {                                             \
    const VM_Shred s   = ME(o);                                                \
    const m_str    str = code_name((src), 1);                                  \
    const size_t   len = strlen(str);                                          \
    char           c[len + 1];                                                 \
    strcpy(c, str);                                                            \
    size_t sz = len;                                                           \
    while (sz) {                                                               \
      if (c[sz] == PATH_CHR) {                                                 \
        c[sz] = 0;                                                             \
        break;                                                                 \
      }                                                                        \
      --sz;                                                                    \
    }                                                                          \
    *(m_uint *)RETURN = (m_uint)new_string(shred->info->vm->gwion, c);         \
  }
    describe_path_and_dir(, s->info->orig->name)
        describe_path_and_dir(_code, s->code->name)

static DTOR(shred_dtor) {
  VM_Shred s = ME(o);
  free_vm_shred(s);
}

static MFUN(shred_lock) { if(ME(o)->tick) MUTEX_LOCK(ME(o)->mutex); }

static MFUN(shred_unlock) { if(ME(o)->tick) MUTEX_UNLOCK(ME(o)->mutex); }

static void stop(const M_Object o) {
  VM *vm = ME(o)->info->vm;
  MUTEX_LOCK(vm->shreduler->mutex);
  MUTEX_LOCK(ME(o)->mutex);
  vm->shreduler->bbq->is_running       = 0;
  *(m_int *)(o->data + o_shred_cancel) = 1;
  MUTEX_UNLOCK(ME(o)->mutex);
  MUTEX_UNLOCK(vm->shreduler->mutex);
}

static inline void join(const M_Object o) {
  if (FORK_THREAD(o)) {
    THREAD_JOIN(FORK_THREAD(o));
    FORK_THREAD(o) = 0;
  }
}

static DTOR(fork_dtor) {
  VM *parent = ME(o)->info->vm->parent;
//  MUTEX_LOCK(parent->shreduler->mutex);
  MUTEX_LOCK(ME(o)->mutex);
  *(m_int *)(o->data + o_fork_done) = 1;
  MUTEX_UNLOCK(ME(o)->mutex);
  stop(o);
  join(o);
//  MUTEX_UNLOCK(parent->shreduler->mutex);
  MUTEX_LOCK(parent->shreduler->mutex);
//  MUTEX_LOCK(ME(o)->mutex);
  if (parent->gwion->data->child.ptr) {
    const m_int idx = vector_find(&parent->gwion->data->child, (vtype)o);
    if (idx > -1) VPTR(&parent->gwion->data->child, idx) = 0;
  }
  if (!parent->gwion->data->child2.ptr)
    vector_init(&parent->gwion->data->child2);
  vector_add(&parent->gwion->data->child2, (vtype)ME(o)->info->vm->gwion);
//  MUTEX_UNLOCK(ME(o)->mutex);
  MUTEX_UNLOCK(parent->shreduler->mutex);
  vmcode_remref(ME(o)->code, ME(o)->info->vm->gwion);
}

static MFUN(fork_join) {
  if (*(m_int *)(o->data + o_fork_done)) return;
  if (!ME(o)->tick) return;
  shreduler_remove(shred->tick->shreduler, shred, false);
  vector_add(&EV_SHREDS(*(M_Object *)(o->data + o_fork_ev)), (vtype)shred);
}

static MFUN(shred_cancel) {
  if(!ME(o)->tick)return;
//  vm_lock(ME(o)->info->vm);
  MUTEX_LOCK(ME(o)->mutex);
  *(m_int *)(o->data + o_shred_cancel) = *(m_int *)MEM(SZ_INT);
  MUTEX_UNLOCK(ME(o)->mutex);
//  vm_unlock(ME(o)->info->vm);
}

static MFUN(shred_test_cancel) {
  MUTEX_LOCK(ME(o)->mutex);
  if (*(m_int *)(o->data + o_shred_cancel)) {
    MUTEX_UNLOCK(ME(o)->mutex);
    vm_shred_exit(ME(o));
  } else
  MUTEX_UNLOCK(ME(o)->mutex);
}

static MFUN(fork_test_cancel) {
  VM *parent = ME(o)->info->vm;
  MUTEX_LOCK(parent->shreduler->mutex);
  if (*(m_int *)(o->data + o_shred_cancel)) {
    MUTEX_UNLOCK(parent->shreduler->mutex);
    stop(o);
    join(o);
    _release(o, ME(o));
    vm_shred_exit(ME(o));
  } else
    MUTEX_UNLOCK(parent->shreduler->mutex);
}

static MFUN(shred_now) {
  VM *vm = ME(o)->info->vm;
  while (vm->parent) vm = vm->parent;
  MUTEX_LOCK(vm->shreduler->mutex);
  *(m_float *)RETURN = vm->bbq->pos;
  MUTEX_UNLOCK(vm->shreduler->mutex);
}

struct ThreadLauncher {
  MUTEX_TYPE       mutex;
  THREAD_COND_TYPE cond;
  VM *             vm;
};

static inline int fork_running(VM *vm, const M_Object o) {
  MUTEX_LOCK(ME(o)->mutex);
  const int cancel = *(m_int *)(o->data + o_shred_cancel);
  MUTEX_UNLOCK(ME(o)->mutex);
  if(cancel)return false;
  MUTEX_LOCK(vm->shreduler->mutex);
  const int ret = vm->bbq->is_running;
  MUTEX_UNLOCK(vm->shreduler->mutex);
  return ret;
}

static ANN THREAD_FUNC(fork_run) {
  struct ThreadLauncher *tl    = data;
  VM *                   vm    = tl->vm;
  MUTEX_TYPE             mutex = tl->mutex;
  const M_Object         me    = vm->shreduler->list->self->info->me;
  MUTEX_COND_LOCK(mutex);
  THREAD_COND_SIGNAL(tl->cond);
  MUTEX_COND_UNLOCK(mutex);
// THREAD_COND_CLEANUP(tl->cond);
//  MUTEX_CLEANUP(tl->mutex);
  while (fork_running(vm, me)) {
    vm_run(vm);
    ++vm->bbq->pos;
  }
  gwion_end_child(ME(me), vm->gwion);
vm_lock(vm);
//  MUTEX_LOCK(vm->shreduler->mutex);
//  MUTEX_LOCK(vm->parent->shreduler->mutex);
  MUTEX_LOCK(ME(me)->mutex);
  if (!*(m_int *)(me->data + o_shred_cancel) &&
      me->type_ref != vm->gwion->type[et_fork])
    memcpy(me->data + vm->gwion->type[et_fork]->nspc->offset, ME(me)->reg,
  ((Type)vector_front(&me->type_ref->info->tuple->types))->size);
  *(m_int *)(me->data + o_fork_done) = 1;
  MUTEX_UNLOCK(ME(me)->mutex);
  if (!*(m_int *)(me->data + o_shred_cancel))
    broadcast(*(M_Object *)(me->data + o_fork_ev));
vm_unlock(vm);
//  MUTEX_UNLOCK(vm->parent->shreduler->mutex);
//  MUTEX_UNLOCK(vm->shreduler->mutex);
  THREAD_RETURN(0);
}

ANN void fork_launch(const M_Object o) {
  MUTEX_TYPE mutex;
  MUTEX_SETUP(mutex);
  THREAD_COND_TYPE cond;
  THREAD_COND_SETUP(cond);
  struct ThreadLauncher tl = {
      .mutex = mutex, .cond = cond, .vm = ME(o)->info->vm};
  ++o->ref;
  MUTEX_COND_LOCK(mutex);
  THREAD_CREATE(FORK_THREAD(o), fork_run, &tl);
  THREAD_COND_WAIT(cond, mutex);
  MUTEX_COND_UNLOCK(mutex);
  THREAD_COND_CLEANUP(cond);
  MUTEX_CLEANUP(mutex);
}

ANN void fork_clean(const VM_Shred shred, const Vector v) {
  for (m_uint i = 0; i < vector_size(v); ++i) {
    const M_Object o = (M_Object)vector_at(v, i);
    if (!o) continue;
    stop(o);
  }
  for (m_uint i = 0; i < vector_size(v); ++i) {
    const M_Object o = (M_Object)vector_at(v, i);
    if (!o) continue;
    join(o);
  }
  for (m_uint i = 0; i < vector_size(v); ++i) {
    const M_Object o = (M_Object)vector_at(v, i);
    if (!o) continue;
    _release(o, shred);
  }
  vector_release(v);
  v->ptr = NULL;
}

GWION_IMPORT(shred) {
  const Type t_shred = gwi_class_ini(gwi, "Shred", NULL);
  gwi_class_xtor(gwi, NULL, shred_dtor);

  t_shred->nspc->offset += SZ_INT;

  gwi_item_ini(gwi, "int", "cancel");
  GWI_BB((o_shred_cancel = gwi_item_end(gwi, ae_flag_const, num, 0)))

  gwi_func_ini(gwi, "void", "exit");
  GWI_BB(gwi_func_end(gwi, gw_shred_exit, ae_flag_none))

  gwi_func_ini(gwi, "bool", "running");
  GWI_BB(gwi_func_end(gwi, vm_shred_is_running, ae_flag_none))

  gwi_func_ini(gwi, "bool", "done");
  GWI_BB(gwi_func_end(gwi, vm_shred_is_done, ae_flag_none))

  gwi_func_ini(gwi, "int", "id");
  GWI_BB(gwi_func_end(gwi, vm_shred_id, ae_flag_none))

  gwi_func_ini(gwi, "Shred", "fromId");
  gwi_func_arg(gwi, "int", "xid");
  GWI_BB(gwi_func_end(gwi, vm_shred_from_id, ae_flag_static))

  gwi_func_ini(gwi, "void", "yield");
  GWI_BB(gwi_func_end(gwi, shred_yield, ae_flag_none))

  gwi_func_ini(gwi, "int", "args");
  GWI_BB(gwi_func_end(gwi, shred_args, ae_flag_none))

  gwi_func_ini(gwi, "string", "arg");
  gwi_func_arg(gwi, "int", "n");
  GWI_BB(gwi_func_end(gwi, shred_arg, ae_flag_none))

  gwi_func_ini(gwi, "string", "name");
  GWI_BB(gwi_func_end(gwi, shred_name, ae_flag_none))

  gwi_func_ini(gwi, "string", "path");
  GWI_BB(gwi_func_end(gwi, shred_path, ae_flag_none))

  gwi_func_ini(gwi, "string", "dir");
  GWI_BB(gwi_func_end(gwi, shred_dir, ae_flag_none))

  gwi_func_ini(gwi, "string", "code_name");
  GWI_BB(gwi_func_end(gwi, shred_code_name, ae_flag_none))

  gwi_func_ini(gwi, "string", "code_path");
  GWI_BB(gwi_func_end(gwi, shred_code_path, ae_flag_none))

  gwi_func_ini(gwi, "string", "code_dir");
  GWI_BB(gwi_func_end(gwi, shred_code_dir, ae_flag_none))

  gwi_func_ini(gwi, "void", "set_cancel");
  gwi_func_arg(gwi, "int", "n");
  GWI_BB(gwi_func_end(gwi, shred_cancel, ae_flag_none))
  gwi_func_ini(gwi, "void", "test_cancel");
  GWI_BB(gwi_func_end(gwi, shred_test_cancel, ae_flag_none))
  gwi_func_ini(gwi, "void", "lock");
  GWI_BB(gwi_func_end(gwi, shred_lock, ae_flag_none))
  gwi_func_ini(gwi, "void", "unlock");
  GWI_BB(gwi_func_end(gwi, shred_unlock, ae_flag_none))
  gwi_func_ini(gwi, "float", "get_now");
  GWI_BB(gwi_func_end(gwi, shred_now, ae_flag_none))
  GWI_BB(gwi_class_end(gwi))
  SET_FLAG(t_shred, abstract | ae_flag_final);
  gwi->gwion->type[et_shred] = t_shred;

  struct SpecialId_ spid = {.type = t_shred, .exec = RegPushMe, .is_const = 1};
  gwi_specialid(gwi, "me", &spid);

  const Type t_fork = gwi_class_ini(gwi, "Fork", "Shred");
  gwi_class_xtor(gwi, NULL, fork_dtor);
  gwi->gwion->type[et_fork] = t_fork;
  o_fork_thread = t_fork->nspc->offset;
  t_fork->nspc->offset += SZ_INT;

  gwi_item_ini(gwi, "int", "is_done");
  GWI_BB((o_fork_done = gwi_item_end(gwi, ae_flag_const, num, 0)))
  gwi_item_ini(gwi, "Event", "ev");
  GWI_BB((o_fork_ev = gwi_item_end(gwi, ae_flag_const, num, 0)))
  gwi_func_ini(gwi, "void", "join");
  GWI_BB(gwi_func_end(gwi, fork_join, ae_flag_none))
  gwi_func_ini(gwi, "void", "test_cancel");
  GWI_BB(gwi_func_end(gwi, fork_test_cancel, ae_flag_none))
  GWI_BB(gwi_class_end(gwi))
  SET_FLAG(t_fork, abstract | ae_flag_final);

  const Type t_typed = gwi_class_ini(gwi, "TypedFork:[A]", "Fork");
  gwi_item_ini(gwi, "A", "retval");
  GWI_BB(gwi_item_end(gwi, ae_flag_const, num, 0))
  GWI_BB(gwi_class_end(gwi))
  SET_FLAG(t_typed, abstract | ae_flag_final);
  return GW_OK;
}
