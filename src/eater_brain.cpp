#include "eater_brain.hpp"

#include <stdexcept>

EaterBrain::EaterBrain(size_t input_count, size_t output_count)
    : input_count(input_count), output_count(output_count) {
    nodes.reserve(input_count + output_count);

    for (size_t i = 0; i < input_count; ++i) {
        nodes.push_back(std::make_unique<Node>(Node{NodeType::Input, {}, 0.0f, 0.0f}));
    }
    for (size_t i = 0; i < output_count; ++i) {
        nodes.push_back(std::make_unique<Node>(Node{NodeType::Output, {}, 0.0f, 0.0f}));
    }
}

EaterBrain::EaterBrain(const EaterBrain& other)
    : input_count(other.input_count), output_count(other.output_count) {
    clone_from(other);
}

EaterBrain& EaterBrain::operator=(const EaterBrain& other) {
    if (this != &other) {
        input_count = other.input_count;
        output_count = other.output_count;
        clone_from(other);
    }
    return *this;
}

void EaterBrain::set_input(size_t input_index, float value) {
    if (input_index >= input_count) {
        throw std::out_of_range("input_index out of range");
    }
    nodes[input_index]->output_register = clamp01(value);
}

void EaterBrain::update() {
    if (nodes.empty()) {
        return;
    }

    // Compute new input registers based on previous output registers.
    std::vector<float> new_input_registers(nodes.size(), 0.0f);
    for (size_t i = 0; i < nodes.size(); ++i) {
        const auto* node = nodes[i].get();
        if (node->input_nodes.empty()) {
            new_input_registers[i] = 0.0f;
            continue;
        }

        float sum = 0.0f;
        for (const auto* source_node : node->input_nodes) {
            if (source_node) {
                sum += source_node->output_register;
            }
        }
        new_input_registers[i] = sum / static_cast<float>(node->input_nodes.size());
    }

    for (size_t i = 0; i < nodes.size(); ++i) {
        nodes[i]->input_register = new_input_registers[i];
    }

    // Update outputs; input nodes keep their externally set output.
    for (auto& node_ptr : nodes) {
        auto& node = *node_ptr;
        if (node.type == NodeType::Input) {
            continue;
        }

        float probability = clamp01(node.input_register);
        if (node.type == NodeType::Inverted) {
            node.output_register = (random_unit() < probability) ? 0.0f : 1.0f;
        } else {
            node.output_register = (random_unit() < probability) ? 1.0f : 0.0f;
        }
    }
}

float EaterBrain::read_output(size_t output_index) const {
    if (output_index >= output_count) {
        throw std::out_of_range("output_index out of range");
    }
    return nodes[input_count + output_index]->output_register;
}

float EaterBrain::read_output_input_register(size_t output_index) const {
    if (output_index >= output_count) {
        throw std::out_of_range("output_index out of range");
    }
    return nodes[input_count + output_index]->input_register;
}

void EaterBrain::mutate(float add_node_probability, float remove_node_probability, float add_connection_probability, float remove_connection_probability) {
    if (random_unit() < add_node_probability) {
        NodeType new_type = (random_unit() < 0.5f) ? NodeType::Hidden : NodeType::Inverted;
        add_hidden_node(new_type);
    }
    if (random_unit() < remove_node_probability) {
        remove_random_hidden_node();
    }
    if (random_unit() < add_connection_probability) {
        add_random_connection();
    }
    if (random_unit() < remove_connection_probability) {
        remove_random_connection();
    }
}

bool EaterBrain::has_hidden_nodes() const {
    return nodes.size() > input_count + output_count;
}

void EaterBrain::clone_from(const EaterBrain& other) {
    nodes.clear();
    nodes.reserve(other.nodes.size());

    for (const auto& other_node_ptr : other.nodes) {
        auto new_node = std::make_unique<Node>();
        new_node->type = other_node_ptr->type;
        new_node->input_register = other_node_ptr->input_register;
        new_node->output_register = other_node_ptr->output_register;
        nodes.push_back(std::move(new_node));
    }

    std::unordered_map<const Node*, size_t> index_map;
    for (size_t i = 0; i < other.nodes.size(); ++i) {
        index_map[other.nodes[i].get()] = i;
    }

    for (size_t i = 0; i < other.nodes.size(); ++i) {
        const auto& other_inputs = other.nodes[i]->input_nodes;
        auto& new_inputs = nodes[i]->input_nodes;
        new_inputs.clear();
        new_inputs.reserve(other_inputs.size());
        for (const Node* other_input : other_inputs) {
            auto it = index_map.find(other_input);
            if (it != index_map.end()) {
                new_inputs.push_back(nodes[it->second].get());
            }
        }
    }
}

void EaterBrain::add_hidden_node(NodeType type) {
    auto new_node = std::make_unique<Node>();
    new_node->type = type;
    new_node->input_register = 0.0f;
    new_node->output_register = 0.0f;

    nodes.push_back(std::move(new_node));
    size_t new_index = nodes.size() - 1;

    // Attach at least one input when possible; allow self and duplicates.
    size_t available_nodes = nodes.size();
    if (available_nodes > 0) {
        size_t input_links = (available_nodes == 1) ? 0 : 1;
        for (size_t i = 0; i < input_links; ++i) {
            size_t chosen = random_node_index();
            nodes[new_index]->input_nodes.push_back(nodes[chosen].get());
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
    Node* removed_node = nodes[chosen_slot].get();

    nodes.erase(nodes.begin() + static_cast<long>(chosen_slot));

    if (nodes.empty()) {
        return;
    }

    for (auto& node_ptr : nodes) {
        auto& input_nodes = node_ptr->input_nodes;
        input_nodes.erase(
            std::remove(input_nodes.begin(), input_nodes.end(), removed_node),
            input_nodes.end()
        );
    }
}

void EaterBrain::add_random_connection() {
    if (nodes.empty()) {
        return;
    }

    size_t target_index = random_node_index();
    Node& target = *nodes[target_index];
    target.input_nodes.push_back(nodes[random_node_index()].get());
}

void EaterBrain::remove_random_connection() {
    if (nodes.empty()) {
        return;
    }

    // Collect all connections
    std::vector<std::pair<size_t, size_t>> all_connections;
    for (size_t i = 0; i < nodes.size(); ++i) {
        for (size_t j = 0; j < nodes[i]->input_nodes.size(); ++j) {
            all_connections.push_back({i, j});
        }
    }

    if (all_connections.empty()) {
        return;
    }

    // Randomly choose a connection to remove
    size_t chosen = static_cast<size_t>(std::rand() % all_connections.size());
    size_t target_index = all_connections[chosen].first;
    size_t input_slot = all_connections[chosen].second;

    nodes[target_index]->input_nodes.erase(
        nodes[target_index]->input_nodes.begin() + static_cast<long>(input_slot)
    );
}

size_t EaterBrain::random_node_index() const {
    if (nodes.empty()) {
        return 0;
    }
    return static_cast<size_t>(std::rand() % nodes.size());
}
