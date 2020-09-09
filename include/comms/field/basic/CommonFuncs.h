//
// Copyright 2017 - 2020 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <type_traits>
#include <iterator>

#include "comms/ErrorStatus.h"
#include "comms/util/Tuple.h"
#include "comms/field/details/FieldOpHelpers.h"

namespace comms
{

namespace field
{

namespace basic
{

struct CommonFuncs
{
    template <typename TField, typename TIter>
    static comms::ErrorStatus readSequence(TField& field, TIter& iter, std::size_t len)
    {
        field.clear();
        auto remLen = len;
        while (0 < remLen) {
            auto& elem = field.createBack();
            auto es = field.readElement(elem, iter, remLen);
            if (es != comms::ErrorStatus::Success) {
                field.value().pop_back();
                return es;
            }
        }

        return ErrorStatus::Success;
    }

    template <typename TField, typename TIter>
    static comms::ErrorStatus readSequenceN(TField& field, std::size_t count, TIter& iter, std::size_t& len)
    {
        field.clear();
        while (0 < count) {
            auto& elem = field.createBack();
            auto es = field.readElement(elem, iter, len);
            if (es != comms::ErrorStatus::Success) {
                field.value().pop_back();
                return es;
            }
            --count;
        }
        return comms::ErrorStatus::Success;
    }

    template <typename TField, typename TIter>
    static void readSequenceNoStatusN(TField& field, std::size_t count, TIter& iter)
    {
        field.clear();
        while (0 < count) {
            auto& elem = field.createBack();
            field.readElementNoStatus(elem, iter);
            --count;
        }
    }

    template <typename TField>
    static bool canWriteSequence(const TField& field)
    {
        for (auto& elem : field.value()) {
            if (!field.canWriteElement(elem)) {
                return false;
            }
        }

        return true;
    }

    template <typename TField, typename TIter>
    static comms::ErrorStatus writeSequence(const TField& field, TIter& iter, std::size_t len)
    {
        auto es = ErrorStatus::Success;
        auto remainingLen = len;
        for (auto& elem : field.value()) {
            if (!field.canWriteElement(elem)) {
                es = ErrorStatus::InvalidMsgData;
                break;
            }

            es = field.writeElement(elem, iter, remainingLen);
            if (es != comms::ErrorStatus::Success) {
                break;
            }
        }

        return es;
    }

    template <typename TField, typename TIter>
    static void writeSequenceNoStatus(TField& field, TIter& iter)
    {
        for (auto& elem : field.value()) {
            field.writeElementNoStatus(elem, iter);
        }
    }

    template <typename TField, typename TIter>
    static comms::ErrorStatus writeSequenceN(const TField& field, std::size_t count, TIter& iter, std::size_t& len)
    {
        auto es = ErrorStatus::Success;
        for (auto& elem : field.value()) {
            if (count == 0) {
                break;
            }

            es = field.writeElement(elem, iter, len);
            if (es != ErrorStatus::Success) {
                break;
            }

            --count;
        }

        return es;
    }

    template <typename TField, typename TIter>
    static void writeSequenceNoStatusN(const TField& field, std::size_t count, TIter& iter)
    {
        for (auto& elem : field.value()) {
            if (count == 0) {
                break;
            }

            field.writeElementNoStatus(elem, iter);
            --count;
        }
    }

    template <typename TIter>
    static void advanceWriteIterator(TIter& iter, std::size_t len)
    {
        using IterType = typename std::decay<decltype(iter)>::type;
        using ByteType = typename std::iterator_traits<IterType>::value_type;
        while (len > 0U) {
            *iter = ByteType();
            ++iter;
            --len;
        }
    }

    static constexpr std::size_t maxSupportedLength()
    {
        return 0xffff;
    }

    template <typename TFields>
    static constexpr bool areMembersVersionDependent()
    {
        return comms::util::tupleTypeAccumulate<TFields>(
            false, comms::field::details::FieldVersionDependentCheckHelper<>());
    }

    template <typename TFields>
    static constexpr bool doMembersMembersHaveNonDefaultRefresh()
    {
        return comms::util::tupleTypeAccumulate<TFields>(
            false, comms::field::details::FieldNonDefaultRefreshCheckHelper<>());
    }

    template <typename TFields, typename TVersionType>
    static bool setVersionForMembers(TFields& fields, TVersionType version)
    {
        return comms::util::tupleAccumulate(fields, false, makeVersionUpdater(version));
    }

#if COMMS_IS_MSVC_2017_OR_BELOW    
// #if 1
    template <typename... TFields>
    using IsAnyFieldVersionDependentBoolType = 
        typename comms::util::Conditional<
            comms::util::tupleTypeIsAnyOf<std::tuple<TFields...> >(
                comms::field::details::FieldVersionDependentCheckHelper<>())
        >::template Type<
            std::true_type,
            std::false_type
        >;

    template <typename... TFields>
    using FieldSelectMaxLengthIntType = 
            std::integral_constant<
                std::size_t, 
                comms::util::tupleTypeAccumulate<std::tuple<TFields...> >(
                    std::size_t(0), comms::field::details::FieldMaxLengthCalcHelper<>())
            >;

    template <typename... TFields>
    using HasAnyFieldNonDefaultRefreshBoolType = 
        typename comms::util::Conditional<
            comms::util::tupleTypeIsAnyOf<std::tuple<TFields...> >(
                comms::field::details::FieldNonDefaultRefreshCheckHelper<>())
        >::template Type<
            std::true_type,
            std::false_type
        >;    

#else // #if COMMS_IS_MSVC_2017_OR_BELOW
    template <typename... TFields>
    using IsAnyFieldVersionDependentBoolType = 
        typename comms::util::Accumulate<>::template Type<
            comms::util::FieldCheckVersionDependent,
            comms::util::LogicalOrBinaryOp,
            std::false_type,
            TFields...
        >; 

    template <typename... TFields>
    using FieldSelectMaxLengthIntType = 
        typename comms::util::Accumulate<>::template Type<
            comms::util::FieldMaxLengthIntType,
            comms::util::IntMaxBinaryOp,
            std::integral_constant<std::size_t, 0U>,
            TFields...
        >;    

    template <typename... TFields>
    using HasAnyFieldNonDefaultRefreshBoolType = 
        typename comms::util::Accumulate<>::template Type<
            comms::util::FieldCheckNonDefaultRefresh,
            comms::util::LogicalOrBinaryOp,
            std::false_type,
            TFields...
        >; 
#endif // #if COMMS_IS_MSVC_2017_OR_BELOW

private:

    template <typename TVerType>
    class VersionUpdater
    {
    public:
        explicit VersionUpdater(TVerType val) : version_(val) {}

        template <typename TField>
        bool operator()(bool soFar, TField& field) const
        {
            return field.setVersion(static_cast<typename TField::VersionType>(version_)) || soFar;
        }
    private:
        TVerType version_;
    };

    template <typename TVerType>
    static VersionUpdater<TVerType> makeVersionUpdater(TVerType val)
    {
        return VersionUpdater<TVerType>(val);
    }

};

} // namespace basic

} // namespace field

} // namespace comms
