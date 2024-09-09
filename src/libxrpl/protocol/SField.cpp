//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012, 2013 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#include <xrpl/protocol/SField.h>
#include <cassert>
#include <string>
#include <string_view>
#include <utility>

namespace ripple {

// Storage for static const members.
SField::IsSigning const SField::notSigning;
int SField::num = 0;
std::map<int, SField const*> SField::knownCodeToField;

// Give only this translation unit permission to construct SFields
template <class T>
template <class... Args>
TypedField<T>::TypedField(Args&&... args) : SField(std::forward<Args>(args)...)
{
}

// Construct all compile-time SFields, and register them in the knownCodeToField
// database:

SField::SField(
    SerializedTypeID tid,
    int fv,
    const char* fn,
    int meta,
    IsSigning signing)
    : fieldCode(field_code(tid, fv))
    , fieldType(tid)
    , fieldValue(fv)
    , fieldName(fn)
    , fieldMeta(meta)
    , fieldNum(++num)
    , signingField(signing)
    , jsonName(fieldName.c_str())
{
    knownCodeToField[fieldCode] = this;
}

SField::SField(int fc)
    : fieldCode(fc)
    , fieldType(STI_UNKNOWN)
    , fieldValue(0)
    , fieldMeta(sMD_Never)
    , fieldNum(++num)
    , signingField(IsSigning::yes)
    , jsonName(fieldName.c_str())
{
    knownCodeToField[fieldCode] = this;
}

SField const&
SField::getField(int code)
{
    auto it = knownCodeToField.find(code);

    if (it != knownCodeToField.end())
    {
        return *(it->second);
    }
    return sfInvalid;
}

int
SField::compare(SField const& f1, SField const& f2)
{
    // -1 = f1 comes before f2, 0 = illegal combination, 1 = f1 comes after f2
    if ((f1.fieldCode <= 0) || (f2.fieldCode <= 0))
        return 0;

    if (f1.fieldCode < f2.fieldCode)
        return -1;

    if (f2.fieldCode < f1.fieldCode)
        return 1;

    return 0;
}

SField const&
SField::getField(std::string const& fieldName)
{
    for (auto const& [_, f] : knownCodeToField)
    {
        (void)_;
        if (f->fieldName == fieldName)
            return *f;
    }
    return sfInvalid;
}

}  // namespace ripple
