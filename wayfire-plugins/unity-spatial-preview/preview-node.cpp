/* preview-node.cpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "preview-node.hpp"

#include <wayfire/toplevel-view.hpp>

namespace unity_spatial_preview
{
namespace
{
wf::geometry_t frame_of(wayfire_view view)
{
    if (auto toplevel = wf::toplevel_cast(view))
    {
        return toplevel->get_geometry();
    }

    return view->get_surface_root_node()->get_bounding_box();
}

class preview_instance_t : public wf::scene::simple_render_instance_t<preview_node_t>
{
  public:
    preview_instance_t(preview_node_t *node, wf::scene::floating_inner_ptr source_root,
        wf::scene::damage_callback push_damage, wf::output_t *output) :
        simple_render_instance_t(node, push_damage, output), source_root(std::move(source_root))
    {
        self->cached_damage |= this->source_root->get_bounding_box();
        this->source_root->connect(&on_source_regen);
        regen_source_instances();
    }

    void schedule_instructions(std::vector<wf::scene::render_instruction_t>& instructions,
        const wf::render_target_t& target, wf::regionf_t& damage) override
    {
        auto ours = damage & self->get_bounding_box() & self->get_clip();
        if (!ours.empty())
        {
            instructions.push_back({.instance = this, .target = target, .damage = std::move(ours)});
        }
    }

    void render(const wf::scene::render_instruction_t& data) override
    {
        auto texture = source_texture(data.target.scale);
        texture->set_filter_mode(WLR_SCALE_FILTER_BILINEAR);
        data.pass->add_texture(texture, data.target, texture_rect(), data.damage, self->alpha);
    }

    void presentation_feedback(wf::output_t *output) override
    {
        for (auto& child : children)
        {
            child->presentation_feedback(output);
        }
    }

    void compute_visibility(wf::output_t *output, wf::regionf_t&) override
    {
        wf::regionf_t visible{source_root->get_bounding_box()};
        wf::scene::compute_visibility_from_list(children, output, visible, {0, 0});
    }

  private:
    wf::geometry_t texture_rect()
    {
        auto rect   = self->get_bounding_box();
        auto bounds = source_root->get_bounding_box();
        auto view   = self->source.lock();
        auto frame  = view ? frame_of(view.get()) : bounds;

        if ((frame.width <= 0) || (frame.height <= 0))
        {
            return rect;
        }

        double sx = rect.width / frame.width;
        double sy = rect.height / frame.height;
        return {rect.x - (frame.x - bounds.x) * sx, rect.y - (frame.y - bounds.y) * sy,
            bounds.width * sx, bounds.height * sy};
    }

    void regen_source_instances()
    {
        children.clear();
        source_root->gen_render_instances(children, [this] (const wf::regionf_t& damage)
        {
            self->cached_damage |= damage;
            push_damage(self->get_bounding_box());
        }, output);
    }

    std::shared_ptr<wf::texture_t> source_texture(float scale)
    {
        auto *single = dynamic_cast<wf::scene::zero_copy_texturable_node_t*>(source_root.get());
        auto texture = single ? single->to_texture() : nullptr;
        if (texture && !texture->get_wait_timeline())
        {
            self->release_buffers();
            return texture;
        }

        return self->get_updated_contents(source_root->get_bounding_box(), scale, children, output);
    }

    wf::scene::floating_inner_ptr source_root;
    std::vector<wf::scene::render_instance_uptr> children;

    wf::signal::connection_t<wf::scene::node_regen_instances_signal> on_source_regen = [this] (auto)
    {
        regen_source_instances();
    };
};
}

preview_node_t::preview_node_t(wayfire_view source, wlr_surface *surface) :
    transformer_base_node_t(false), source(source->weak_from_this()), surface(surface)
{}

wf::geometry_t preview_node_t::get_bounding_box()
{
    return {0, 0, double(surface->current.width), double(surface->current.height)};
}

wf::geometry_t preview_node_t::get_clip() const
{
    auto *sub = wlr_subsurface_try_from_wlr_surface(surface);
    if (!sub || !sub->parent)
    {
        return {0, 0, double(surface->current.width), double(surface->current.height)};
    }

    return {-double(sub->current.x), -double(sub->current.y),
        double(sub->parent->current.width), double(sub->parent->current.height)};
}

void preview_node_t::gen_render_instances(std::vector<wf::scene::render_instance_uptr>& instances,
    wf::scene::damage_callback push_damage, wf::output_t *shown_on)
{
    auto view = source.lock();
    if (!view || generating)
    {
        return;
    }

    generating = true;
    instances.push_back(std::make_unique<preview_instance_t>(this, view->get_surface_root_node(), push_damage,
        shown_on));
    generating = false;
}

std::string preview_node_t::stringify() const
{
    return "unity-spatial-preview";
}
}
