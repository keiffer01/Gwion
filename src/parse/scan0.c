#include <string.h>
#include "gwion_util.h"
#include "gwion_ast.h"
#include "gwion_env.h"
#include "vm.h"
#include "traverse.h"
#include "template.h"
#include "parse.h"
#include "gwion.h"
#include "object.h"
#include "instr.h"
#include "operator.h"
#include "import.h"

static inline void add_type(const Env env, const Nspc nspc, const Type t) {
  nspc_add_type_front(nspc, insert_symbol(t->name), t);
}

static inline void context_global(const Env env) {
  if (env->context) env->context->global = true;
}

static inline Type scan0_type(const Env env, const m_str name, const Type t) {
  return new_type(env->gwion->mp, name, t);
}

ANN static inline m_bool scan0_defined(const Env env, const Symbol s,
                                       const loc_t pos) {
  if (nspc_lookup_type1(env->curr, s))
    ERR_B(pos, _("type '%s' already defined"), s_name(s));
  return already_defined(env, s, pos);
}

ANN static void fptr_assign(const Fptr_Def fptr) {
  const Func_Def def = fptr->type->info->func->def;
  if (GET_FLAG(fptr->base, global)) {
    SET_FLAG(fptr->value, global);
    SET_FLAG(fptr->base->func, global);
    SET_FLAG(def->base, global);
  } else if (!GET_FLAG(fptr->base, static)) {
    set_vflag(fptr->value, vflag_member);
    set_vflag(fptr->base->func->value_ref, vflag_member);
    def->stack_depth += SZ_INT;
  } else {
    SET_FLAG(fptr->value, static);
    SET_FLAG(fptr->base->func, static);
    SET_FLAG(def->base, static);
  }
  if (fbflag(def->base, fbflag_variadic)) def->stack_depth += SZ_INT;
}

static void fptr_def(const Env env, const Fptr_Def fptr) {
  const Func_Def def = new_func_def(
      env->gwion->mp, cpy_func_base(env->gwion->mp, fptr->base), NULL);
  fptr->base->func = new_func(env->gwion->mp, s_name(fptr->base->xid), def);
  fptr->value->d.func_ref     = fptr->base->func;
  fptr->base->func->value_ref = fptr->value;
  fptr->type->info->func      = fptr->base->func;
  def->base->func             = fptr->base->func;
}

ANN m_bool scan0_fptr_def(const Env env, const Fptr_Def fptr) {
  CHECK_BB(env_access(env, fptr->base->flag, fptr->base->td->pos));
  CHECK_BB(scan0_defined(env, fptr->base->xid, fptr->base->td->pos));
  const m_str name   = s_name(fptr->base->xid);
  const Type  t      = scan0_type(env, name, env->gwion->type[et_fptr]);
  const bool  global = !env->class_def && GET_FLAG(fptr->base, global);
  t->flag |= fptr->base->flag;
  fptr->type = t;
  if (global) {
    context_global(env);
    env_push_global(env);
  }
  fptr->value = mk_class(env, t, fptr->base->pos);
  if (global) env_pop(env, 0);
  valuefrom(env, fptr->value->from, fptr->base->pos);
  fptr_def(env, fptr);
  if (env->class_def) fptr_assign(fptr);
  set_vflag(fptr->value, vflag_func);
  add_type(env, t->info->value->from->owner, t);
  type_addref(t);
  return GW_OK;
}

static OP_CHECK(opck_implicit_similar) {
  const struct Implicit *imp = (struct Implicit *)data;
  return imp->e->type;
}

static OP_CHECK(opck_contract_similar) {
  const Exp_Cast *cast = (Exp_Cast *)data;
  if(tflag(exp_self(cast)->type, tflag_contract)) {
    struct Implicit imp = { .t=exp_self(cast)->type, .e=cast->exp};
    struct Op_Import opi    = {
      .op = insert_symbol("@implicit"), .lhs = cast->exp->type, .rhs = exp_self(cast)->type, .data=(m_uint)&imp };
    CHECK_NN(op_check(env, &opi));
  }
  return opck_similar_cast(env, data);
}

ANN /*static */ void scan0_implicit_similar(const Env env, const Type lhs,
                                            const Type rhs) {
  struct Op_Func   opfunc = {.ck = opck_similar_cast};
  struct Op_Import opi    = {
      .op = insert_symbol("$"), .lhs = lhs, .rhs = rhs, .func = &opfunc};
  add_op(env->gwion, &opi);
  opi.lhs = rhs;
  opi.rhs = lhs;
  add_op(env->gwion, &opi);
  opfunc.ck = opck_implicit_similar;
  opi.op    = insert_symbol("@implicit");
  add_op(env->gwion, &opi);
}

ANN static void scan0_explicit_distinct(const Env env, const Type lhs,
                                        const Type rhs) {
  struct Op_Func   opfunc = {.ck = opck_similar_cast};
  if(tflag(rhs, tflag_contract)) {
    opfunc.ck = opck_contract_similar;
    opfunc.em = opem_contract_similar;
  }
  struct Op_Import opi    = {
      .op = insert_symbol("$"), .lhs = lhs, .rhs = rhs, .func = &opfunc};
  add_op(env->gwion, &opi);
  opi.lhs = rhs;
  opi.rhs = lhs;
  add_op(env->gwion, &opi);
}

ANN static void typedef_simple(const Env env, const Type_Def tdef,
                               const Type base) {
  const Type t    = scan0_type(env, s_name(tdef->xid), base);
  t->size         = base->size;
  const Nspc nspc = (!env->class_def && GET_FLAG(tdef->ext, global))
                        ? env->global_nspc
                        : env->curr;
  if (GET_FLAG(tdef->ext, global)) context_global(env);
  add_type(env, nspc, t);
  tdef->type = t;
  if (base->nspc) // create a new nspc if `distinct`?
    nspc_addref((t->nspc = base->nspc));
  t->flag = tdef->ext->flag;
  if (tdef->ext->array && !tdef->ext->array->exp) set_tflag(t, tflag_empty);
  mk_class(env, tdef->type, tdef->pos);
}

ANN static m_bool typedef_complex(const Env env, const Type_Def tdef,
                                  const Type base) {
  const ae_flag   flag = base->info->cdef ? base->info->cdef->flag : 0;
  const Class_Def cdef = new_class_def(env->gwion->mp, flag, tdef->xid,
                                       cpy_type_decl(env->gwion->mp, tdef->ext),
                                       NULL, tdef->ext->pos);
  CHECK_BB(scan0_class_def(env, cdef));
  tdef->type      = cdef->base.type;
  cdef->base.tmpl = tdef->tmpl; // check cpy
  mk_class(env, tdef->type, tdef->pos);
  return GW_OK;
}

ANN static void typedef_fptr(const Env env, const Type_Def tdef,
                             const Type base) {
  tdef->type               = type_copy(env->gwion->mp, base);
  tdef->type->info->func   = base->info->func;
  tdef->type->name         = s_name(tdef->xid);
  tdef->type->info->parent = base;
  add_type(env, env->curr, tdef->type);
  mk_class(env, tdef->type, tdef->pos);
  if (base->info->func->def->base->tmpl) set_tflag(tdef->type, tflag_ftmpl);
}

ANN m_bool scan0_type_def(const Env env, const Type_Def tdef) {
  CHECK_BB(env_access(env, tdef->ext->flag, tdef->ext->pos));
  DECL_OB(const Type, base, = tdef->tmpl ? find_type(env, tdef->ext)
                                         : known_type(env, tdef->ext));
  CHECK_BB(scan0_defined(env, tdef->xid, tdef->ext->pos));
  const bool global =
      GET_FLAG(tdef->ext, global); // TODO: handle global in class
  if (global) {
    context_global(env);
    env_push_global(env);
  }
  if (!is_func(env->gwion, base)) {
    if (!tdef->ext->types && (!tdef->ext->array || !tdef->ext->array->exp))
      typedef_simple(env, tdef, base);
    else
      CHECK_BB(typedef_complex(env, tdef, base));
  } else
    typedef_fptr(env, tdef, base);
  if (tdef->when) set_tflag(tdef->type, tflag_contract);
  if (!tdef->distinct && !tdef->when)
    scan0_implicit_similar(env, base, tdef->type);
  if (tdef->distinct || tdef->when) {
//    tdef->type->info->parent = base->info->parent;
    if (base->info->gack)
      vmcode_addref(tdef->type->info->gack = base->info->gack);
    set_tflag(tdef->type, tflag_distinct);
    struct Op_Import opi = {.lhs = base, .rhs = tdef->type};
    op_cpy(env, &opi);
    scan0_explicit_distinct(env, base, tdef->type);
    type_addref(tdef->type); // maybe because of scope_iter in nspc_free_values
  } else
    set_tflag(tdef->type, tflag_typedef);
  if(tflag(base, tflag_ref)) {
    set_tflag(tdef->type, tflag_ref);
    set_tflag(tdef->type, tflag_infer);
  }
  if (global) env_pop(env, 0);
  tdef->type->info->base_type = base;
  return GW_OK;
}

#define scan0_nspc(env, a)                                                     \
  GET_FLAG(a, global) ? !env->class_def ? env->global_nspc : NULL : env->curr

ANN static Type enum_type(const Env env, const Enum_Def edef) {
  const Type   t    = type_copy(env->gwion->mp, env->gwion->type[et_int]);
  t->name           = s_name(edef->xid);
  t->info->parent   = env->gwion->type[et_int];
  const bool global = GET_FLAG(edef, global); // TODO: handle global in class
  if (global) {
    context_global(env);
    env_push_global(env);
  }
  add_type(env, env->curr, t);
  mk_class(env, t, edef->pos);
  if (global) env_pop(env, 0);
//  scan0_implicit_similar(env, t, env->gwion->type[et_int]);
//  scan0_implicit_similar(env, env->gwion->type[et_int], t);
  return t;
}

ANN static inline m_bool scan0_global(const Env env, const ae_flag flag,
                                      const loc_t pos) {
  if (!env->class_def || !((flag & ae_flag_global) == ae_flag_global))
    return GW_OK;
  ERR_B(pos, _("can't declare as global in class def"))
}

ANN m_bool scan0_enum_def(const Env env, const Enum_Def edef) {
  CHECK_BB(env_storage(env, edef->flag, edef->pos));
  CHECK_BB(scan0_defined(env, edef->xid, edef->pos));
  CHECK_BB(scan0_global(env, edef->flag, edef->pos));
  edef->t = enum_type(env, edef);
  vector_init(&edef->values);
  if (GET_FLAG(edef, global)) context_global(env);
  return GW_OK;
}

ANN static Type union_type(const Env env, const Symbol s, const loc_t loc) {
  const m_str name = s_name(s);
  const Type  t    = new_type(env->gwion->mp, name, env->gwion->type[et_union]);
  t->nspc          = new_nspc(env->gwion->mp, name);
  t->nspc->parent  = env->curr;
  t->info->tuple   = new_tupleform(env->gwion->mp, NULL); // ???
  set_tflag(t, tflag_union);
  add_type(env, env->curr, t);
  mk_class(env, t, loc);
  SET_FLAG(t, final);
  if (strncmp(t->name, "Option", 6)) SET_FLAG(t, abstract);
  return t;
}

ANN static void union_tmpl(const Env env, const Union_Def udef) {
  if (tmpl_base(udef->tmpl)) {
    const Union_Def u      = cpy_union_def(env->gwion->mp, udef);
    u->type                = udef->type;
    udef->type->info->udef = u;
    set_tflag(u->type, tflag_tmpl);
    set_tflag(u->type, tflag_udef);
  }
  //  if(GET_FLAG(udef, global))
  //    SET_FLAG(udef->type, global);
}

ANN m_bool scan0_union_def(const Env env, const Union_Def udef) {
  CHECK_BB(env_storage(env, udef->flag, udef->pos));
  CHECK_BB(scan0_global(env, udef->flag, udef->pos));
  const bool global = GET_FLAG(udef, global); // TODO: handle global in class
  if (global) {
    context_global(env);
    env_push_global(env);
  }
  if (GET_FLAG(udef, global)) context_global(env);
  CHECK_BB(scan0_defined(env, udef->xid, udef->pos));
  udef->type   = union_type(env, udef->xid, udef->pos);
  SET_ACCESS(udef, udef->type);
  if (udef->tmpl) union_tmpl(env, udef);
  if (global) env_pop(env, 0);
  set_tflag(udef->type, tflag_scan0);
  return GW_OK;
}

ANN static m_bool scan0_class_def_pre(const Env env, const Class_Def cdef) {
  CHECK_BB(env_storage(env, cdef->flag, cdef->pos));
  CHECK_BB(isres(env, cdef->base.xid, cdef->pos));
  return GW_OK;
}

ANN static inline void cdef_flag(const Class_Def cdef, const Type t) {
  if (cdef->base.tmpl) set_tflag(t, tflag_tmpl);
  if (cdef->base.ext && cdef->base.ext->array) set_tflag(t, tflag_typedef);
}

ANN static Type get_parent_base(const Env env, Type_Decl *td) {
  DECL_OO(const Type, t, = find_type(env, td));
  Type owner = env->class_def;
  while (owner) {
    if (t == owner)
      ERR_O(td->pos, _("'%s' as parent inside itself\n."), owner->name);
    owner = owner->info->value->from->owner_class;
  }
  return t;
}

ANN static inline Type scan0_final(const Env env, Type_Decl *td) {
  DECL_OO(const Type, t, = known_type(env, td));
  if (!GET_FLAG(t, final)) return t;
  ERR_O(td->pos, _("can't inherit from final parent class '%s'\n."), t->name);
}

ANN static Type cdef_parent(const Env env, const Class_Def cdef) {
  if (cflag(cdef, cflag_struct)) return env->gwion->type[et_compound];
  if (!cdef->base.ext) return env->gwion->type[et_object];
  if (tmpl_base(cdef->base.tmpl)) return get_parent_base(env, cdef->base.ext);
  const bool tmpl = !!cdef->base.tmpl;
  if (tmpl) template_push_types(env, cdef->base.tmpl);
  const Type t = scan0_final(env, cdef->base.ext);
  if (tmpl) nspc_pop_type(env->gwion->mp, env->curr);
  return t ?: (Type)GW_ERROR;
}

ANN static m_bool find_traits(const Env env, ID_List traits, const loc_t pos) {
  for(uint32_t i = 0; i < traits->len; i++) {
    Symbol xid = *mp_vector_at(traits, Symbol, i);
    if (!nspc_lookup_trait1(env->curr, xid)) {
      gwerr_basic(_("can't find trait"), NULL, NULL, env->name, pos, 0);
      did_you_mean_trait(env->curr, s_name(xid));
      env_set_error(env);
      return GW_ERROR;
    }
  }
  return GW_OK;
}

ANN static Type scan0_class_def_init(const Env env, const Class_Def cdef) {
  CHECK_BO(scan0_defined(env, cdef->base.xid, cdef->pos));
  const Type parent = cdef_parent(env, cdef);
  if (parent == (Type)GW_ERROR) return NULL;
  if (cdef->traits) CHECK_BO(find_traits(env, cdef->traits, cdef->pos));
  const Type t = scan0_type(env, s_name(cdef->base.xid), parent);
  if (cflag(cdef, cflag_struct)) {
    t->size = 0;
    set_tflag(t, tflag_struct);
  }
  t->info->tuple  = new_tupleform(env->gwion->mp, parent);
  t->nspc         = new_nspc(env->gwion->mp, t->name);
  t->nspc->parent = env->curr;
  t->info->cdef   = cdef;
  t->flag |= cdef->flag;
  //add_type(env, t->info->value->from->owner, t);
  cdef_flag(cdef, t);
  return t;
}

ANN static m_bool scan0_stmt_list(const Env env, Stmt_List l) {
  for(m_uint i = 0; i < l->len; i++) {
    const Stmt stmt = mp_vector_at(l, struct Stmt_, i);
    if (stmt->stmt_type == ae_stmt_pp) {
      if (stmt->d.stmt_pp.pp_type == ae_pp_include)
        env->name = stmt->d.stmt_pp.data;
      else if (stmt->d.stmt_pp.pp_type == ae_pp_import)
        CHECK_BB(plugin_ini(env->gwion, stmt->d.stmt_pp.data, stmt->pos));
    }
  }
  return GW_OK;
}

ANN static Exp mk_default_args(const MemPool p, const Arg_List args, const uint32_t max) {
  Exp exp = NULL, base_exp = NULL;
  for(uint32_t i = 0; i < args->len; i++) {
    Arg *arg = mp_vector_at(args, Arg, i);
    const Exp arg_exp = new_prim_id(p, arg->var_decl.xid, arg->var_decl.pos);
    if(exp)
      exp = (exp->next = arg_exp);
    else
      base_exp = exp = arg_exp;
  }
  // now add default args
  for(uint32_t i = args->len; i < max; i++) {
    Arg *arg = mp_vector_at(args, Arg, i);
    const Exp arg_exp = cpy_exp(p, arg->exp);
    if(exp)
      exp = (exp->next = arg_exp);
    else
      base_exp = exp = arg_exp;
  }
  return base_exp;
}

ANN2(1) static void scan0_func_def_default(const MemPool p, const Section *s,
                                            Ast *acc) {
  const Func_Def base_fdef = s->d.func_def;
  Arg_List       args = base_fdef->base->args;
  const uint32_t len = args->len;
  while(args->len--) {
    Arg *arg = mp_vector_at(args, Arg, args->len);
    if(!arg->exp) break;
    Func_Base *base = new_func_base(
        p, base_fdef->base->td ? cpy_type_decl(p, base_fdef->base->td) : NULL, base_fdef->base->xid,
        cpy_arg_list(p, args),
        base_fdef->base->flag, base_fdef->base->pos);
    const Exp efunc = new_prim_id(p, base->xid, base->pos);
    const Exp exp_arg = mk_default_args(p, args, len);
    const Exp ecall = new_exp_call(p, efunc, exp_arg, base->pos);
    Stmt_List slist = new_mp_vector(p, sizeof(struct Stmt_), 1);
    mp_vector_set(slist, struct Stmt_, 0,
      ((struct Stmt_) {
        .stmt_type = ae_stmt_return, .d = { .stmt_exp = { .val = ecall }},
        .pos = base_fdef->base->pos
    }));
    const Stmt      body  = new_stmt_code(p, slist, base->pos);
    const Func_Def  fdef  = new_func_def(p, base, body);
    Section       section = MK_SECTION(func, func_def, fdef);
    mp_vector_add(p, acc, Section, section);
  }
  args->len = len;
}

#define scan0_func_def dummy_func

ANN static m_bool scan0_extend_def(const Env env, const Extend_Def xdef) {
  DECL_OB(const Type, t, = known_type(env, xdef->td));
  if (isa(t, env->gwion->type[et_compound]) < 0)
    ERR_B(xdef->td->pos, _("only compound types can be extended"))
  if (GET_FLAG(t, final)) // TODO: add type initial declaration
    ERR_B(xdef->td->pos, _("can't extend final type"))
  Ast ast = xdef->body;
  for(m_uint i = 0; i < ast->len; i++) {
    Section * section = mp_vector_at(ast, Section, i);
    if (section->section_type == ae_section_func &&
        GET_FLAG(section->d.func_def->base, abstract))
      ERR_B(section->d.func_def->base->pos,
            _("can't use {/+}abstract{0} functions in {+/}extends{0}"))
  }
  xdef->t = t;
  return GW_OK;
}

ANN static m_bool _scan0_trait_def(const Env env, const Trait_Def pdef) {
  const Trait trait = new_trait(env->gwion->mp, pdef->pos);
  trait->loc        = pdef->pos;
  trait->name       = s_name(pdef->xid);
  trait->filename   = env->name;
  nspc_add_trait(env->curr, pdef->xid, trait);
  Ast ast = pdef->body;
  if(!ast) return GW_OK; // ???
  for(m_uint i = 0; i < ast->len; i++) {
    Section *section = mp_vector_at(ast, Section, i);
    if (section->section_type == ae_section_func) {
      const Func_Def fdef = section->d.func_def;
      if (!trait->requested_funcs.ptr) vector_init(&trait->requested_funcs);
      vector_add(&trait->requested_funcs, (m_uint)fdef);
    }
  }
  return GW_OK;
}

ANN static m_bool scan0_trait_def(const Env env, const Trait_Def pdef) {
  const Symbol s      = pdef->xid;
  const Trait  exists = nspc_lookup_trait1(env->curr, s);
  if (exists) {
    gwerr_basic("trait already defined", NULL, NULL, env->name, pdef->pos, 0);
    gwerr_secondary("defined here", env->name, exists->loc);
    env_set_error(env);
    return already_defined(env, s, pdef->pos);
  }
  if (pdef->traits) CHECK_BB(find_traits(env, pdef->traits, pdef->pos));
  _scan0_trait_def(env, pdef);
  return GW_OK;
}

HANDLE_SECTION_FUNC(scan0, m_bool, Env)

ANN static m_bool scan0_class_def_inner(const Env env, const Class_Def cdef) {
  CHECK_OB((cdef->base.type = scan0_class_def_init(env, cdef)));
  cdef->base.type->info->traits = cdef->traits;
  set_tflag(cdef->base.type, tflag_scan0);
  (void)mk_class(env, cdef->base.type, cdef->pos);
  add_type(env, cdef->base.type->info->value->from->owner, cdef->base.type);
  const m_uint scope = env_push(env, cdef->base.type->info->value->from->owner_class, cdef->base.type->info->value->from->owner);
  const m_bool ret = cdef->body ? env_body(env, cdef, scan0_section) : GW_OK;
  env_pop(env, scope);
  return ret;
}

ANN m_bool scan0_class_def(const Env env, const Class_Def c) {
  CHECK_BB(scan0_global(env, c->flag, c->pos));
  const int       cpy  = tmpl_base(c->base.tmpl) || GET_FLAG(c, global);
  const Class_Def cdef = !cpy ? c : cpy_class_def(env->gwion->mp, c);
  if (GET_FLAG(cdef, global)) { // could be updated
    vector_add(&env->scope->nspc_stack, (vtype)env->curr);
    env->curr = env->global_nspc;
    context_global(env);
  }
  const m_bool ret = scan0_class_def_pre(env, cdef) > 0
                         ? scan0_class_def_inner(env, cdef)
                         : GW_ERROR;

  if (GET_FLAG(cdef, global))
    env->curr = (Nspc)vector_pop(&env->scope->nspc_stack);
  if (cpy && cdef->base.type) {
    c->base.type = cdef->base.type;
    c->base.type->info->cdef = cdef;
    set_tflag(c->base.type, tflag_cdef);
  }
//  if (GET_FLAG(cdef, global))
//    type_addref(c->base.type);
  return ret;
}

ANN m_bool scan0_ast(const Env env, Ast *ast) {
  Ast a = *ast;
  Ast acc = new_mp_vector(env->gwion->mp, sizeof(Section), 0);
  for(m_uint i = 0; i < a->len; i++) {
    Section * section = mp_vector_at(a, Section, i);
    CHECK_BB(scan0_section(env, section));

    if (section->section_type != ae_section_func ||
        !fbflag(section->d.func_def->base, fbflag_default))
      continue;
    scan0_func_def_default(env->gwion->mp, section, &acc);

  }

  for(m_uint i = 0; i < acc->len; i++) {
    Section * section = mp_vector_at(acc, Section, i);
    mp_vector_add(env->gwion->mp, ast, Section, *section);
  }
  free_mp_vector(env->gwion->mp, sizeof(Section), acc);

  return GW_OK;
}
