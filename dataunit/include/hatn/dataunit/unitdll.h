
#if 0

#ifdef HDU_V2_UNIT_EXPORT_SHARED

#ifdef HDU_V2_UNIT_EXPLICIT_INSTANTIATION
#undef HDU_V2_UNIT_EXPLICIT_INSTANTIATION
#endif

#define HDU_V2_UNIT_EXPLICIT_INSTANTIATION(UnitName) \
    }\
    template class HDU_V2_UNIT_EXPORT_SHARED HATN_DATAUNIT_META_NAMESPACE::unit_t<UnitName::unit_base_t>;\
    template class HDU_V2_UNIT_EXPORT_SHARED HATN_DATAUNIT_META_NAMESPACE::unit_t<decltype(UnitName::shared_unit_c)::type>;\
    template class HDU_V2_UNIT_EXPORT_SHARED HATN_DATAUNIT_META_NAMESPACE::managed_unit<HATN_DATAUNIT_META_NAMESPACE::unit_t<UnitName::unit_base_t>>;\
    template class HDU_V2_UNIT_EXPORT_SHARED HATN_DATAUNIT_META_NAMESPACE::shared_managed_unit<HATN_DATAUNIT_META_NAMESPACE::unit_t<decltype(UnitName::shared_unit_c)::type>>;\
    namespace UnitName {

#else

#ifdef HDU_V2_UNIT_EXPORT

#ifdef HDU_V2_UNIT_EXPLICIT_INSTANTIATION
#undef HDU_V2_UNIT_EXPLICIT_INSTANTIATION
#endif

#define HDU_V2_UNIT_EXPLICIT_INSTANTIATION(UnitName) \
    }\
    template class HDU_V2_UNIT_EXPORT HATN_DATAUNIT_META_NAMESPACE::unit_t<UnitName::unit_base_t>;\
    template class HDU_V2_UNIT_EXPORT HATN_DATAUNIT_META_NAMESPACE::managed_unit<HATN_DATAUNIT_META_NAMESPACE::unit_t<UnitName::unit_base_t>>;\
    namespace UnitName {

#endif

#endif

#endif
