/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/metautils.h
  *
  *      Utils for metaprogramming.
  *
  */

/****************************************************************************/

#ifndef HATNMETAUTILS_H
#define HATNMETAUTILS_H

#include <memory>
#include <type_traits>
#include <tuple>

#ifdef interface
#undef interface
#endif

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

     //! Get size of const char* at compile time
     size_t constexpr CStrLength(const char* str) noexcept
     {
         return *str ? 1 + hatn::common::CStrLength(str + 1) : 0;
     }

     /****************************** Compilation time counter ************************************/
     /* first method
      * call once per scope HATN_PREPARE_COUNTERS()
      * then create counter with HATN_MAKE_COUNTER(Id)
     */
    #define HATN_MAX_COUNT 64
    #define HATN_MAKE_COUNTER(Id) inline auto _counter##Id (_ICount<1>) {return _Count<1>();}
    #define HATN_GET_COUNT(Id) (sizeof(_counter##Id (_ICount<HATN_MAX_COUNT + 1>())) - 1)
    #define HATN_INC_COUNT(Id) inline _Count<HATN_GET_COUNT(Id) + 2> _counter##Id (_ICount<2 + HATN_GET_COUNT(Id)>) {return _Count<HATN_GET_COUNT(Id) + 1>();}

    #define HATN_GET_COUNT_NS(Namespace,Id) (sizeof(Namespace::_counter##Id (Namespace::_ICount<HATN_MAX_COUNT + 1>())) - 1)
    #define HATN_INC_COUNT_NS(Namespace,Id) Namespace::_Count<HATN_GET_COUNT_NS(Namespace,Id) + 2> _counter##Id (Namespace::_ICount<2 + HATN_GET_COUNT_NS(Namespace,Id)>);

    #define HATN_PREPARE_COUNTERS() \
     template<unsigned int n> struct _Count { char data[n]; }; \
     template<int n> struct _ICount : public _ICount<n-1> {}; \
     template<> struct _ICount<0> {};

     /****************************** Check if variadic list contains a type **************************/

     template < typename Tp, typename... List >
         struct containsType : std::true_type {};

     template < typename Tp, typename Head, typename... Rest >
         struct containsType<Tp, Head, Rest...>
             : std::conditional< std::is_same<Tp, Head>::value,
                                     std::true_type,
                                 hatn::common::containsType<Tp, Rest...>
                                >::type {};

     template < typename Tp > struct containsType<Tp> : std::false_type {};

     /****************************** Concatenate type list incrementally ************************************/
     /*
      * How to use:
      *
      * 1. Declare single type
           template <int N> struct SingleTypeSample
            {
                using type=void;
            };
        2. Specialize types by index up to N:
           template <> struct SingleTypeSample<0>
            {
                using type=Type0;
            };
           template <> struct SingleTypeSample<1>
            {
                using type=Type1;
            };
            ...
           template <> struct SingleTypeSample<N>
            {
                using type=TypeN;
            };
        3. Specialize your variadic template with type list:

            template <typename ... TypeList> struct MyType
            {};

            using type=Concat<SingleTypeSample,N,MyType>::type;
      *
      *
      *
      *
      */

     template <typename ...Types> struct VariadicTypedef
     {};

     template <template <int> class SingleType, int N, template <typename ...> class T, typename ...Types>
         struct Concat
     {
         using type=typename Concat<SingleType,N-1,T,VariadicTypedef<typename SingleType<N>::type,Types...>>::type;
     };

     template <template <int> class SingleType, int N, template <typename ...> class T, typename ...Types>
         struct Concat<SingleType,N,T,VariadicTypedef<Types...>>
     {
         using type=typename Concat<SingleType,N,T,Types...>::type;
     };

     template <template <int> class SingleType, template <typename ...> class T, typename ...Types>
         struct Concat<SingleType,0,T,Types...>
     {
         using type=T<typename SingleType<0>::type,Types...>;
     };

     template <template <int> class SingleType, template <typename ...> class T, typename ...Types>
         struct Concat<SingleType,0,T,VariadicTypedef<Types...>>
     {
         using type=typename Concat<SingleType,0,T,Types...>::type;
     };

    // the same as Concat, but "shared" typedef is used instead of "type" in SingleType

     template <template <int> class SingleType, int N, template <typename ...> class T, typename ...Types>
         struct ConcatShared
     {
         using type=typename ConcatShared<SingleType,N-1,T,VariadicTypedef<typename SingleType<N>::shared,Types...>>::type;
     };

     template <template <int> class SingleType, int N, template <typename ...> class T, typename ...Types>
         struct ConcatShared<SingleType,N,T,VariadicTypedef<Types...>>
     {
         using type=typename ConcatShared<SingleType,N,T,Types...>::type;
     };

     template <template <int> class SingleType, template <typename ...> class T, typename ...Types>
         struct ConcatShared<SingleType,0,T,Types...>
     {
         using type=T<typename SingleType<0>::shared,Types...>;
     };

     template <template <int> class SingleType, template <typename ...> class T, typename ...Types>
         struct ConcatShared<SingleType,0,T,VariadicTypedef<Types...>>
     {
         using type=typename ConcatShared<SingleType,0,T,Types...>::type;
     };


    /****************************** Extendable template class base to construct packs of interfaces ************************************/

    //! Helper to get tuple's element by type name
    template<int Index, class Search, class First, class... Types>
    struct getT_internal
    {
        typedef typename getT_internal<Index + 1, Search, Types...>::type type;
        static constexpr int index = Index;
    };
    //! Helper to get tuple's element by type name
    template<int Index, class Search, class... Types>
    struct getT_internal<Index, Search, Search, Types...>
    {
     typedef getT_internal type;
     static constexpr int index = Index;
    };

    //! Get reference to tuple's element by type name
    template<class T, class... Types>
    constexpr T& getT(std::tuple<Types...>& tuple) noexcept
    {
        return std::get<getT_internal<0,T,Types...>::type::index>(tuple);
    }

    //! Get const reference to tuple's element by type name
    template<class T, class... Types>
    constexpr const T& getT(const std::tuple<Types...>& tuple) noexcept
    {
        return std::get<getT_internal<0,T,Types...>::type::index>(tuple);
    }

    //! Interfaces pack
    template <typename ...Interfaces> class VInterfacesPack
    {
        public:

            VInterfacesPack(Interfaces&&... vals):m_interfaces(std::forward<Interfaces>(vals)...)
            {}

            VInterfacesPack()=default;
            ~VInterfacesPack()=default;
            VInterfacesPack(const VInterfacesPack&)=default;
            VInterfacesPack(VInterfacesPack&&)=default;
            VInterfacesPack& operator=(const VInterfacesPack&)=default;
            VInterfacesPack& operator=(VInterfacesPack&&)=default;

            //! Type by index
            template <int Index> using type=typename std::tuple_element<Index, std::tuple<Interfaces...>>::type;

            //! Get position of interface in the tuple
            template <typename T> constexpr inline static int getInterfacePos() noexcept
            {
                return getT_internal<0,T,Interfaces...>::type::index;
            }

            //! Get interface by type
            template <typename T> constexpr T& getInterface() noexcept
            {
                return getT<T>(m_interfaces);
            }

            //! Get interface by type
            template <typename T> constexpr const T& getInterface() const noexcept
            {
                return getT<T>(m_interfaces);
            }

            //! Get interface by index
            template <int Index>
            constexpr typename std::tuple_element<Index, std::tuple<Interfaces...>>::type&
                    getInterface() noexcept
            {
                return std::get<Index>(m_interfaces);
            }

			//! Get const interface by index
			template <int Index>
            constexpr const typename std::tuple_element<Index, std::tuple<Interfaces...>>::type&
                getInterface() const noexcept
			{
				return std::get<Index>(m_interfaces);
			}

            constexpr static const int InterfaceCount=sizeof...(Interfaces);
            constexpr static const int MaxInterfaceIndex=InterfaceCount-1;

        protected:

            std::tuple<Interfaces...> m_interfaces;
    };

    //! Base class for classes with interfaces
    class InterfacesBase
    {
        //! Get specific interface
        template <typename Interface,typename ...Interfaces>
        constexpr static Interface& interface(common::VInterfacesPack<Interfaces...>* interfaces) noexcept
        {
            return interfaces->template getInterface<Interface>();
        }

        //! Get specific interface
        template <typename Interface,typename ...Interfaces>
        constexpr static const Interface& interface(const common::VInterfacesPack<Interfaces...>* interfaces) noexcept
        {
            return interfaces->template getInterface<Interface>();
        }
    };

    //! Get template arg from enum
    template <typename T> constexpr static int EnumToTemplateArg(T t) noexcept
    {
        return static_cast<int>(t);
    }

    //! Traits helpers to work with pointer types
    template <typename T, typename=void> struct PointerTraits
    {
        static_assert(std::is_pointer<T>::value,"Type must be either raw or smart pointer!");
    };
    template <typename T> struct PointerTraits<T,std::enable_if_t<std::is_pointer<T>::value>>
    {
        using Type=typename std::remove_pointer<T>::type;
        using Pointer=T;
        static inline Pointer pointer(T val) noexcept
        {
            return val;
        }
    };
    template <template <typename> class T1, typename T> struct PointerTraits<T1<T>>
    {
        using Type=T;
        using Pointer=T*;
        static inline Pointer pointer(const T1<T>& val) noexcept
        {
            return val.get();
        }
    };

    //! Get type of last argument in parameters pack
    template <typename ... Args> struct LastArg
    {
        using type=typename std::tuple_element<sizeof...(Args)-1, std::tuple<Args...> >::type;
    };

    namespace detail
    {
        template <typename T,typename Enable,typename ...Args> struct ConstructWithArgsOrDefault
        {
        };
        template <typename T,typename ...Args> struct ConstructWithArgsOrDefault<
                    T,
                    std::enable_if_t<std::is_constructible<T,Args...>::value>,
                    Args...
                >
        {
            constexpr inline static T f(Args&&... args)
            {
                return T(std::forward<Args>(args)...);
            }
        };
        template <typename T,typename ...Args> struct ConstructWithArgsOrDefault<
                    T,
                    std::enable_if_t<!std::is_constructible<T,Args...>::value>,
                    Args...
                >
        {
            constexpr inline static T f(Args&&...)
            {
                return T();
            }
        };
    }
    //! Helper to construct value with initializer list or without the list depending on the constructor
    template <typename T,typename ...Args> struct ConstructWithArgsOrDefault
    {
        constexpr inline static T f(Args&&... args)
        {
            return detail::ConstructWithArgsOrDefault<T,void,Args...>::f(std::forward<Args>(args)...);
        }
    };

    #define _HATN_CONCAT(x,y,z) x ## y ## z
    #define HATN_CONCAT(x,y,z) _HATN_CONCAT(x,y,z)

    template <typename DerivedT, typename BaseT>
    DerivedT* dynamicCastWithSample(const BaseT* target,const DerivedT* sample) noexcept
    {
        auto offset=reinterpret_cast<uintptr_t>(static_cast<DerivedT*>(const_cast<DerivedT*>(sample)))-reinterpret_cast<uintptr_t>(sample);
        auto casted=reinterpret_cast<DerivedT*>(reinterpret_cast<uintptr_t>(target)-offset);
        return casted;
    }

    template <typename T, bool isConst> struct ConstTraits
    {};
    template <typename T> struct ConstTraits<T,true>
    {
        using type=const T;
    };
    template <typename T> struct ConstTraits<T,false>
    {
        using type=T;
    };

    template <typename T, typename=void> struct ReverseConst
    {
    };
    template <typename T> struct ReverseConst<T,
                                            std::enable_if_t<std::is_const<typename std::pointer_traits<T>::element_type>::value>>
    {
        using type=typename std::remove_const<typename std::pointer_traits<T>::element_type>::type*;
    };
    template <typename T> struct ReverseConst<T,
                                            std::enable_if_t<!std::is_const<typename std::pointer_traits<T>::element_type>::value>>
    {
        using type=const typename std::pointer_traits<T>::element_type*;
    };
}
}

#endif // HATNMETAUTILS_H
