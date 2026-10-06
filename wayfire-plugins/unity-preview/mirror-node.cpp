/* mirror-node.cpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mirror-node.hpp"

namespace unity_preview
{
namespace
{
class mirror_instance_t : public wf::scene::simple_render_instance_t<mirror_node_t>
{
  public:
    mirror_instance_t(mirror_node_t *node, wf::scene::floating_inner_ptr source_root,
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
        auto ours = damage & self->get_bounding_box() & self->get_host_bounds();
        if (!ours.empty())
        {
            instructions.push_back({.instance = this, .target = target, .damage = std::move(ours)});
        }
    }

    void render(const wf::scene::render_instruction_t& data) override
    {
        auto texture = source_texture(data.target.scale);
        texture->set_filter_mode(WLR_SCALE_FILTER_BILINEAR);
        data.pass->add_texture(texture, data.target, self->get_bounding_box(), data.damage, self->alpha);
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

    wf::signal::connection_t<wf::scene::node_regen_instances_signal> on_source_regen =
        [this] (auto)
    {
        regen_source_instances();
    };
};
}

mirror_node_t::mirror_node_t(wayfire_view source, wlr_surface *host) :
    transformer_base_node_t(false), source(source->weak_from_this()), host(host)
{}

wf::geometry_t mirror_node_t::get_host_bounds() const
{
    return {0, 0, double(host->current.width), double(host->current.height)};
}

wf::geometry_t mirror_node_t::get_bounding_box()
{
    return rect;
}

void mirror_node_t::gen_render_instances(std::vector<wf::scene::render_instance_uptr>& instances,
    wf::scene::damage_callback push_damage, wf::output_t *shown_on)
{
    if (auto view = source.lock())
    {
        instances.push_back(std::make_unique<mirror_instance_t>(this, view->get_surface_root_node(),
            push_damage, shown_on));
    }
}

std::string mirror_node_t::stringify() const
{
    return "unity-preview mirror";
}
}
