/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004-2018 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#pragma once

#include "HTMLElement.h"

namespace WebCore {

class HTMLFormControlElement;

class HTMLLegendElement final : public HTMLElement {
    WTF_MAKE_ISO_ALLOCATED(HTMLLegendElement);
public:
    static Ref<HTMLLegendElement> create(const QualifiedName&, Document&);

    WEBCORE_EXPORT HTMLFormElement* form() const final;

private:
    HTMLLegendElement(const QualifiedName&, Document&);

    // Control in the legend's field set that gets focus and access key.
    RefPtr<HTMLFormControlElement> associatedControl();

    void accessKeyAction(bool sendMouseEvents) final;
    void focus(const FocusOptions&) final;
};

} // namespace WebCore
