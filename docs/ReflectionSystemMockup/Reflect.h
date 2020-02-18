#ifdef AS_REFLECT_TYPE
#undef AS_REFLECT_TYPE
#endif
#define AS_REFLECT_TYPE(_name, _content) (asReflectContainer){.name = _name, .data = {_content}};

#ifdef AS_ENTRY_SINGLE
#undef AS_ENTRY_SINGLE
#endif
#define AS_ENTRY_SINGLE(_structName, _type, _name) (asEntry){sizeof(_type), offsetof(_structName, _name), _type, _name},

#ifdef AS_ENTRY_ARRAY
#undef AS_ENTRY_ARRAY
#endif
#define AS_ENTRY_ARRAY(_structName, _type, _name, _count) (asEntry){sizeof(_type * _count), offsetof(_structName, _name),_type, _name},

#ifdef AS_ENTRY_UNION
#undef AS_ENTRY_UNION
#endif
#define AS_ENTRY_UNION(_content) 

#ifdef AS_ENTRY_STRUCT
#undef AS_ENTRY_STRUCT
#endif
#define AS_ENTRY_STRUCT(_content)