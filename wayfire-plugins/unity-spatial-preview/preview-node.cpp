/* preview-node.cpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "preview-node.hpp"

#include <wayfire/scene-operations.hpp>
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
        auto mapping = self->mapping();
        if (!mapping)
        {
            return;
        }

        auto texture = source_texture(data.target.scale);
        auto bounds  = source_root->get_bounding_box();
        auto base    = texture->get_source_box().value_or(wlr_fbox{0, 0, double(texture->get_width()),
            double(texture->get_height())});
        double kx = base.width / bounds.width;
        double ky = base.height / bounds.height;
        texture->set_source_box(wlr_fbox{base.x + mapping->source.x * kx, base.y + mapping->source.y * ky,
            mapping->source.width * kx, mapping->source.height * ky});
        texture->set_filter_mode(WLR_SCALE_FILTER_BILINEAR);
        data.pass->add_texture(texture, data.target, mapping->box, data.damage);
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
    void regen_source_instances()
    {
        children.clear();
        source_root->gen_render_instances(children, [this] (const wf::regionf_t& damage)
        {
            self->cached_damage |= damage;
            push_damage(self->get_bounding_box());
        }, output);

        direct = zero_copy() != nullptr;
        if (direct)
        {
            self->release_buffers();
        }
    }

    std::shared_ptr<wf::texture_t> zero_copy() const
    {
        auto *single = dynamic_cast<wf::scene::zero_copy_texturable_node_t*>(source_root.get());
        return single ? single->to_texture() : nullptr;
    }

    std::shared_ptr<wf::texture_t> source_texture(float scale)
    {
        if (auto texture = direct ? zero_copy() : nullptr)
        {
            return texture;
        }

        return self->get_updated_contents(source_root->get_bounding_box(), scale, children, output);
    }

    wf::scene::floating_inner_ptr source_root;
    std::vector<wf::scene::render_instance_uptr> children;
    bool direct = false;

    wf::signal::connection_t<wf::scene::node_regen_instances_signal> on_source_regen = [this] (auto)
    {
        regen_source_instances();
    };
};
}

preview_node_t::preview_node_t(wayfire_view source, wlr_subsurface *subsurface) :
    transformer_base_node_t(false), source(source->weak_from_this()), subsurface(subsurface)
{}

std::optional<mapping_t> preview_node_t::mapping()
{
    auto view = source.lock();
    if (!view)
    {
        return std::nullopt;
    }

    return map_window(frame_of(view), view->get_surface_root_node()->get_bounding_box(),
        subsurface->surface->current.width, subsurface->surface->current.height);
}

wf::geometry_t preview_node_t::get_bounding_box()
{
    if (auto current = mapping())
    {
        return current->box;
    }

    return {0, 0, double(subsurface->surface->current.width), double(subsurface->surface->current.height)};
}

void preview_node_t::refresh()
{
    auto box = get_bounding_box();
    wf::regionf_t damage{last_box};
    damage |= box;
    last_box = box;
    wf::scene::damage_node(shared_from_this(), damage);
}

wf::geometry_t preview_node_t::get_clip() const
{
    return {-double(subsurface->current.x), -double(subsurface->current.y),
        double(subsurface->parent->current.width), double(subsurface->parent->current.height)};
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
