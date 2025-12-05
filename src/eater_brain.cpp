#include "eater_brain.hpp"

#include <stdexcept>

EaterBrain::EaterBrain(size_t input_count, size_t output_count)
    : input_count(input_count), output_count(output_count) {
    nodes.reserve(input_count + output_count);

    for (size_t i = 0; i < input_count; ++i) {
        nodes.push_back({NodeType::Input, {}, 0.0f, 0.0f});
    }
    for (size_t i = 0; i < output_count; ++i) {
        nodes.push_back({NodeType::Output, {}, 0.0f, 0.0f});
    }
}

void EaterBrain::set_input(size_t input_index, float value) {
    if (input_index >= input_count) {
        throw std::out_of_range("input_index out of range");
    }
    nodes[input_index].output_register = clamp01(value);
}

void EaterBrain::update() {
    if (nodes.empty()) {
        return;
    }

    // Compute new input registers based on previous output registers.
    std::vector<float> new_input_registers(nodes.size(), 0.0f);
    for (size_t i = 0; i < nodes.size(); ++i) {
        const auto& node = nodes[i];
        if (node.input_indices.empty()) {
            new_input_registers[i] = 0.0f;
            continue;
        }

        float sum = 0.0f;
        for (size_t source_index : node.input_indices) {
            if (source_index < nodes.size()) {
                sum += nodes[source_index].output_register;
            }
        }
        new_input_registers[i] = sum / static_cast<float>(node.input_indices.size());
    }

    for (size_t i = 0; i < nodes.size(); ++i) {
        nodes[i].input_register = new_input_registers[i];
    }

    // Update outputs; input nodes keep their externally set output.
    for (auto& node : nodes) {
        if (node.type == NodeType::Input) {
            continue;
        }

        float probability = clamp01(node.input_register);
        node.output_register = (random_unit() < probability) ? 1.0f : 0.0f;
    }
}

float EaterBrain::read_output(size_t output_index) const {
    if (output_index >= output_count) {
        throw std::out_of_range("output_index out of range");
    }
    return nodes[input_count + output_index].output_register;
}

float EaterBrain::read_output_input_register(size_t output_index) const {
    if (output_index >= output_count) {
        throw std::out_of_range("output_index out of range");
    }
    return nodes[input_count + output_index].input_register;
}

void EaterBrain::mutate(float add_node_probability, float remove_node_probability, float rewire_probability) {
    if (random_unit() < add_node_probability) {
        add_hidden_node();
    }
    if (random_unit() < remove_node_probability) {
        remove_random_hidden_node();
    }
    if (random_unit() < rewire_probability) {
        rewire_random_node();
    }
}

bool EaterBrain::has_hidden_nodes() const {
    return nodes.size() > input_count + output_count;
}

void EaterBrain::add_hidden_node() {
    Node new_node;
    new_node.type = NodeType::Hidden;
    new_node.input_register = 0.0f;
    new_node.output_register = 0.0f;

    nodes.push_back(new_node);
    size_t new_index = nodes.size() - 1;

    // Attach at least one input when possible; allow self and duplicates.
    size_t available_nodes = nodes.size();
    if (available_nodes > 0) {
        size_t input_links = (available_nodes == 1) ? 0 : 1;
        for (size_t i = 0; i < input_links; ++i) {
            size_t chosen = random_node_index();
            nodes[new_index].input_indices.push_back(chosen);
        }
    }
}

void EaterBrain::remove_random_hidden_node() {
    if (!has_hidden_nodes()) {
        return;
    }

    std::vector<size_t> hidden_indices;
    for (size_t i = input_count + output_count; i < nodes.size(); ++i) {
        hidden_indices.push_back(i);
    }

    size_t chosen_slot = hidden_indices[static_cast<size_t>(std::rand() % hidden_indices.size())];

    nodes.erase(nodes.begin() + static_cast<long>(chosen_slot));

    if (nodes.empty()) {
        return;
    }

    for (auto& node : nodes) {
        for (auto& idx : node.input_indices) {
            if (idx == chosen_slot) {
                idx = random_node_index();
            } else if (idx > chosen_slot) {
                --idx;
            }
        }
    }
}

void EaterBrain::rewire_random_node() {
    if (nodes.empty()) {
        return;
    }

    size_t target_index = random_node_index();
    Node& target = nodes[target_index];

    if (target.input_indices.empty()) {
        target.input_indices.push_back(random_node_index());
    } else {
        size_t slot = static_cast<size_t>(std::rand() % target.input_indices.size());
        target.input_indices[slot] = random_node_index();
    }
}

size_t EaterBrain::random_node_index() const {
    if (nodes.empty()) {
        return 0;
    }
    return static_cast<size_t>(std::rand() % nodes.size());
}
