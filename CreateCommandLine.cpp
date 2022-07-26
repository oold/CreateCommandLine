// SPDX-FileCopyrightText: 2022 Oliver Old <oliver.old@outlook.com>
// SPDX-License-Identifier: MIT

/*
MIT License

Copyright (c) 2022 Oliver Old

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next paragraph) shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "CreateCommandLine.hpp"

#pragma region Target architecture definitions
#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) &&                 \
    !defined(_ARM_) && !defined(_ARM64_) && defined(_M_IX86)
#define _X86_
#if !defined(_CHPE_X86_ARM64_) && defined(_M_HYBRID)
#define _CHPE_X86_ARM64_
#endif
#endif

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) &&                 \
    !defined(_ARM_) && !defined(_ARM64_) && defined(_M_AMD64)
#define _AMD64_
#endif

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) &&                 \
    !defined(_ARM_) && !defined(_ARM64_) && defined(_M_ARM)
#define _ARM_
#endif

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) &&                 \
    !defined(_ARM_) && !defined(_ARM64_) && defined(_M_ARM64)
#define _ARM64_
#endif

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) &&                 \
    !defined(_ARM_) && !defined(_ARM64_) && defined(_M_M68K)
#define _68K_
#endif

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) &&                 \
    !defined(_ARM_) && !defined(_ARM64_) && defined(_M_MPPC)
#define _MPPC_
#endif

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_M_IX86) && !defined(_AMD64_) &&                \
    !defined(_ARM_) && !defined(_ARM64_) && defined(_M_IA64)
#if !defined(_IA64_)
#define _IA64_
#endif
#endif

#ifndef _MAC
#if defined(_68K_) || defined(_MPPC_)
#define _MAC
#endif
#endif
#pragma endregion

#include <winerror.h>

#include <intsafe.h>

#include <cstddef>
#include <cstdlib>
#include <cwchar>
#include <memory>
#include <utility>

namespace createcommandline
{
namespace details
{
static long GetArgumentLength(const wchar_t *pszArgument, std::size_t &rcchArgument) noexcept
{
    long hr;

    if (!*pszArgument)
    {
        // Empty argument. Needs double quote plus NUL character.
        rcchArgument = 3;
        return S_FALSE;
    }

    auto bNeedsProcessing = false;
    const wchar_t *pCh;
    for (pCh = pszArgument; *pCh; ++pCh)
    {
        if (*pCh == L' ' || *pCh == L'\t' || *pCh == L'\n' || *pCh == L'\v' || *pCh == L'\"')
        {
            bNeedsProcessing = true;
            break;
        }
    }

    if (!bNeedsProcessing)
    {
        while (*pCh)
            ++pCh;

        // No modification needed.
        rcchArgument = pCh - pszArgument;
        return S_OK;
    }

    std::size_t cchArgument = 2; // Quote + quote.
    pCh = pszArgument;
    do
    {
        std::size_t cchBackslashes = 0;

        while (*pCh && *pCh == L'\\')
        {
            ++pCh;
            ++cchBackslashes;
        }

        if (!*pCh)
        {
            // Double backslashes.
            if (FAILED(hr = SizeTMult(cchBackslashes, 2, &cchBackslashes)))
                return hr;

            if (FAILED(hr = SizeTAdd(cchBackslashes, cchArgument, &cchArgument)))
                return hr;

            break;
        }

        if (*pCh == L'"')
        {
            // Double backslashes.
            if (FAILED(hr = SizeTMult(cchBackslashes, 2, &cchBackslashes)))
                return hr;

            // ... + backslash + quote.
            if (FAILED(hr = SizeTAdd(cchBackslashes, 2, &cchBackslashes)))
                return hr;

            if (FAILED(hr = SizeTAdd(cchBackslashes, cchArgument, &cchArgument)))
                return hr;
        }
        else
        {
            // Backslashes + next character.
            if (FAILED(hr = SizeTAdd(cchBackslashes, 1, &cchBackslashes)))
                return hr;

            if (FAILED(hr = SizeTAdd(cchBackslashes, cchArgument, &cchArgument)))
                return hr;
        }

        ++pCh;
    } while (*pCh);

    rcchArgument = cchArgument;
    return S_FALSE;
}

static wchar_t *EscapeArgument(wchar_t *pszDest, const wchar_t *pszArgument) noexcept
{
    auto pCh = pszArgument;
    *pszDest++ = L'"';

    do
    {
        std::size_t cchBackslashes = 0;
        while (*pCh && *pCh == L'\\')
        {
            ++pCh;
            ++cchBackslashes;
        }

        if (!*pCh)
        {
            const auto cchTheseBackslashes = cchBackslashes * 2;
            pszDest = std::wmemset(pszDest, L'\\', cchTheseBackslashes) + cchTheseBackslashes;
            break;
        }

        if (*pCh == L'"')
        {
            const auto cchTheseBackslashes = cchBackslashes * 2 + 1;
            pszDest = std::wmemset(pszDest, L'\\', cchTheseBackslashes) + cchTheseBackslashes;
            *pszDest++ = *pCh++;
            continue;
        }

        pszDest = std::wmemset(pszDest, L'\\', cchBackslashes) + cchBackslashes;
        *pszDest++ = *pCh++;
    } while (pCh);

    *pszDest++ = L'"';

    return pszDest;
}

void Free(void *pMem) noexcept
{
    std::free(pMem);
}
} // namespace details

long CreateCommandLine(const wchar_t *pszCommand, const wchar_t *const *ppszArguments,
                       CommandLinePtr &rpszCommandLine) noexcept
{
    using namespace details;

    long hr;
    long hrArgLen;
    void *pMem;

    if (!pszCommand)
        return E_INVALIDARG;

    std::size_t cpszArguments = 0;
    if (ppszArguments)
        while (ppszArguments[cpszArguments])
            ++cpszArguments;

    std::size_t cbacchArguments;
    if (FAILED(hr = SizeTMult(cpszArguments, sizeof(std::size_t), &cbacchArguments)))
        return hr;

    pMem = std::malloc(cbacchArguments);
    if (!pMem)
        return E_OUTOFMEMORY;

    // Elements contain either argument length (argument will be copied) or 0 (argument will be constructed).
    std::unique_ptr<std::size_t[], CommandLineDeleter> pcchArguments((std::size_t *)pMem);

    // Ditto.
    std::size_t cchCommand;
    if (FAILED(hrArgLen = GetArgumentLength(pszCommand, cchCommand)))
        return hrArgLen;

    std::size_t cchCommandLine = cchCommand;
    if (hrArgLen != S_OK)
        cchCommand = 0;

    for (std::size_t i = 0; i < cpszArguments; ++i)
    {
        if (FAILED(hrArgLen = GetArgumentLength(ppszArguments[i], pcchArguments[i])))
            return hrArgLen;

        // This size plus space character.
        if (FAILED(hr = SizeTAdd(cchCommandLine, pcchArguments[i], &cchCommandLine)))
            return hr;
        if (FAILED(hr = SizeTAdd(cchCommandLine, 1, &cchCommandLine)))
            return hr;

        if (hrArgLen != S_OK)
            pcchArguments[i] = 0;
    }

    // Total size plus NUL.
    if (FAILED(hr = SizeTAdd(cchCommandLine, 1, &cchCommandLine)))
        return hr;

    std::size_t cbszCommandLine;
    if (FAILED(hr = SizeTMult(cchCommandLine, sizeof(wchar_t), &cbszCommandLine)))
        return hr;

    pMem = std::malloc(cbszCommandLine);
    if (!pMem)
        return E_OUTOFMEMORY;
    CommandLinePtr pszCommandLine((wchar_t *)pMem);
    auto pCh = (wchar_t *)pMem;

    if (cchCommand)
        pCh = std::wmemcpy(pCh, pszCommand, cchCommand) + cchCommand;
    else
        pCh = EscapeArgument(pCh, pszCommand);

    for (std::size_t i = 0; i < cpszArguments; ++i)
    {
        *pCh++ = L' ';
        if (pcchArguments[i])
            pCh = std::wmemcpy(pCh, ppszArguments[i], pcchArguments[i]) + pcchArguments[i];
        else
            pCh = EscapeArgument(pCh, ppszArguments[i]);
    }

    *pCh = L'\0';

    rpszCommandLine = std::move(pszCommandLine);
    return S_OK;
}
} // namespace createcommandline
