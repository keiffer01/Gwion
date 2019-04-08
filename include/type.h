#ifndef __TYPE
#define __TYPE

struct Type_ {
  m_str     name;
  Nspc      nspc;
  Type      parent;
  Nspc      owner;
  Class_Def def;
  union type_data {
    Func      func;
    Type      base_type;
  } d;
  struct pool* p;
  size_t xid;
  size_t size;
  size_t array_depth;
  HAS_OBJ
  ae_flag flag;
};

Type t_void, t_int, t_bool, t_float, t_dur, t_time, t_now, t_complex, t_polar, t_vec3, t_vec4,
  t_null, t_object, t_shred, t_fork, t_event, t_ugen, t_string, t_ptr, t_array, t_gack,
  t_function, t_fptr, t_vararg, t_lambda, t_class, t_union;

ANN2(1,3) ANEW Type new_type(MemPool, const m_uint xid, const m_str name, const Type);
ANEW ANN Type type_copy(MemPool, const Type type);
ANN m_str get_type_name(const Env, const m_str, const m_uint);
ANN Value find_value(const Type, const Symbol);
ANN Func find_func(const Type, const Symbol);
ANN m_bool isa(const Type, const Type) __attribute__((pure));
ANN m_bool isres(const Symbol);
ANN Type array_type(const Env, const Type, const m_uint);
ANN Type find_common_anc(const Type, const Type) __attribute__((pure));
ANN m_uint id_list_len(ID_List);
ANN void type_path(const m_str, const ID_List);
ANN Type typedef_base(Type) __attribute__((pure));
ANN Type array_base(Type) __attribute__((pure));
ANN m_bool type_ref(Type) __attribute__((pure));
__attribute__((returns_nonnull))
ANN Type template_parent(const Env, const Type type);
static inline Type actual_type(const Type t) {
  return isa(t, t_class) > 0 ? t->d.base_type : t;
}
ANN static inline m_uint env_push_type(const Env env, const Type type) { return env_push(env, type, type->nspc); }
#endif

