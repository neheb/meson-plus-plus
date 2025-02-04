// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

/// Does this block have only one parent?
inline bool is_strictly_dominated(const BasicBlock * block) { return block->parents.size() == 1; }

/// Get a variable from an Object
inline auto get_variable = [](const auto & obj) { return obj->var; };

/// Allows comparing Phi pointers, using the value comparisons
struct PhiComparator {
    bool operator()(const Phi * l, const Phi * r) const { return *l < *r; };
};

template <typename T> struct reversion_wrapper { T & iterable; };

template <typename T> auto begin(reversion_wrapper<T> w) { return std::rbegin(w.iterable); }

template <typename T> auto end(reversion_wrapper<T> w) { return std::rend(w.iterable); }

template <typename T> reversion_wrapper<T> reverse(T && iterable) { return {iterable}; }

} // namespace

bool insert_phis(BasicBlock * block, ValueTable & values) {
    // If there is only one path into this block then we don't need to worry
    // about variables, they should already be strictly dominated in the parent
    // blocks.
    if (block->parents.empty() || is_strictly_dominated(block)) {
        return false;
    }

    /*
     * Now calculate the phi nodes
     *
     * We can't rely on all branches defining all variables (we haven't checked
     * things like, does this branch actually continue?)
     * https://github.com/dcbaker/meson-plus-plus/issues/57
     *
     * So, we need to check each parent for variables, and if they exist in more
     * than one branch we need to insert a phi node.
     *
     * XXX: What happens if a variable is erroniously undefined in a branch?
     */
    std::list<Object> phis{};

    // Find all phis in the block arleady, so we don't re-add them
    std::set<Phi *, PhiComparator> existing_phis{};
    for (const auto & obj : block->instructions) {
        if (std::holds_alternative<std::unique_ptr<Phi>>(obj)) {
            existing_phis.emplace(std::get<std::unique_ptr<Phi>>(obj).get());
        }
    }

    // Create a set of all variables in all parents, and one of dominated variables
    // TODO: we could probably do less rewalking here
    std::set<std::string> all_vars{};
    std::set<std::string> dominated{};
    for (const auto & p : block->parents) {
        for (const auto & i : p->instructions) {
            if (auto v = std::visit(get_variable, i)) {
                if (all_vars.count(v.name)) {
                    dominated.emplace(v.name);
                }
                all_vars.emplace(v.name);
            }
        }
    }

    // For variables that are dominated, create phi nodes. The first value will
    // be a phi of two parent values, but any additional phis will be either the
    // previous phi or a parent value.
    for (const auto & name : dominated) {
        uint32_t last = 0;
        for (const auto p : block->parents) {
            for (const auto & i : reverse(p->instructions)) {
                if (const auto & var = std::visit(get_variable, i)) {
                    if (last) {
                        auto phi = std::make_unique<Phi>(last, var.version, Variable{name});
                        if (!existing_phis.count(phi.get())) {
                            // Only set the version if we're actually using this phi
                            phi->var.version = ++values[name];
                            last = phi->var.version;
                            phis.emplace_back(std::move(phi));
                        }
                    } else {
                        last = var.version;
                    }
                    break;
                }
            }
        }
    }

    if (phis.empty()) {
        return false;
    }

    block->instructions.splice(block->instructions.begin(), phis);
    return true;
}

bool fixup_phis(BasicBlock * block) {
    bool progress = false;
    for (auto it = block->instructions.begin(); it != block->instructions.end(); ++it) {
        if (std::holds_alternative<std::unique_ptr<Phi>>(*it)) {
            const auto & phi = std::get<std::unique_ptr<Phi>>(*it);
            bool right = false;
            bool left = false;
            for (const auto & p : block->parents) {
                for (const Object & i : p->instructions) {
                    const auto & var = std::visit([](const auto & obj) { return obj->var; }, i);
                    if (var.name == phi->var.name) {
                        if (var.version == phi->left) {
                            left = true;
                            break;
                        } else if (var.version == phi->right) {
                            right = true;
                            break;
                        }
                    }
                }
                if (left && right) {
                    break;
                }
            }

            if (left ^ right) {
                progress = true;
                auto id = std::make_unique<Identifier>(phi->var.name, left ? phi->left : phi->right,
                                                       Variable{phi->var});
                it = block->instructions.erase(it);
                it = block->instructions.emplace(it, std::move(id));
                continue;
            }

            // While we are walking the instructions in this block, we know that
            // if one side was found, then the other is found that the first
            // found is dead code after the second, so we can ignore it and
            // treat the second one as the truth
            for (auto it2 = block->instructions.begin(); it2 != it; ++it2) {
                const auto & var = std::visit([](const auto & obj) { return obj->var; }, *it2);
                if (var.name == phi->var.name) {
                    left = var.version == phi->left;
                    right = var.version == phi->right;
                }
            }

            if (left ^ right) {
                progress = true;
                auto id = std::make_unique<Identifier>(phi->var.name, left ? phi->left : phi->right,
                                                       Variable{phi->var});
                it = block->instructions.erase(it);
                it = block->instructions.emplace(it, std::move(id));
            }
        }
    }
    return progress;
}

} // namespace MIR::Passes
