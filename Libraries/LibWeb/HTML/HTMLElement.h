/*
 * Copyright (c) 2018-2022, Andreas Kling <andreas@ladybird.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Optional.h>
#include <LibWeb/DOM/Element.h>
#include <LibWeb/HTML/EventNames.h>
#include <LibWeb/HTML/GlobalEventHandlers.h>
#include <LibWeb/HTML/HTMLOrSVGElement.h>
#include <LibWeb/HTML/ToggleTaskTracker.h>
#include <LibWeb/HTML/TokenizedFeatures.h>

namespace Web::HTML {

// https://html.spec.whatwg.org/multipage/dom.html#attr-dir
#define ENUMERATE_HTML_ELEMENT_DIR_ATTRIBUTES   \
    __ENUMERATE_HTML_ELEMENT_DIR_ATTRIBUTE(ltr) \
    __ENUMERATE_HTML_ELEMENT_DIR_ATTRIBUTE(rtl) \
    __ENUMERATE_HTML_ELEMENT_DIR_ATTRIBUTE(auto)

// https://html.spec.whatwg.org/multipage/interaction.html#attr-contenteditable
enum class ContentEditableState {
    True,
    False,
    PlaintextOnly,
    Inherit,
};

struct ShowPopoverOptions {
    GC::Ptr<HTMLElement> source;
};

struct TogglePopoverOptions : public ShowPopoverOptions {
    Optional<bool> force {};
};

using TogglePopoverOptionsOrForceBoolean = Variant<TogglePopoverOptions, bool>;

enum class ThrowExceptions {
    Yes,
    No,
};

enum class FocusPreviousElement {
    Yes,
    No,
};

enum class FireEvents {
    Yes,
    No,
};

enum class ExpectedToBeShowing {
    Yes,
    No,
};

enum class IgnoreDomState {
    Yes,
    No,
};

enum class IsPopover {
    Yes,
    No,
};

class HTMLElement
    : public DOM::Element
    , public HTML::GlobalEventHandlers
    , public HTML::HTMLOrSVGElement<HTMLElement> {
    WEB_PLATFORM_OBJECT(HTMLElement, DOM::Element);
    GC_DECLARE_ALLOCATOR(HTMLElement);

public:
    virtual ~HTMLElement() override;

    Optional<String> title() const { return attribute(HTML::AttributeNames::title); }

    bool translate() const;
    void set_translate(bool);

    StringView dir() const;
    void set_dir(String const&);

    virtual bool is_focusable() const override;
    bool is_content_editable() const;
    StringView content_editable() const;
    ContentEditableState content_editable_state() const { return m_content_editable_state; }
    WebIDL::ExceptionOr<void> set_content_editable(StringView);

    String inner_text();
    void set_inner_text(StringView);

    [[nodiscard]] String outer_text();
    WebIDL::ExceptionOr<void> set_outer_text(String const&);

    int offset_top() const;
    int offset_left() const;
    int offset_width() const;
    int offset_height() const;
    GC::Ptr<Element> offset_parent() const;
    GC::Ptr<Element> scroll_parent() const;

    bool cannot_navigate() const;

    Variant<bool, double, String> hidden() const;
    void set_hidden(Variant<bool, double, String> const&);

    void click();

    [[nodiscard]] String access_key_label() const;

    bool fire_a_synthetic_pointer_event(FlyString const& type, DOM::Element& target, bool not_trusted);

    // https://html.spec.whatwg.org/multipage/forms.html#category-label
    virtual bool is_labelable() const { return false; }

    GC::Ptr<DOM::NodeList> labels();

    virtual Optional<ARIA::Role> default_role() const override;

    String get_an_elements_target(Optional<String> target = {}) const;
    TokenizedFeature::NoOpener get_an_elements_noopener(URL::URL const& url, StringView target) const;

    WebIDL::ExceptionOr<GC::Ref<ElementInternals>> attach_internals();

    WebIDL::ExceptionOr<void> set_popover(Optional<String> value);
    Optional<String> popover() const;
    Optional<String> opened_in_popover_mode() const { return m_opened_in_popover_mode; }

    virtual void removed_from(Node* old_parent, Node& old_root) override;

    enum class PopoverVisibilityState {
        Hidden,
        Showing,
    };
    PopoverVisibilityState popover_visibility_state() const { return m_popover_visibility_state; }

    WebIDL::ExceptionOr<void> show_popover_for_bindings(ShowPopoverOptions const& = {});
    WebIDL::ExceptionOr<void> hide_popover_for_bindings();
    WebIDL::ExceptionOr<bool> toggle_popover(TogglePopoverOptionsOrForceBoolean const&);

    WebIDL::ExceptionOr<bool> check_popover_validity(ExpectedToBeShowing expected_to_be_showing, ThrowExceptions throw_exceptions, GC::Ptr<DOM::Document>, IgnoreDomState ignore_dom_state);
    WebIDL::ExceptionOr<void> show_popover(ThrowExceptions throw_exceptions, GC::Ptr<HTMLElement> invoker);
    WebIDL::ExceptionOr<void> hide_popover(FocusPreviousElement focus_previous_element, FireEvents fire_events, ThrowExceptions throw_exceptions, IgnoreDomState ignore_dom_state, GC::Ptr<HTMLElement> source);

    static void hide_all_popovers_until(Variant<GC::Ptr<HTMLElement>, GC::Ptr<DOM::Document>> endpoint, FocusPreviousElement focus_previous_element, FireEvents fire_events);
    static GC::Ptr<HTMLElement> topmost_popover_ancestor(GC::Ptr<DOM::Node> new_popover_or_top_layer_element, Vector<GC::Ref<HTMLElement>> const& popover_list, GC::Ptr<HTMLElement> invoker, IsPopover is_popover);

    static void light_dismiss_open_popovers(UIEvents::PointerEvent const&, GC::Ptr<DOM::Node>);

    bool is_inert() const { return m_inert; }

    virtual bool is_valid_invoker_command(String&) { return false; }
    virtual void invoker_command_steps(DOM::Element&, String&) { }

    bool is_form_associated_custom_element();

protected:
    HTMLElement(DOM::Document&, DOM::QualifiedName);

    virtual void initialize(JS::Realm&) override;

    virtual void attribute_changed(FlyString const& name, Optional<String> const& old_value, Optional<String> const& value, Optional<FlyString> const& namespace_) override;
    virtual WebIDL::ExceptionOr<void> cloned(DOM::Node&, bool) const override;
    virtual void inserted() override;

    virtual void visit_edges(Cell::Visitor&) override;

    // https://html.spec.whatwg.org/multipage/dom.html#block-rendering
    void block_rendering();
    // https://html.spec.whatwg.org/multipage/dom.html#unblock-rendering
    void unblock_rendering();
    // https://html.spec.whatwg.org/multipage/urls-and-fetching.html#potentially-render-blocking
    bool is_potentially_render_blocking();
    // https://html.spec.whatwg.org/multipage/urls-and-fetching.html#implicitly-potentially-render-blocking
    virtual bool is_implicitly_potentially_render_blocking() const { return false; }

    void set_inert(bool inert) { m_inert = inert; }
    void set_subtree_inertness(bool is_inert);

private:
    virtual bool is_html_element() const final { return true; }

    virtual void adjust_computed_style(CSS::ComputedProperties&) override;

    // ^HTML::GlobalEventHandlers
    virtual GC::Ptr<DOM::EventTarget> global_event_handlers_to_event_target(FlyString const&) override { return *this; }
    virtual void did_receive_focus() override;
    virtual void did_lose_focus() override;

    [[nodiscard]] String get_the_text_steps();
    GC::Ref<DOM::DocumentFragment> rendered_text_fragment(StringView input);

    GC::Ptr<DOM::NodeList> m_labels;

    void queue_a_popover_toggle_event_task(String old_state, String new_state, GC::Ptr<HTMLElement> source);

    static Optional<String> popover_value_to_state(Optional<String> value);
    void hide_popover_stack_until(Vector<GC::Ref<HTMLElement>> const& popover_list, FocusPreviousElement focus_previous_element, FireEvents fire_events);
    GC::Ptr<HTMLElement> nearest_inclusive_open_popover();
    GC::Ptr<HTMLElement> nearest_inclusive_target_popover_for_invoker();
    static void close_entire_popover_list(Vector<GC::Ref<HTMLElement>> const& popover_list, FocusPreviousElement focus_previous_element, FireEvents fire_events);
    static GC::Ptr<HTMLElement> topmost_clicked_popover(GC::Ptr<DOM::Node> node);
    size_t popover_stack_position();

    // https://html.spec.whatwg.org/multipage/custom-elements.html#attached-internals
    GC::Ptr<ElementInternals> m_attached_internals;

    // https://html.spec.whatwg.org/multipage/interaction.html#attr-contenteditable
    ContentEditableState m_content_editable_state { ContentEditableState::Inherit };

    // https://html.spec.whatwg.org/multipage/interaction.html#click-in-progress-flag
    bool m_click_in_progress { false };

    bool m_inert { false };

    // Popover API

    // https://html.spec.whatwg.org/multipage/popover.html#popover-visibility-state
    PopoverVisibilityState m_popover_visibility_state { PopoverVisibilityState::Hidden };

    // https://html.spec.whatwg.org/multipage/popover.html#popover-invoker
    GC::Ptr<HTMLElement> m_popover_invoker;

    // https://html.spec.whatwg.org/multipage/popover.html#popover-showing-or-hiding
    bool m_popover_showing_or_hiding { false };

    // https://html.spec.whatwg.org/multipage/popover.html#the-popover-attribute:toggle-task-tracker
    Optional<ToggleTaskTracker> m_popover_toggle_task_tracker;

    // https://html.spec.whatwg.org/multipage/popover.html#popover-close-watcher
    GC::Ptr<CloseWatcher> m_popover_close_watcher;

    Optional<String> m_opened_in_popover_mode;
};

}

namespace Web::DOM {

template<>
inline bool Node::fast_is<HTML::HTMLElement>() const { return is_html_element(); }

}
